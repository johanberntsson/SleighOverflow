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

**28 Oct 2026: the game plays.** All the art is the owner's (`assets/`),
and the names in `convgfx.py`, `game.c` and the asset files are the new
game's: Santa, the sack, the bear's and fox's houses (`HUT_*` inside,
`house_*` outside), the sea lion, and the gifts by colour (`GIFT_GREEN` the
bear's, `GIFT_CYLINDER` the fox's, called GOLD in the dialogue,
`GIFT_BLUE` the sea lion's). `game.c` plays `VISION.md`'s dialogue and
puzzles from the opening to the sack's "READY.":

- **The ball**: in `tree2`, the taller tree left of the sack. SPACE there
  plays Santa's lines, his cheer shakes the tree (`TREE2`, a bitmap as well
  as part of `WORLD`, drawn over its own place only while it shakes) and
  the ball bounces down (`ease_ball_y`); SPACE again picks it up.
- **The bear** gives the cookies on the first visit, and the green present
  for the remote. **The fox** eats the cookies, and leaving then takes the
  remote and the gold present. A conversation no longer ends the visit:
  SPACE leaves, as at the sack.
- **The sea lion** sulks with the blue present behind him, and trades it
  for the ball; his walk off already has it on his nose.
- **The sack** says how many are missing, from `?3 PRESENTS MISSING ERROR`
  down to `READY.`

The `AUTOPLAY` walkthrough is this game's, 88 steps from the title to the
end, and was checked headless at every scene.

**Next**: step 3, the splash, Santa dropping the sack, and the ending;
then the font and the tune, the last two placeholders (steps 5 and 6).
Also open:

- **On the machine**: the tree's shake (13.6 KB a frame from attic RAM
  for 36 frames, worth a look with `TIMING=1`) and how it looks: it shows
  the backdrop's copy two pixels out at its edges, and is drawn over the
  edge of the smaller tree in front of it.
- **The bear's cookies** stay on his floor after Santa has them, until the
  remote comes back: there is no picture of the test-pattern TV without
  them.
- **`VISION.md`'s extras**: Santa's idle lines, the presents' tags, the
  "BLUE present" label (the game says "Present"), "(cable TV)" on the
  houses' labels.
- **Santa starts sitting** (`SANTA_CHAIR`, Stranded's chair at 65, the
  `player_sitting` speech) and stands up on SPACE.

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
| `HUT_BEAR`/`HUT_BEAR_NOGREEN`, `HUT_FOX`/`HUT_FOX_NOCYLINDER` | the two house interiors, with and without their present |
| `SEALION`, `GIFT_BLUE`, `SEALION_WALK` | the sea lion with the third present, walking off |
| `TITLE`, `FACE`, `LOGO`, the credits | the BASIC splash and Santa dropping the sack (step 3) |
| `BOAT` easing in, the palette fade | the sleigh on its way (step 3) |

The flags carry over one for one: `has_green`, `has_cylinder` and
`has_blue` (Stranded's dish, bulb and battery) are one per present,
`looked_at_sack` having looked in the sack,
`rescued` all three delivered. `dlg_sack_*` become the sack's "still
missing" lines.

**The presents are three wrapped packages**, told apart by the colour of
the paper and by size. Being small and alike, they are cheap: three small
bitmaps, drawn in the sack's close-up, in the houses, and with the sea lion.
The dialogue names them by colour ("the RED one"), as Stranded's names its
parts in capitals. Who has which is settled by the art and the code: the
bear has `GIFT_GREEN`, the fox `GIFT_CYLINDER`, the sea lion `GIFT_BLUE`.
`VISION.md`'s dialogue calls them GREEN, GOLD (the cylinder, gold with a
red ribbon) and BLUE, and its flags are the game's names.

**The puzzles are `VISION.md`'s, and in.** The ball from the tree for the
sea lion; the bear's cookies for the fox; the fox's remote back to the
bear. Carried things are only flags (`has_ball`, `has_cookies`,
`has_remote`), never drawn in Santa's hands.

The sack's close-up is still 8 pictures, one for each combination. One
picture with the gifts drawn over it, as the houses do, would be fewer
bytes on the disk at the same cost a frame.

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

**Done, 28 Oct**: every picture is the owner's and has its new name. The
positions in `SCENES` and the sprite coordinates in `game.c` are still
Stranded's, and the art was drawn to them.

Budget, from MegaStranded's measurements: anything drawn every frame from
attic RAM costs 17.6 cycles a byte, so a full-screen moving picture is 1.4
frames. Static props go into the backdrop.

### 5. Text and font

The dialogue and prompts in `game.c` are `VISION.md`'s (28 Oct). Left:
the title credits; `volter.fnt` replaced or kept (it is a free font, but it came with
dos-like — check its licence before keeping it). `tools/labels.py` reads the
strings from `game.c` by pattern, so keep to the patterns it knows.

### 6. Music

A new `assets/tune.mid`, and `INSTRUMENTS` and `PARTS` in
`tools/convmidi.py` redone for it. Tune on the machine: xemu has no audio
recording. Check that the MEGA65's SIDs are at the clock `SID_CLOCK` says.

### 7. The walkthrough

**Done, 28 Oct**: `AUTOPLAY`'s script in `src/input.c` is this game's, 88
steps. Keep it in step with the game, so that every scene can be
screenshotted headless.

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
