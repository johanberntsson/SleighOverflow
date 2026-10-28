# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Sleigh Overflow**, an original MEGA65 game in C for the Calypsi toolchain,
with hot paths in 45GS02 assembly. It is built on the engine of
`../MegaStranded`, a finished port of Mattias Gustavsson's *Stranded* (the
headline sample of his [dos-like](https://github.com/mattiasgustavsson/dos-like)
framework). What carries over is the dos-like side: a point-and-walk game over
a 64-entry sprite table, a dialogue interpreter driven by `#SetFlag` /
`#ClearFlag`, labels rendered offline, a SID tune player. **The art, the
setting, the story and the music are to be new.**

**`VISION.md` is the owner's description of the game** (Santa, three
dropped presents, two houses and a sea lion, a BASIC-screen splash).
**`TODO.md` is the plan and the running log — read it first.**

Where it is (28 Oct 2026, finished): the engine was copied from
MegaStranded v1.0 (`31edcd5`) without its history, and the game is now
Sleigh Overflow's. All the art is the owner's, and `src/game.c` plays `VISION.md` start to finish:
the opening, the ball in the tree, the sack, the bear, the fox and the sea
lion, and the sack's "READY." It opens with the BASIC splash
(`src/splash.c`), typed on the ROM's own screen while the game loads.
After the title, Santa drops the sack (`drop_sack_screen()`), four
seconds or SPACE. The end returns to BASIC for real, through the `SYS`
that started the game, where BASIC `LIST`s the two-liner it leaves there
(`src/basic.c`). The music is a medley of public-domain Christmas songs,
written out by hand in `assets/songs/` (`tools/convsong.py`). All of it
is played through and confirmed on the machine, and the build takes
nothing from MegaStranded any more: the font, `volter.fnt`, is the one
file the game shares with Stranded, kept by the owner's decision
(Licensing below). Still Stranded's in the code: the title screen's
sequence and the sleigh's (Stranded's boat's) easing.

**The owner does all `git add` and `git commit` by hand.** Leave changes in
the working tree and say what changed.

## Assets: this game's own, and Stranded's never

`assets/` holds everything the build reads: the owner's pictures, the
songs (`songs/`), and `volter.fnt`, the font. Until 28 Oct the Makefile
borrowed Stranded's art, font and tune from `../MegaStranded/assets` as
placeholders; every one of them has been replaced or (the font) taken in,
and the Makefile no longer looks there.

**Never copy a file from `../MegaStranded/assets` into this repository.** Not
having Mattias' art in this game's history is why it is a new repository.
`volter.fnt` is the one exception, copied from dos-like itself
(`~/dos-like/files/stranded/`) on the owner's decision; see Licensing.

**There is one 256-colour palette**, `assets/palette.gif`'s. New art is
painted in Krita and exported as RGBA PNG: each colour goes to the
palette's nearest, alpha below 128 is transparent. A GIF in `assets/` must
be drawn in exactly the palette (`convgfx.py` refuses one with its own).
`convgfx.py` can still take extra directories after `assets/`, and remaps
a picture from one drawn in another palette; nothing uses that now.

## Layout

| | |
|---|---|
| `assets/` | this game's own pictures and font (see above), and `songs/`, the music |
| `src/game.c` | the game: Stranded's `stranded.c` and MegaStranded's `PORT` changes, with Sleigh Overflow's scenes and dialogue |
| `src/main.c` | loads everything, runs the game until it is won, then the end |
| `src/splash.*`, `src/splash_asm.s` | the BASIC splash, typed on the ROM's own screen around the load |
| `src/basic.*`, `src/basic_asm.s` | the end: BASIC's memory kept at startup and put back, with `10 PRINT "MERRY CHRISTMAS"` in it |
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
| `tools/convsong.py` | the songs arranged for twelve SID voices: `build/music.bin`, `build/music_table.c` |
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
  RAM, or the C65 ROM's interrupt dispatcher.** Its `quit_to_basic` is
  where this game's return to BASIC (`src/basic*`) comes from.

A fix to the engine here may be worth carrying back to MegaStranded by hand;
the two do not share code.

## Build flags

Each is in the Makefile's config stamp, so changing one rebuilds everything.
All default to 0, which is the disk to hand out.

| | |
|---|---|
| `AUTOPLAY=n` | play the first n steps of a scripted walkthrough, then sit there (89 is the whole walkthrough) |
| `TIMING=1` | the border shows each frame's draw time: white while drawing, red if it ran past a frame |
| `STATS=1` | F1 prints a histogram of every frame's draw time on the ROM's screen |
| `BENCH=1` | before the game, time the DMA every way the blitter could use it and print a table |

