# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Sleigh Overflow**, an original MEGA65 game in C for the Calypsi toolchain,
with hot paths in 45GS02 assembly. It is built on the engine of
`../MegaStranded`, a finished port of Mattias Gustavsson's *Stranded* (the
headline sample of his [dos-like](https://github.com/mattiasgustavsson/dos-like)
framework). What carries over is the dos-like side: a point-and-walk game over
a 64-entry sprite table, a dialogue interpreter driven by `#SetFlag` /
`#ClearFlag`, labels rendered offline, a SID tune from a MIDI. **The art, the
setting, the story and the music are to be new.**

**`VISION.md` is the owner's description of the game** (Santa, three
dropped presents, two houses and a sea lion, a BASIC-screen splash).
**`TODO.md` is the plan and the running log — read it first.**

Where it is (27 Oct 2026): the engine was copied from MegaStranded v1.0
(`31edcd5`) without its history. `make` builds a disk that is still
byte-for-byte Stranded, because `src/game.c` is still Stranded's game and
every asset is borrowed from MegaStranded as a placeholder (below). The plan
for the new game is written (`TODO.md` steps 1-3), and the palette remap is
in `convgfx.py`; the owner is drawing the first art. The work from here is
replacing the placeholders, one file at a time.

**The owner does all `git add` and `git commit` by hand.** Leave changes in
the working tree and say what changed.

## Placeholders: Stranded's assets, borrowed, never copied

`assets/` holds this game's own art, font and tune, and only those. The
Makefile's `PLACEHOLDERS` (default `../MegaStranded/assets`) is searched after
it: `tools/convgfx.py` takes every picture (`name.png`, else `name.gif`) and
`volter.fnt` from the first directory that has it, and the tune is `assets/tune.mid` when there is one
and `stranded.mid` from there until then. So a new picture replaces a
placeholder by having its name, and the game keeps building in between.

**Never copy a file from `../MegaStranded/assets` into this repository.** Not
having Mattias' art in this game's history is why it is a new repository.
`make PLACEHOLDERS=` builds without them, which is how to see what is still
missing; when that succeeds, the Makefile's placeholder lines can go.

Mixing old and new art has one catch: **there is one 256-colour palette**,
`assets/palette.gif`'s. New art is painted in Krita and exported as RGBA
PNG: each colour goes to the palette's nearest, alpha below 128 is
transparent. A GIF in `assets/` must be drawn in exactly the palette
(`convgfx.py` refuses one with its own); a placeholder drawn in another is
remapped to it by nearest colour and looks off until replaced. Without a
`palette.gif`, Stranded's palette is the game's and nothing is remapped.

## Layout

| | |
|---|---|
| `assets/` | this game's own GIFs, font and `tune.mid` (see above) |
| `src/game.c` | the game; still Stranded's `stranded.c` with MegaStranded's `PORT` changes |
| `src/main.c` | loads everything, then runs the game for ever |
| `src/dos.*` | the sixteen dos-like calls the game makes, by their dos-like names |
| `src/input.*` | keyboard and joystick port 2, and `AUTOPLAY`'s scripted walkthrough |
| `src/vic4.*` | 320x200 full-colour display, double buffered |
| `src/gfx.*`, `src/gfx_asm.s` | the bitmap table, the column-DMA blitter, the backdrop cache in chip RAM |
| `src/music.*` | the tune's player, a step a frame from `waitvbl` |
| `src/loader.*`, `src/exo*` | GFXn.BIN and MUSIC.BIN off the disk, decrunched into attic RAM |
| `src/dma.*` | plain DMA copy and fill |
| `src/timer.*` | CIA2's timers as a 32-bit clock, for the measurements |
| `src/stats.c`, `src/bench.c` | `STATS` and `BENCH`: frame times and DMA timings, printed on the machine |
| `tools/convgfx.py` | GIFs and labels to crunched GFXn.BIN, plus `build/gfx_table.c` and `build/gfx_ids.h` |
| `tools/labels.py` | every string `game.c` draws, rendered offline in `volter.fnt` as bitmaps |
| `tools/convtables.py` | the easing curves as integer tables, `build/tables.c` |
| `tools/convmidi.py` | the MIDI arranged for twelve SID voices: `build/music.bin`, `build/music_table.c` |
| `tools/diskutil.rb` | D81 builder, from SearchAndRescue |

