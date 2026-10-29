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

**28 Oct 2026, later: on the machine.** The owner played it through on a
real MEGA65: everything works, at no speed difference from xemu that shows,
the tree's shake included (13.6 KB a frame from attic RAM for 36 frames; not
measured with `TIMING=1`). The bear's milk and cookies are now their own
sprite (`MILK_COOKIES`), gone from his floor once Santa has them.

**28 Oct 2026, evening: the splash.** Step 3.1 is in (below): the BASIC
session typed on the ROM's screen around the load, checked in xemu, not
yet on the machine. Worth a look there: the typing speed, the cursor, and
the cut to the title (xemu once caught one garbled frame there, maybe
older than the splash). It adds about 8 s before the title, so `AUTOPLAY`
screenshots need that much longer. The program is 97% of its space
(`$2001-$9FFF`); `printf` no longer fits outside `BENCH` and `STATS`.

**28 Oct 2026, night: Santa drops the sack.** Step 3.2 is in: SPACE on
the title shows `santa_drops_sack.png` for four seconds, or until SPACE
again, then cuts to the game (`drop_sack_screen()` in `game.c`). Checked
in xemu; `AUTOPLAY`'s step 2 waits 200 frames longer for it.

**28 Oct 2026, night: the end goes back to BASIC.** After the sleigh
and the fade, the screen fades up to BASIC's blue, and BASIC itself
`LIST`s `10 PRINT "MERRY CHRISTMAS"` / `20 GOTO 10`, a real program that
`RUN` runs (`src/basic.c`, `src/basic_asm.s`; step 3.4). Checked in xemu
with the whole walkthrough, and that `RUN` and `DIR` work after it; not yet
on the machine, then confirmed there. A sack bug on the way: a bitmap read across a megabyte
boundary in attic RAM wraps (CLAUDE.md, Memory), fixed in `convgfx.py`.
The 768-byte copy of the palette in `dos.c` is gone (the registers are
read back instead), which is what made room: the program has 746 bytes
free.

Worth a look on the machine: the end's fade, which now reads the palette
registers back (if it never finishes, that is why), and BASIC after it.
`DIR` shows a garbled disk name in xemu, with or without the game: the
header `tools/diskutil.rb` writes, or xemu's.

**28 Oct 2026, night: the music is this game's.** Stranded's MIDI and
`convmidi.py` are gone. The tune is a medley of three public-domain songs,
Jingle Bells, Deck the Halls and We Wish You a Merry Christmas, written
out as text in `assets/songs/` and arranged by `tools/convsong.py` (step
6). The player lost the pitch bends and slides it had for Stranded's bass
and congas. Checked in xemu that all twelve voices step through the new
streams together; heard and tuned on the machine since (below).

**28 Oct 2026, late: finished.** On the machine the
owner played it through: the sack's close-up whole again, the fox's
present taken in front of him, the end back in BASIC, and the music, at
`speed 5` (20% faster than first written), with a triangle lead (the
pulse was harsh) and the chords on SIDs of their own at volume 11 of 15.

**The font is kept, and the build is the game's own.** `volter.fnt`
("Volter (Goldfish)", cocoFabien) has no licence text; DaFont calls it
"100% Free". The owner decided to keep it: the game is free, its
distribution very limited, and it is withdrawn if anyone objects
(CLAUDE.md, Licensing, has the reasoning). It is now `assets/volter.fnt`,
copied from dos-like, and the Makefile's `PLACEHOLDERS` is gone: the
build reads only `assets/`. The disk came out byte for byte the same.

**28 Oct 2026, later still: the cursor after the end.** On the machine,
BASIC after the end had no cursor, though typing and `RUN` worked. BASIC's
raster interrupt never fired: `$D011`'s top bit reads as the raster's bit
8 and writes as the raster compare's, and `save_rom` (`src/vic4.c`) had
read it with the raster past line 255 (`$9B`), so `vic4_leave` moved the
compare to a line that never comes. Only some runs, by where the raster
was. Seen in xemu as `-dumpmem`s at different moments coming out
identical; the cursor blinks there now, not yet tried on the machine. The
same fix went into SearchAndRescue and MegaStranded.

**`BENCH=1` and `STATS=1` no longer link**: "Failed to place 1 section
fragment(s)", the program is full (the same in MegaStranded). Either needs
room made before it can be used again.

Also open, none of it needed:

- **`VISION.md`'s extras**: Santa's idle lines, the presents' tags, the
  "BLUE present" label (the game says "Present"), "(cable TV)" on the
  houses' labels.
- **Santa starts sitting** (`SANTA_SNOW`, Stranded's chair at 65, the
  `player_sitting` speech) and stands up on SPACE.

What the engine already does, all measured on the machine in MegaStranded
(its `TODO.md` has the numbers and the dead ends):

- 320x200 full colour, double buffered, 50 fps with half a frame to spare
- a column-DMA masked blitter, and a backdrop cache in chip RAM so that only
  the columns scrolling on come out of slow attic RAM
- art cropped and exomizer-crunched in related groups: Stranded's 77 GIFs
  were 423 KB on the disk, 22 s to load on the machine
- every string rendered offline with its outline, as a bitmap
- a SID player for twelve voices on four SIDs, a step a frame
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
| `SANTA_SNOW`, the game's start, sitting | Santa by the sack (or start standing and drop it) |
| `SANTA_JUMP`, rescued | Santa's cheer when the third present is back |
| `SACK_*`: the close-up, all 8 combinations of 3 parts | the sack's close-up with 0..3 presents back |
| `HUT_BEAR` + `HUT_BEAR_BORED`/`HUT_BEAR_HAPPY`, `HUT_FOX` + `HUT_FOX_HUNGRY`/`HUT_FOX_EATING` | the two house interiors, empty, and each animal before and after, drawn over its interior |
| `MILK_COOKIES` | the bear's cookies, drawn over his interior until Santa takes them |
| `SEALION`, `GIFT_BLUE`, `SEALION_WALK` | the sea lion with the third present, walking off |
| `TITLE`, `FACE`, `LOGO`, the credits | the BASIC splash and Santa dropping the sack (step 3) |
| `SLEIGH` easing in, the palette fade | the sleigh on its way (step 3) |

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