`TIMING`, `STATS` and `BENCH` exist for the real machine, which has no memory
dump; the owner tests there and reports back with a photo of the screen.

## Running it headless

```sh
xemu-xmega65 -skipconfigfile -besure -headless -sleepless \
    -8 build/sleigh.d81 -screenshot out.png -dumpmem mem.bin &
P=$!; sleep 40; kill -TERM $P; while kill -0 $P 2>/dev/null; do sleep 1; done
```

Loading takes about 30 seconds of that, the splash included (a few
seconds of typing and a pause either side of the load). SIGTERM, not SIGINT, and kill by
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
| `$8500000` | the tune's twelve streams, about 10 KB |
| `$8600000` | staging for the crunched stream being loaded |
| `$8700000` | BASIC's bank 1, then its `$0002-$1FFF`, kept at startup for the return (`src/basic_asm.s`) |

**A DMA source wraps within its megabyte** (`src/dma.c` gives the
megabyte its own byte), so `convgfx.py` pads the streams so that no bitmap
straddles `$8100000`. A picture read across one gets the rest from
`$8000000`, which is `WORLD`.

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
- A PNG smaller than 320x200 is placed into a 320x200 frame at its
  `CEL_ORIGIN` in `tools/convgfx.py` (Santa's and the sea lion's walks, the
  ball). The owner draws most sprites as whole 320x200 frames instead,
  painted where they belong on screen.
- **Where a sprite lands.** Santa is drawn at the same place on screen
  always (x 143-189); an object at sprite x X is at Santa when `xpos` is X,
  which is what `is_near` and `set_interact` assume. A 320x200-frame
  picture at sprite y `-10 + o` (20) appears as painted on screen when
  Santa stands at it; an interior at (0, 0) is drawn as painted, so a
  frame-painted prop over one, like the gifts in the houses, goes at
  (0, `-10 + o`).
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
there. The script is this game's walkthrough, every step commented; a
change to a dialogue's length or a new scene shifts every step after it,
and the comments are numbered so that `AUTOPLAY=n` is the step on that
line. A step that holds an arrow walks 2 pixels a frame and must not start
inside a `delay( 30 )`: wait 50 after a line of dialogue ends one. A
`#SetFlag` that plays an animation (`ball_fell`, `sealion_running`) blocks
for its length, so the press after it waits longer (120, 200).

Adding up the script's frames (`wait + hold` per step, 50 a second) plus
about 30 seconds of loading is how long to let xemu run before its
screenshot. Several can run at once, one disk each; write `P=$!` and the
kill in the same shell function as the launch, or the `&` takes the
variables with it.

## Changing the music

**A song is a text file in `assets/songs/`**: a melody in notes (`E5:4`,
C4 middle C, `:4` a quarter, `.` dotted, `r` a rest) and a line of chords
(`C | F C | G7 |`), in bars, which `tools/convsong.py` checks add up. The
format is at the top of that file. `SONGS` in the Makefile is the medley,
in order; it loops. The songs must be public domain, and the arrangement
the game's own: a MIDI file or a modern songbook is someone's arrangement,
and can be copyright where the tune is not.

`convsong.py` is also the arrangement, the same for every song: the melody
and an oom-pah bass, each doubled on a left and a right SID, the chord as
off-beat stabs, sleigh bells where a song asks (`bells`). `INSTRUMENTS`
(waveform, ADSR, pulse width) and `VOLUME` (each SID's, which is how the
chords, on two SIDs of their own, are turned down) are the tables to tune
by ear, on the machine:
xemu has no recording. The player, `src/music.c`, does only what the
streams say: note on, note off, wait, loop.

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
`stranded.c`, MIT / Unlicense, whose notice it keeps. Stranded's art and
tune come with dos-like, and the permission Mattias gave (dos-like issue
#49) was for the port: **a released disk must contain none of them**, and
none is in this repository.

**The font is kept knowingly** (28 Oct 2026). `volter.fnt` is "Volter
(Goldfish)", a pixel font credited to cocoFabien. DaFont lists it as
"100% Free", but it comes with no licence text, sites disagree about its
terms, and its origin (the author's site was habbohotel.fr; Habbo Hotel
used a font called Volter) is unclear. The owner's decision: keep it.
The game is free, its distribution very limited, the disk holds only
labels rendered from the font and not the font file, and should anyone
object, the game is withdrawn. Replacing it would only change
`tools/labels.py`'s input; every label is rendered at build time.