The `PORT:` comments in `game.c` and the word "Stranded" in the engine's
comments are inherited; they say why the code is as it is and can go as the
code they describe is rewritten. `../MegaStranded/src/stranded.c` is the
original, for reference, and `~/dos-like/source/dos.h` the framework the
engine replaces.

`game.c` ends with Mattias' MIT / Unlicense notice for `stranded.c`. Keep it
there while any of his code remains in the file.

## The reference projects

- `../MegaStranded`: where every part of this engine was built and measured.
  Its `TODO.md` is the full story of why the renderer is the way it is.
- `../SearchAndRescue`: a finished MEGA65 game by the same author, same
  toolchain. **Its `CLAUDE.md` is 94 KB of hard-won MEGA65 detail — consult
  it before deriving anything about the VIC-IV, the DMA controller, attic
  RAM, or the C65 ROM's interrupt dispatcher.** It also has `quit_to_basic`,
  which this game does not yet.

A fix to the engine here may be worth carrying back to MegaStranded by hand;
the two do not share code.

## Build flags

Each is in the Makefile's config stamp, so changing one rebuilds everything.
All default to 0, which is the disk to hand out.

| | |
|---|---|
| `AUTOPLAY=n` | play the first n steps of a scripted walkthrough, then sit there (51 is the whole of Stranded) |
| `TIMING=1` | the border shows each frame's draw time: white while drawing, red if it ran past a frame |
| `STATS=1` | F1 prints a histogram of every frame's draw time on the ROM's screen |
| `BENCH=1` | before the game, time the DMA every way the blitter could use it and print a table |
| `PLACEHOLDERS=dir` | where missing assets come from; empty for none |

`TIMING`, `STATS` and `BENCH` exist for the real machine, which has no memory
dump; the owner tests there and reports back with a photo of the screen.

## Running it headless

```sh
xemu-xmega65 -skipconfigfile -besure -headless -sleepless \
    -8 build/sleigh.d81 -screenshot out.png -dumpmem mem.bin &
P=$!; sleep 40; kill -TERM $P; while kill -0 $P 2>/dev/null; do sleep 1; done
```

Loading takes about 30 seconds of that. SIGTERM, not SIGINT, and kill by
PID: `pkill -f xemu-xmega65` matches the shell that is running it. The dump
is chip RAM only; the framebuffers are at `$40000` and `$50000` in
column-strip layout (`src/vic4.h`), and de-swizzling one and comparing it
with the same frame composited in Python from `tools/convgfx.py` is the
fastest way to tell a blitter bug from a display one. Globals are found by
name in `build/sleigh.lst` — match `^name = ` exactly, or `sprites = `
finds `drawsprites = ` first:

- `stat_*` (`src/stats.c`): every frame's draw time in CIA2 ticks, about
  1 MHz, 19968 to a frame in xemu and 19650 on the machine.
- `music_pos`, `music_wait`, `music_start_at` (`src/music.c`): where each of
  the twelve voices is in its stream.

**xemu cannot show three things**: the speed of attic RAM (it runs it at chip
speed), the border mid-frame (its screenshots have one border colour), and
sound (it has no recording). Those need the real machine.

## Memory, and why

**DMA out of attic RAM is 17.6 cycles a byte on the machine, out of chip RAM
3.0** (measured, `../MegaStranded/TODO.md`). xemu runs both at chip speed,
so it cannot show a renderer that is too slow on the hardware. Anything read
every frame belongs in chip RAM, and chip RAM is full:

| | |
|---|---|
| `$02001-$09FFF` | program |
| `$0A000`, `$0A800` | the two screen tables (RAM under the BASIC ROM) |
| `$0B000-$0B257` | backdrop cache, slots 317-319 |
| `$10000-$1F7FF` | backdrop cache, slots 0-316; colour RAM is aliased at `$1F800` |
| `$20000-$3FFFF` | C65 ROM |
| `$40000`, `$50000` | framebuffers A and B, 64000 bytes each |
| `$8000000-` | every bitmap, decrunched (`gfx_table.c` has the addresses) |
| `$8500000` | the tune's twelve streams, about 29 KB |
| `$8600000` | staging for the crunched stream being loaded |