**29 Oct: the palette is fitted to the art** (`tools/mkpalette.py`, run by
`make` when a PNG changes). The sixteen hand-made ramps left gaps the
paintings fell into: the bear's crimson couch came out brown, the fox's
cream chest grey. k-means over every picture, each weighted half by its
pixels and half as one picture among all, took the mean error (weighted
RGB distance) from 8.2 to 3.1 and the worst picture's from 12.6 to 5.5.
Separate palettes for inside and outside were considered and not needed;
they would need a switch at every scene change, and Santa and the labels
in both.

### 3. Splash, intro and ending

The sequence in `VISION.md`, replacing Stranded's title and boat:

1. **The BASIC splash: done, 28 Oct.** It is the ROM's own screen, not a
   picture: the MEGA65 boots to its banner (with the real date) and
   autoboots the game under it, so `src/splash.c` carries the session on
   through the Kernal. It types `DLOAD "SANTA"` and prints `SEARCHING FOR
   SANTA` and `LOADING`, the resources really load (22 s on the machine,
   a dot after `LOADING` every 16 KB read so the screen is seen to be
   alive: not BASIC's, but close enough), then `READY.`, `RUN` typed, and
  `ERROR 65 IN 2026: SLEIGH OVERFLOW` for
   three seconds. The ROM's font and colours, so no second font and nothing
   reserved in the palette. The typing cursor is a reversed space written
   into screen RAM at the Kernal's cursor (`PLOT`, `src/splash_asm.s`):
   printed through the editor it shows as control codes inside the quotes.
   The border no longer counts the files loaded; it still goes red on a
   load error. `printf` would not fit in the program, so it prints with
   `_Stub_write`.
2. **Santa dropping the sack: done, 28 Oct.** `SANTA_DROPS_SACK`, a
   full-screen picture after the title, four seconds or SPACE, then a cut
   to the game. No easing.
3. **The start**: Santa by the sack; looking in it is `dlg_sack_first`.
4. **The end: done, 28 Oct.** Back at the sack with all three, a last
   line, the sleigh and the palette fade (Stranded's sequence, the owner's
   `sleigh.png`), then **a real return to BASIC**, as SearchAndRescue
   quits: `__low_level_init` keeps BASIC's `$0002-$1FFF` and bank 1 in
   attic RAM before the game touches them, and `basic_return` puts them
   back and returns through the `SYS`. On the way it writes the two-line
   program over the `SYS` line at `$2001`, sets BASIC's end of program
   (`$82`) and its text pointer (`$3B`, to the program's last zero, so the
   interpreter finds the end and prints `READY.`), and puts CLR `LIST`
   RETURN in the keyboard queue (`$02B0`, count `$D0`). The two BASIC
   addresses were found in xemu by loading test programs with `-prg` and
   reading `-dumpmem`. BASIC's screen is emptied, in the copy too, and
   fades up from black.

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

The dialogue and prompts in `game.c` are `VISION.md`'s (28 Oct).
**`volter.fnt` kept, 28 Oct**, by the owner's decision, as
`assets/volter.fnt` (CLAUDE.md, Licensing). Left, if wanted: the title
credits. `tools/labels.py` reads the strings from `game.c` by pattern, so
keep to the patterns it knows.

### 6. Music

**Done, 28 Oct**: the songs in `assets/songs/`, one text file each, and
`tools/convsong.py`, which checks every bar, arranges them the same way
(melody, oom-pah bass, off-beat chords, sleigh bells) and writes the
player's streams. A new song is a new file and a name in `SONGS`.

Still to do on the machine, by ear: the instruments' envelopes and the
balance between the parts (`INSTRUMENTS`), the tempo (`speed`), and that
the MEGA65's SIDs are at the clock `SID_CLOCK` says.

### 7. The walkthrough

**Done, 28 Oct**: `AUTOPLAY`'s script in `src/input.c` is this game's, 89
steps. Keep it in step with the game, so that every scene can be
screenshotted headless.

### 8. Cut the cord

**`PLACEHOLDERS` dropped from the Makefile, 28 Oct**: the build reads only
`assets/`. Left, none of it urgent: the inherited `PORT:` comments that no
longer describe anything, and Mattias' notice from `game.c` if none of his
code remains (it still does).

## Open, inherited from MegaStranded

- **Quitting.** ESC does nothing (`keystate` in `dos.c` never reports
  it); only winning goes back to BASIC (`basic_ending`, from
  SearchAndRescue's `quit_to_basic`). ESC could do the same without the
  program. The RAM under BASIC at `$A000` that the
  engine uses is bank 0's program space to BASIC, past the end of the
  program, so it needs nothing back.
- **50 frames a second.** Stranded counted frames at 60 Hz and ran at 5/6
  speed on PAL. A new game can simply be timed for 50.
- **`YCROP`.** Stranded's art was 320x240 with the picture in rows 20..219.
  New art can be drawn at 320x200 and the crop forgotten — but `game.c`'s
  coordinates are in the 240-row space, so that is a change to both.
- **A scene switch drops one frame**: filling the backdrop cache from attic
  RAM is 1.66 frames. Unseen on a cut, but there.
