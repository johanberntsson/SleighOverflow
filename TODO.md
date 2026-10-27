# What's next

## The end goal

**A bootable D81 of Sleigh Overflow that plays start to finish on a real
MEGA65**: an original game on MegaStranded's engine, with its own art,
setting, story, text and music, and nothing of Stranded's left on the disk.

## Where it is

**27 Oct 2026: the skeleton.** The engine, the tools and `src/game.c` were
copied from `../MegaStranded` at v1.0 (`31edcd5`), without its history or its
assets. `make` builds `build/sleigh.d81`, byte-for-byte the same as
MegaStranded's disk, with every asset borrowed from `../MegaStranded/assets`
as a placeholder (see CLAUDE.md). It boots to Stranded's title in xemu.

What the engine already does, all measured on the machine in MegaStranded
(its `TODO.md` has the numbers and the dead ends):

- 320x200 full colour, double buffered, 50 fps with half a frame to spare
- a column-DMA masked blitter, and a backdrop cache in chip RAM so that only
  the columns scrolling on come out of slow attic RAM
- art cropped and exomizer-crunched in related groups: Stranded's 77 GIFs
  were 423 KB on the disk, 22 s to load on the machine
- every string rendered offline with its outline, as a bitmap
- a MIDI arranged offline for four SIDs, twelve voices
- `BENCH`, `STATS`, `TIMING` and `AUTOPLAY` for testing on the machine and
  headless

## The plan

### 1. Setting and story

Decide the game before drawing it: the scenes, the characters, the puzzles,
the dialogue. What the engine does without changes: one scrolling outdoor
scene up to a few screens wide, full-screen interiors, a walking player with
idle / walk / one special animation, NPCs on the sprite table, dialogue as
`{ actor, line }` tables with flags, interaction prompts, a title and an
ending with a palette fade. Anything beyond that is engine work and goes
here as its own item.

### 2. The palette — decide before the first picture

**Every bitmap must share one 256-colour palette**, and while placeholders
remain, that means Stranded's. Two ways:

- **draw in a palette of the game's own from the start**, and replace the
  art a scene at a time with `make PLACEHOLDERS=` for those scenes only —
  which `convgfx.py` cannot do yet: it would need to allow one palette per
  group, and the VIC-IV only shows one at a time, so a scene's palette would
  be loaded with it. A real feature, but a bigger one.
- **draw in Stranded's palette until everything is replaced**, then swap the
  palette in one go by remapping every GIF. Simplest, and the game builds
  throughout; the cost is drawing in someone else's colours for a while.

Recommendation: the second, unless the new setting cannot live in a
tropical palette — snow wants whites and blues that Stranded may be short
of. Look at the palette before deciding.

### 3. Art, one placeholder at a time

For each of `BITMAPS` and `SCENES` in `tools/convgfx.py`: draw the new file,
give it the old name in `assets/`, build, look. Then rename it, in
`convgfx.py` and in `game.c` together, once the new name is known. The
positions in `SCENES` and the sprite coordinates in `game.c` are Stranded's
and move with the art.

Budget, from MegaStranded's measurements: anything drawn every frame from
attic RAM costs 17.6 cycles a byte, so a full-screen moving picture is 1.4
frames. Static props go into the backdrop.

### 4. Text and font

The dialogue and prompts in `game.c`, rewritten for the new story; the title
credits; `volter.fnt` replaced or kept (it is a free font, but it came with
dos-like — check its licence before keeping it). `tools/labels.py` reads the
strings from `game.c` by pattern, so keep to the patterns it knows.

### 5. Music

A new `assets/tune.mid`, and `INSTRUMENTS` and `PARTS` in
`tools/convmidi.py` redone for it. Tune on the machine: xemu has no audio
recording. Check that the MEGA65's SIDs are at the clock `SID_CLOCK` says.

### 6. The walkthrough

`AUTOPLAY`'s script in `src/input.c` is Stranded's; rewrite it as the game
takes shape, so that every scene can be screenshotted headless.

### 7. Cut the cord

`make PLACEHOLDERS=` succeeds; then drop `PLACEHOLDERS` from the Makefile,
the inherited `PORT:` comments that no longer describe anything, and
Mattias' notice from `game.c` if none of his code remains.

## Open, inherited from MegaStranded

- **Quitting.** ESC does nothing. SearchAndRescue's `quit_to_basic` puts
  back zero page, bank 1, the display and the SIDs; this engine has taken
  bank 1 and the RAM under BASIC at `$A000` as well.
- **50 frames a second.** Stranded counted frames at 60 Hz and ran at 5/6
  speed on PAL. A new game can simply be timed for 50.
- **`YCROP`.** Stranded's art was 320x240 with the picture in rows 20..219.
  New art can be drawn at 320x200 and the crop forgotten — but `game.c`'s
  coordinates are in the 240-row space, so that is a change to both.
- **A scene switch drops one frame**: filling the backdrop cache from attic
  RAM is 1.66 frames. Unseen on a cut, but there.
