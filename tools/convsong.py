#!/usr/bin/env python3
"""Christmas songs, written out by hand, arranged for the MEGA65's SIDs.

    python3 tools/convsong.py <build dir> <song.txt>...

writes <build>/music.bin -- a crunched resource like the GFXn.BIN files --
and <build>/music_table.c with the instruments. The songs are played one
after another, and the medley loops.

**A song is a melody and its chords** (assets/songs/*.txt):

    speed  5            frames a sixteenth note: 5 is 150 quarters a minute
    beats  4            quarter notes a bar: 4 or 3
    pickup 1            quarter notes in the first, short bar (default 0)
    bells  8            sleigh bells every eighth (8), quarter (4), or none (0)

    melody
      E5:4 E5:4 E5:2 | E5:4 E5:4 E5:2 | ...

    chords
      C | C | F C | G7 | ...

A note is its name and octave (C4 is middle C), a colon and its length: 1,
2, 4, 8 or 16, dotted with a `.`; `r` is a rest. `|` ends a bar, and every
bar must add up: the error names the song and the bar. A bar of chords is
divided evenly between them, or has one a beat with `.` holding the one
before (`C . D` in a waltz): C, Cm, C7, Cm7, Cdim, sharps and flats as
F#, Bb. `r` plays nothing under the bar, as under a pickup. A `#` that
starts a word starts a comment, to the end of the line.

**The arrangement is the same for every song**, and is all here:

  - the melody, on a pulse wave;
  - a bass on the beat, the chord's root, and its fifth on beat 3 of a bar
    of four that has one chord: oom-pah;
  - the chord on the off-beats (2 and 4, or 2 and 3 in a waltz), three short
    pulse stabs, voiced close together around G3-F#4;
  - sleigh bells, a short burst of high noise.

The melody and the bass are played on a left-hand SID and a right-hand one
alike, so that they sit in the middle (the MEGA65 mixes SIDs 1-2 75% left
and 3-4 75% right); the chord's three notes are spread across both sides.
A SID has one volume for its three voices, so the chords have two SIDs of
their own, a left and a right, and `VOLUME` turns them down.

Each of the twelve voices is one byte stream the player steps through a
frame at a time (src/music.c), and every stream lasts the whole medley:

    $01-$7F       wait that many frames
    $80+i lo hi   note on: instrument i at SID frequency hi:lo
    $C0           note off
    $FF           back to the start of this stream

A SID envelope only restarts on a gate that goes from 0 to 1, so every note
ends at least a frame before the next one on its voice begins.
"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(__file__))
from convgfx import TAIL_PAD, crunch, find_exomizer  # noqa: E402

SID_CLOCK = 1000000  # the MEGA65's SIDs, as xemu clocks them
VOICES = 12

# Instruments: control register (waveform), AD, SR and pulse width.
TRI, SAW, PULSE, NOISE = 0x10, 0x20, 0x40, 0x80
INSTRUMENTS = {
    "lead":  (TRI,   0x28, 0xA6, 0x000),  # soft: a flute more than a buzz
    "bass":  (PULSE, 0x09, 0x00, 0x600),
    "chord": (PULSE, 0x06, 0x00, 0x200),
    "bells": (NOISE, 0x02, 0x00, 0x000),
}
INSTRUMENT_IDS = {name: i for i, name in enumerate(INSTRUMENTS)}

# Voice v is SID v // 3, voice v % 3. SIDs 0 and 1 lean left, 2 and 3 right.
LEAD = [0, 6]
BASS = [1, 7]
BELLS = [2, 8]
CHORD = [3, 9, 4]  # low, middle, high: SIDs 1 and 3, the chords' own

# Each SID's master volume, 0-15: the mix.
VOLUME = [15, 11, 15, 11]
BELLS_HZ = 9000

NAMES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
QUALITIES = {"": (0, 4, 7), "m": (0, 3, 7), "7": (4, 7, 10),
             "m7": (3, 7, 10), "dim": (0, 3, 6)}


class SongError(Exception):
    pass


def pitch_class(name):
    m = re.fullmatch(r"([A-G])([#b]?)", name)
    if not m:
        raise SongError(f"no such note {name!r}")
    return (NAMES[m[1]] + {"#": 1, "b": -1, "": 0}[m[2]]) % 12


def note(token, where):
    """`E5:4.` to (MIDI note or None for a rest, sixteenths)."""
    m = re.fullmatch(r"([A-G][#b]?\d|r):(1|2|4|8|16)(\.?)", token)
    if not m:
        raise SongError(f"{where}: cannot read {token!r}")
    length = 16 // int(m[2])
    if m[3]:
        if length % 2:
            raise SongError(f"{where}: a dotted sixteenth is too short")
        length += length // 2
    if m[1] == "r":
        return None, length
    return pitch_class(m[1][:-1]) + 12 * (int(m[1][-1]) + 1), length


def chord(name, where):
    """`F#m7` to its root's pitch class and its notes' intervals."""
    m = re.fullmatch(r"([A-G][#b]?)(m7|m|7|dim|)", name)
    if not m:
        raise SongError(f"{where}: no such chord {name!r}")
    return pitch_class(m[1]), QUALITIES[m[2]]


def read_song(path):
    settings = {"speed": 6, "beats": 4, "pickup": 0, "bells": 0}
    sections = {"melody": "", "chords": ""}
    section = None
    for line in open(path):
        line = re.sub(r"(^|\s)#.*", "", line).strip()
        if not line:
            continue
        word = line.split()
        if word[0] in sections and len(word) == 1:
            section = word[0]
        elif word[0] in settings and len(word) == 2:
            settings[word[0]] = int(word[1])
        elif section:
            sections[section] += " " + line
        else:
            raise SongError(f"{path}: cannot read {line!r}")
    name = os.path.basename(path)
    melody = [b.split() for b in sections["melody"].split("|")]
    chords = [b.split() for b in sections["chords"].split("|")]
    while melody and not melody[-1]:
        melody.pop()
    while chords and not chords[-1]:
        chords.pop()
    if len(melody) != len(chords):
        raise SongError(f"{name}: {len(melody)} bars of melody and "
                        f"{len(chords)} of chords")
    return name, settings, melody, chords


def arrange(path):
    """A song as notes: (voice list, instrument, MIDI note or Hz, start
    frame, frames), and how many frames it lasts."""
    name, s, melody, chords = read_song(path)
    beats, speed = s["beats"], s["speed"]
    if beats not in (3, 4):
        raise SongError(f"{name}: {beats} beats a bar; 3 or 4")
    beat = 4 * speed
    notes, at = [], 0
    for n, (bar, names) in enumerate(zip(melody, chords)):
        where = f"{name} bar {n + 1}"
        length = 4 * (s["pickup"] if n == 0 and s["pickup"] else beats)

        t = 0
        for token in bar:
            pitch, sixteenths = note(token, where)
            if pitch is not None:
                notes.append((LEAD, "lead", pitch, at + t * speed,
                              sixteenths * speed - 2))
            t += sixteenths
        if t != length:
            raise SongError(f"{where}: {t} sixteenths, not {length}")

        bar_beats = length // 4
        if names != ["r"]:
            if bar_beats % len(names) or names[0] == ".":
                raise SongError(f"{where}: {len(names)} chords in "
                                f"{bar_beats} beats")
            # One a beat, `.` the one before.
            each = [names[b * len(names) // bar_beats]
                    for b in range(bar_beats)]
            for b in range(bar_beats):
                if each[b] == ".":
                    each[b] = each[b - 1]
            for b in range(bar_beats):
                root, intervals = chord(each[b], where)
                start = at + b * beat
                if b in ((0, 2) if beats == 4 else (0,)):
                    bass = 36 + root
                    if b and each[b] == each[b - 1]:  # within a chord: its fifth
                        bass += 7
                    notes.append((BASS, "bass", bass, start, beat - 4))
                if b in ((1, 3) if beats == 4 else (1, 2)):
                    # Each chord note into G3-F#4, then low to high.
                    tones = sorted(55 + (root + i - 7) % 12
                                   for i in intervals)
                    for v, tone in zip(CHORD, tones):
                        notes.append(([v], "chord", tone, start, beat // 2))
        if s["bells"]:
            step = 16 // s["bells"] * speed
            for f in range(0, length * speed, step):
                notes.append((BELLS, "bells", None, at + f, 2))
        at += length * speed
    return notes, at


def sid_freq(hz):
    return min(0xFFFF, round(hz * (1 << 24) / SID_CLOCK))


def wait(frames):
    out = bytearray()
    while frames > 0:
        out.append(min(frames, 0x7F))
        frames -= out[-1]
    return out


def streams(notes, frames):
    events = [[] for _ in range(VOICES)]  # (frame, order, bytes)
    for voices, instrument, pitch, start, length in notes:
        hz = BELLS_HZ if pitch is None else 440.0 * 2 ** ((pitch - 69) / 12)
        f = sid_freq(hz)
        on = bytes([0x80 | INSTRUMENT_IDS[instrument], f & 0xFF, f >> 8])
        for v in voices:
            events[v].append((start, 1, on))
            events[v].append((start + max(1, length), 0, b"\xC0"))
    out = []
    for v in range(VOICES):
        data, now = bytearray(), 0
        for f, _, b in sorted(events[v], key=lambda e: (e[0], e[1])):
            data += wait(f - now)
            now = f
            data += b
        data += wait(frames - now) + b"\xFF"
        out.append(bytes(data))
    return out


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    build, songs = sys.argv[1], sys.argv[2:]
    notes, frames = [], 0
    try:
        for path in songs:
            song, length = arrange(path)
            notes += [(v, i, p, frames + start, n)
                      for v, i, p, start, n in song]
            print(f"{os.path.basename(path)}: {length / 50:.0f} s")
            frames += length
    except SongError as e:
        sys.exit(f"convsong: {e}")
    voices = streams(notes, frames)

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
    print(f"music: {frames / 50:.0f} s, {len(blob)} -> {len(crunched)} "
          f"bytes")

    with open(os.path.join(build, "music_table.c"), "w") as f:
        f.write("// Generated by tools/convsong.py. Do not edit.\n")
        f.write('#include "music.h"\n\n')
        f.write("const struct music_instrument music_instruments[] = {\n")
        for name, (ctrl, ad, sr, pw) in INSTRUMENTS.items():
            f.write(f"  {{0x{ctrl:02X}, 0x{ad:02X}, 0x{sr:02X}, "
                    f"0x{pw:03X}}},  // {name}\n")
        f.write("};\n\n")
        f.write("const uint8_t music_volume[4] = {"
                + ", ".join(str(v) for v in VOLUME) + "};\n")


if __name__ == "__main__":
    main()
