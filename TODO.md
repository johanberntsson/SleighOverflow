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

**27 Oct 2026, later: the plan.** `VISION.md` (the owner's) is the game,
and steps 1-3 below turn it into work: how Stranded's parts map onto
Sleigh Overflow's, the presents (three wrapped packages), the palette and
the splash. `convgfx.py` now takes the game's palette from
`assets/palette.gif` and remaps the placeholders to it (step 2); with no
`palette.gif` yet, the disk is still byte-for-byte Stranded.

**Next: the owner is drawing.** When the first art arrives in `assets/`,
the likely work is a `palette.gif` (any GIF of all 256 colours will do:
its pixels are ignored, so a copy of the first finished picture is
enough), checking that the build accepts the new files and what the
remapped placeholders look like next to them, and moving `SCENES`,
`BITMAPS` and `game.c`'s coordinates to fit the new art (step 4). The
puzzles in step 1 are still open.

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

### 1. The game

`VISION.md` is the game: Santa has dropped three presents from the sleigh
and gets them back from the people in two houses and a sea lion on the beach.
Stranded has the same shape, so its `game.c` is the skeleton and each part
of it becomes one of this game's:

| Stranded | Sleigh Overflow |
|---|---|
| `WORLD`: a 1480-wide beach, the radio, two huts, palms, the chair | a snowfield before a forest: the sack, two houses, the sea lion on the beach at the far end |
| `SANTA_IDLE`, `SANTA_LEFT`, `SANTA_RIGHT` | Santa standing and walking |
| `SANTA_CHAIR`, the game's start, sitting | Santa by the sack (or start standing and drop it) |
| `SANTA_JUMP`, rescued | Santa's cheer when the third present is back |
| `SACK_*`: the close-up, all 8 combinations of 3 parts | the sack's close-up with 0..3 presents back |
| `HUT_TV`/`HUT_TV_NODISH`, `HUT_BOOK`/`HUT_BOOK_NOBULB` | the two house interiors, with and without their present |
| `SEALION`, `SEALION_BATTERY`, `SEALION_WALK` | the sea lion with the third present, walking off |
| `TITLE`, `FACE`, `LOGO`, the credits | the BASIC splash and Santa dropping the sack (step 3) |
| `BOAT` easing in, the palette fade | the sleigh on its way (step 3) |

The flags carry over one for one: `has_dish`, `has_bulb` and `has_battery`
become one per present, `looked_at_sack` having looked in the sack,
`rescued` all three delivered. `dlg_sack_*` become the sack's "still
missing" lines.

**The presents are three wrapped packages**, told apart by the colour of
the paper and by size. Being small and alike, they are cheap: three small
bitmaps, drawn in the sack's close-up, in the houses, and with the sea lion.
The dialogue names them by colour ("the RED one"), as Stranded's names its
parts in capitals.

**Still to decide: the puzzles.** `VISION.md` says "some mini problems".
Stranded's are what the code already does: each hut's owner is talked round and the part is taken on leaving,
and the sea lion walks off and leaves it behind. Anything more, such as an
item from one house wanted in the other, is a little new logic in `game.c`
and a new bitmap for every item shown held or given — the cost is in the
drawing. Write the puzzles and the dialogue down here before the first
picture: the art depends on what each scene must show.

With the presents as separate bitmaps, the sack's close-up is one picture
of the sack with the presents drawn on top as each comes back, instead of
Stranded's 8 combinations: fewer bytes on the disk, the same cost a frame.

**Santa is the one big thing drawn every frame.** Stranded's guy is about
40x92, 3.7 KB a cel, out of attic RAM at 17.6 cycles a byte. Santa twice
that size costs about as much again as the guy does; check with `STATS=1`
on the machine once the first walk cycle is in.

### 2. The palette: the game's own, from the start

Stranded's palette has four near-whites, all warm (cream to beige), and one
pale grey-blue (107,149,187). Snow wants a run of cool whites, blue-greys
and shadow blues, so drawing in Stranded's colours until the end is not an
option.

**Done in `convgfx.py`: the palette is `assets/palette.gif`'s**, a palette
image of all 256 colours whose pixels do not matter. Every GIF in `assets/`
must be drawn in exactly it; a placeholder drawn in another is remapped to
it by nearest colour, so the game keeps building and the placeholders look
off. Without a `palette.gif` the build is unchanged, byte for byte.

Still to do: **draw the palette**, the first new asset. Keep 255
transparent and 254 the labels' outline, reserve the splash's two colours
(step 3), and pick the label colours (`COLOR_*` in `game.c`) in it again.

Per-scene palettes stay unnecessary: the whole game is one outdoor scene in
one season.

### 3. Splash, intro and ending

The sequence in `VISION.md`, replacing Stranded's title and boat:

1. **The BASIC splash**: the boot banner, `LOAD "SANTA",8,1`, `SEARCHING
   FOR SANTA`, `RUN`, `ERROR 65: SLEIGH OVERFLOW IN 2026`, typed out. This
   is new engine work. Text is only ever offline bitmaps in `volter.fnt`,
   so it needs a font in the MEGA65's own style for `tools/labels.py` (a
   second font, or the splash's lines drawn as one picture), and a way to
   show a line appearing a character at a time: a bitmap per line drawn
   clipped to a growing width is the least code, if the blitter can clip
   on the right; a fixed-width glyph blitter is the general one. A
   flashing cursor is one more small sprite. Black-on-blue is two colours
   of the palette, which step 2 should reserve.
2. **Santa dropping the sack**: a full-screen picture, like `FACE` and
   `LOGO`, with or without easing, then a cut to the game.
3. **The start**: Santa by the sack; looking in it is `dlg_sack_first`.
4. **The end**: back at the sack with all three, a last line, then the
   picture of Santa on his way and the palette fade, as `BOAT` is now.

`title_screen()` and the `rescued && fadeout` branch of `game_main()` are
where this goes; the easing tables are `tools/convtables.py`'s.

### 4. Art, one placeholder at a time

For each of `BITMAPS` and `SCENES` in `tools/convgfx.py`: draw the new file,
give it the old name in `assets/`, build, look. Then rename it, in
`convgfx.py` and in `game.c` together, once the new name is known. The
positions in `SCENES` and the sprite coordinates in `game.c` are Stranded's
and move with the art.

Budget, from MegaStranded's measurements: anything drawn every frame from
attic RAM costs 17.6 cycles a byte, so a full-screen moving picture is 1.4
frames. Static props go into the backdrop.

### 5. Text and font

The dialogue and prompts in `game.c`, rewritten for the new story; the title
credits; `volter.fnt` replaced or kept (it is a free font, but it came with
dos-like — check its licence before keeping it). `tools/labels.py` reads the
strings from `game.c` by pattern, so keep to the patterns it knows.

### 6. Music

A new `assets/tune.mid`, and `INSTRUMENTS` and `PARTS` in
`tools/convmidi.py` redone for it. Tune on the machine: xemu has no audio
recording. Check that the MEGA65's SIDs are at the clock `SID_CLOCK` says.

### 7. The walkthrough

`AUTOPLAY`'s script in `src/input.c` is Stranded's; rewrite it as the game
takes shape, so that every scene can be screenshotted headless.

### 8. Cut the cord

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