## How a frame fits

The game redraws **every visible sprite, every frame, back to front**,
including a backdrop wider than the screen. On the MEGA65 that works, at
50 fps with half a frame to spare, because of three things: the static props
are baked into the backdrop (`SCENES` in `tools/convgfx.py`); the backdrop is
drawn from a cache of the visible columns in chip RAM, with only the columns
scrolling on read from attic RAM; and `drawsprites` starts at the topmost
picture that is opaque and covers the screen. A Stranded frame is 31-47% of
the time on the machine. Something new drawn every frame from attic RAM
costs 17.6 cycles a byte; a 320x200 picture of it would be 1.4 frames. **New
art has to respect that**: big moving things are expensive, static things
belong in the backdrop.

## The art the pipeline expects

- GIFs, palette mode, **in `assets/palette.gif`'s 256 colours exactly**.
  Colour 255 is transparent; 254 is the labels' outline.
- The screen is 320x200. Files may be 320x240 like Stranded's, in which case
  rows 20..219 are shown (`YCROP`); anything taller than 200 is cropped to
  that window, and every bitmap to its non-transparent box.
- Bitmaps are listed in `BITMAPS` in `tools/convgfx.py` (the generated
  `enum bitmap_t`); a stem with `%04d` is an animation whose cels all share
  one box. `SCENES` composites layers into one opaque backdrop, `GROUPS` puts
  bitmaps into crunched files — similar pictures in one group crunch against
  each other.
- Labels are not drawn on the machine: every string `game.c` draws is found
  by `tools/labels.py`'s patterns (dialogue `{ actor, line }`,
  `set_interact`, `label(...)`) and rendered offline. **A string drawn any
  other way has no bitmap and is drawn as nothing.**

## Changing the game

**Test with `make AUTOPLAY=n`**, which stops the game at step n of a
scripted playthrough (`src/input.c`) so a headless screenshot catches it
there. The script is Stranded's walkthrough and has to be rewritten along
with the game. A step that holds an arrow walks 2 pixels a frame and must
not start inside a `delay( 30 )`: wait 50 after a line of dialogue ends one.

## Changing the music

`tools/convmidi.py` is the arrangement: `INSTRUMENTS` (waveform, ADSR, pulse
width, pitch slide per frame) and `PARTS` (which voices, how many notes at
once, priority) are the two tables to tune, and it prints how many notes
each part loses. Both are keyed to Stranded's MIDI and will need redoing for
a new tune. The stream format and the reasons for the voice layout are at
the top of the file. The player, `src/music.c`, does only what the streams
say; it should not need to change for an arrangement.

## Toolchain

Installed and on `PATH`: `cc6502`, `as6502`, `ln6502` (Calypsi, use
`--target=mega65`), `xemu-xmega65`, `exomizer`, Ruby for `diskutil.rb`, and
Python 3 with Pillow and NumPy for asset conversion.

`make` builds `build/sleigh.d81` and `make run` launches it in
`xemu-xmega65`. There is no `make prg`: the game cannot run without its
resources. Add any new build flag to `CONFIG_STAMP`.

**Calypsi 5.18 miscompiles some 32-bit arithmetic.** A `uint32_t` product
came back with its low bytes swapped; SearchAndRescue's CLAUDE.md
(Performance) lists more of the family. Do products with `mul16` in
`src/gfx.c` (the 45GS02 multiplier), and when a number is wrong, read the
listing before suspecting the hardware.

## Licensing

This game is MIT (`LICENSE`). The engine is the author's own work from
MegaStranded, also MIT; `game.c` started as Mattias Gustavsson's
`stranded.c`, MIT / Unlicense, whose notice it keeps. The placeholder assets
come with dos-like, and the permission Mattias gave (dos-like issue #49) was
for the port: **a released disk must contain none of them.**
