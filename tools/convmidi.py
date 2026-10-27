#!/usr/bin/env python3
"""Stranded's MIDI, arranged for the MEGA65's four SIDs.

    python3 tools/convmidi.py <tune.mid> <build dir>

writes <build>/music.bin -- a crunched resource like the GFXn.BIN files --
and <build>/music_table.c with the instruments and where the streams are.

**This is an arrangement, not a conversion.** The tune is eleven parts:
upright bass, a guitar playing six- and seven-note chords, piano chords,
piano stabs, strings, clarinet, flute, congas, tambourine, maracas and
claves. At its busiest that is twenty-odd notes at once, and four SIDs are
twelve voices. So:

  - the bass and the three percussion lines get voices of their own, and the
    bass is played on two, one on a left-hand SID and one on a right-hand
    one, so that it sits in the middle (the MEGA65 mixes SIDs 1-2 75% left
    and 3-4 75% right; see xemu's audio65.c);
  - the rest share a pool of seven voices. Each part has a limit of its own,
    and a chord over its limit keeps its outer notes -- the bottom and the
    top are the harmony, the middle is colour -- and when the pool runs out
    a note takes the voice of the lowest-priority note sounding, or is not
    played;
  - everything is quantised to 50 Hz frames, which is when the player runs.

Then each of the twelve voices is one byte stream the player steps through a
frame at a time (src/music.c):

    $01-$7F       wait that many frames
    $80+i lo hi   note on: instrument i at SID frequency hi:lo
    $C0           note off
    $C1 lo hi     set the frequency (the bass's pitch bends)
    $FF           back to the start of this stream

Every stream lasts exactly the song, so they loop together.

**A voice is silent for at least one frame between notes.** A SID envelope
only restarts on a gate that goes from 0 to 1, so a note that follows
another on the same voice is cut a frame early to make room for the gap.
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
from convgfx import TAIL_PAD, crunch, find_exomizer  # noqa: E402

FRAME_US = 20000           # PAL
SID_CLOCK = 1000000        # the MEGA65's SIDs, as xemu clocks them

VOICES = 12
# Voice v is SID v // 3, voice v % 3. SIDs 0 and 1 lean left, 2 and 3 right.
POOL = [2, 3, 4, 5, 9, 10, 11]

# Instruments: control register (waveform), AD, SR, pulse width, and a
# frequency slide applied every frame the note sounds -- the congas' fall.
TRI, SAW, PULSE, NOISE = 0x10, 0x20, 0x40, 0x80
INSTRUMENTS = {
    "bass":        (PULSE, 0x09, 0x00, 0x600, 0),
    "guitar":      (PULSE, 0x0A, 0x00, 0x200, 0),
    "piano":       (PULSE, 0x0B, 0x00, 0x600, 0),
    "stab":        (PULSE, 0x05, 0x00, 0x800, 0),
    "strings":     (SAW,   0x80, 0xA9, 0x000, 0),
    "clarinet":    (PULSE, 0x30, 0xA6, 0x800, 0),
    "flute":       (TRI,   0x40, 0xB6, 0x000, 0),
    "conga_mute":  (TRI,   0x04, 0x00, 0x000, -40),
    "conga_open":  (TRI,   0x07, 0x00, 0x000, -40),
    "tambourine":  (NOISE, 0x05, 0x00, 0x000, 0),
    "maracas":     (NOISE, 0x02, 0x00, 0x000, 0),
    "claves":      (TRI,   0x03, 0x00, 0x000, 0),
}
INSTRUMENT_IDS = {name: i for i, name in enumerate(INSTRUMENTS)}

# Percussion: General MIDI note to instrument and a fixed pitch in Hz.
DRUMS = {
    62: ("conga_mute", 330),
    63: ("conga_open", 330),
    64: ("conga_open", 220),
    54: ("tambourine", 7000),
    70: ("maracas", 12000),
    75: ("claves", 2500),
}


class Part:
    def __init__(self, name, instrument, voices, limit, priority,
                 drums=False):
        self.name = name
        self.instrument = instrument
        self.voices = voices      # the voices it may use, preferred first
        self.limit = limit        # notes at once
        self.priority = priority  # who gives way when the pool is full
        self.drums = drums


# MIDI track number to part. Tracks 3 and 4 are the same piano.
PARTS = {
    1: Part("bass", "bass", [0], 1, 9),
    2: Part("guitar", "guitar", POOL, 3, 2),
    3: Part("piano", "piano", POOL[::-1], 2, 3),
    4: Part("piano", "piano", POOL[::-1], 2, 3),
    5: Part("stabs", "stab", POOL, 2, 4),
    6: Part("congas", None, [1], 1, 9, drums=True),
    7: Part("congas", None, [1], 1, 9, drums=True),
    8: Part("tambourine", None, [7], 1, 9, drums=True),
    9: Part("tambourine", None, [7], 1, 9, drums=True),
    10: Part("percussion", None, [8], 1, 9, drums=True),
    11: Part("strings", "strings", POOL[::-1], 2, 3),
    12: Part("clarinet", "clarinet", POOL, 2, 5),
    13: Part("flute", "flute", POOL, 2, 5),
}
BASS_MIRROR = 6  # voice 0's stream is played on this one as well


# --- MIDI ----------------------------------------------------------------

def read_midi(path):
    d = open(path, "rb").read()
    if d[:4] != b"MThd":
        sys.exit(f"{path}: not a MIDI file")
    _, ntracks, division = struct.unpack(">HHH", d[8:14])
    p, tracks = 14, []

    def varlen(q):
        v = 0
        while True:
            b = d[q]
            q += 1
            v = v << 7 | (b & 0x7F)
            if b < 0x80:
                return v, q

    for _ in range(ntracks):
        if d[p:p + 4] != b"MTrk":
            sys.exit(f"{path}: bad track")
        length = struct.unpack(">I", d[p + 4:p + 8])[0]
        q, end = p + 8, p + 8 + length
        p = end
        events, tick, status = [], 0, 0
        while q < end:
            delta, q = varlen(q)
            tick += delta
            b = d[q]
            if b == 0xFF:
                kind = d[q + 1]
                n, q = varlen(q + 2)
                events.append((tick, "meta", kind, d[q:q + n]))
                q += n
            elif b in (0xF0, 0xF7):
                n, q = varlen(q + 1)
                q += n
            else:
                if b & 0x80:
                    status = b
                    q += 1
                size = 1 if status & 0xF0 in (0xC0, 0xD0) else 2
                events.append((tick, status & 0xF0, status & 0x0F,
                               tuple(d[q:q + size])))
                q += size
        tracks.append(events)
    return division, tracks


def tempo(tracks):
    tempos = {e[3] for t in tracks for e in t
              if e[1] == "meta" and e[2] == 0x51}
    if len(tempos) != 1:
        sys.exit("the tune is expected to keep one tempo")
    return int.from_bytes(tempos.pop(), "big")


def sid_freq(hz):
    return min(0xFFFF, round(hz * (1 << 24) / SID_CLOCK))


def note_hz(note, bend=0):
    # A bend of +-8192 is +-2 semitones, the General MIDI default.
    return 440.0 * 2 ** ((note - 69 + bend / 4096.0) / 12)


# --- the arrangement -----------------------------------------------------

class Note:
    def __init__(self, start, end, part, pitch, instrument, freq):
        self.start, self.end = start, end
        self.part, self.pitch = part, pitch
        self.instrument, self.freq = instrument, freq
        self.voice = None
        self.bends = []  # (frame, freq)


def collect(division, tracks):
    us_per_tick = tempo(tracks) / division

    def frame(tick):
        return round(tick * us_per_tick / FRAME_US)

    notes, song_ticks = [], 0
    for number, events in enumerate(tracks):
        song_ticks = max(song_ticks, max((e[0] for e in events), default=0))
        part = PARTS.get(number)
        if not part:
            continue
        bend, sounding = 0, {}
        for tick, kind, channel, data in events:
            if kind == 0xE0:
                bend = (data[0] | data[1] << 7) - 8192
                for n in sounding.values():
                    n.bends.append((frame(tick), sid_freq(note_hz(n.pitch,
                                                                  bend))))
            elif kind == 0x90 and data[1] > 0:
                pitch = data[0]
                if part.drums:
                    if pitch not in DRUMS:
                        sys.exit(f"track {number}: no drum for note {pitch}")
                    name, hz = DRUMS[pitch]
                    n = Note(frame(tick), None, part, pitch, name,
                             sid_freq(hz))
                else:
                    n = Note(frame(tick), None, part, pitch,
                             part.instrument, sid_freq(note_hz(pitch, bend)))
                sounding[pitch] = n
                notes.append(n)
            elif kind == 0x80 or (kind == 0x90 and data[1] == 0):
                n = sounding.pop(data[0], None)
                if n:
                    n.end = max(frame(tick), n.start + 1)
    for n in notes:
        if n.end is None:
            sys.exit("a note never ends")
    # The tune is in 4/4: loop on the bar after the last event.
    bar = division * 4
    song_ticks = -(-song_ticks // bar) * bar
    return notes, frame(song_ticks)


def arrange(notes):
    """Give each note a voice, or none. Returns the notes that play."""
    # Chords in one go, most important part first, low to high.
    notes.sort(key=lambda n: (n.start, -n.part.priority, n.pitch))
    holder = [None] * VOICES   # the note each voice is playing, or last played
    dropped = {}

    def sounding(at):
        return [h for h in holder if h and h.end >= at]

    def cut(victim, at):
        # The new note starts at `at`; the old one has to be off a frame
        # before. A note that has not had a frame of its own is not played.
        victim.end = at - 1
        if victim.end <= victim.start:
            victim.voice = None
            dropped[victim.part.name] = dropped.get(victim.part.name, 0) + 1

    for n in notes:
        part = n.part
        mine = [h for h in sounding(n.start) if h.part is part]
        if len(mine) >= part.limit:
            # Over the part's limit: keep the outer notes, lose the middle.
            group = sorted(mine + [n], key=lambda h: h.pitch)
            middle = group[len(group) // 2] if len(group) > 2 else group[0]
            if part.limit == 1:
                middle = mine[0]  # one at a time: the newest wins
            if middle is n:
                dropped[part.name] = dropped.get(part.name, 0) + 1
                continue
            n.voice = middle.voice
            cut(middle, n.start)
            holder[n.voice] = n
            continue
        # A voice that is free, with a frame of silence before this note:
        # the one this part used last if there is one, so that an
        # instrument stays where it was.
        free = [v for v in part.voices
                if holder[v] is None or holder[v].end < n.start]
        if free:
            free.sort(key=lambda v: (holder[v] is None
                                     or holder[v].part is not part))
            n.voice = free[0]
            holder[n.voice] = n
            continue
        # The pool is full: take the voice of the least important note
        # sounding, if it is less important than this one.
        victims = [holder[v] for v in part.voices
                   if holder[v].part.priority < part.priority]
        if not victims:
            dropped[part.name] = dropped.get(part.name, 0) + 1
            continue
        victim = min(victims, key=lambda h: (h.part.priority, h.start))
        n.voice = victim.voice
        cut(victim, n.start)
        holder[n.voice] = n
    return [n for n in notes if n.voice is not None], dropped


# --- the streams ---------------------------------------------------------

def streams(notes, song_frames):
    events = [[] for _ in range(VOICES)]  # (frame, order, bytes)
    for n in notes:
        lo, hi = n.freq & 0xFF, n.freq >> 8
        on = bytes([0x80 | INSTRUMENT_IDS[n.instrument], lo, hi])
        events[n.voice].append((n.start, 1, on))
        last = {}
        for f, freq in n.bends:
            if n.start < f < n.end:
                last[f] = freq
        for f, freq in sorted(last.items()):
            events[n.voice].append((f, 2, bytes([0xC1, freq & 0xFF,
                                                 freq >> 8])))
        events[n.voice].append((n.end, 0, b"\xC0"))
    events[BASS_MIRROR] = list(events[0])

    out = []
    for v in range(VOICES):
        data, now = bytearray(), 0
        for f, _, b in sorted(events[v], key=lambda e: (e[0], e[1])):
            if f >= song_frames:
                continue
            data += wait(f - now)
            now = f
            data += b
        data += wait(song_frames - now) + b"\xFF"
        out.append(bytes(data))
    return out


def wait(frames):
    out = bytearray()
    while frames > 0:
        out.append(min(frames, 0x7F))
        frames -= out[-1]
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    midi, build = sys.argv[1], sys.argv[2]
    division, tracks = read_midi(midi)
    notes, song_frames = collect(division, tracks)
    total = len(notes)
    played, dropped = arrange(notes)
    voices = streams(played, song_frames)

    # The file: twelve 16-bit offsets to the streams, then the streams.
    offsets, blob = [], bytearray()
    for s in voices:
        offsets.append(2 * VOICES + len(blob))
        blob += s
    blob = struct.pack(f"<{VOICES}H", *offsets) + blob
    # The player indexes it through a __far pointer, whose index is signed.
    if len(blob) >= 0x8000:
        sys.exit(f"the streams are {len(blob)} bytes; the player reaches 32K")
    crunched = crunch(find_exomizer(), bytes(blob))
    with open(os.path.join(build, "music.bin"), "wb") as f:
        f.write(struct.pack("<I", len(crunched)) + crunched + TAIL_PAD)

    print(f"music: {song_frames} frames, {len(played)} of {total} notes, "
          f"{len(blob)} -> {len(crunched)} bytes")
    if dropped:
        print("  not played: " + ", ".join(f"{k} {v}" for k, v in
                                           sorted(dropped.items())))

    with open(os.path.join(build, "music_table.c"), "w") as f:
        f.write("// Generated by tools/convmidi.py. Do not edit.\n")
        f.write('#include "music.h"\n\n')
        f.write(f"const struct music_instrument music_instruments[] = {{\n")
        for name, (ctrl, ad, sr, pw, slide) in INSTRUMENTS.items():
            f.write(f"  {{0x{ctrl:02X}, 0x{ad:02X}, 0x{sr:02X}, "
                    f"0x{pw:03X}, {slide}}},  // {name}\n")
        f.write("};\n\n")



if __name__ == "__main__":
    main()
