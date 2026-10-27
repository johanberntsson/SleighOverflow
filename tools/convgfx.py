#!/usr/bin/env python3
"""The game's pictures in, crunched resource files and a C manifest out.

    python3 tools/convgfx.py <build dir> <game.c> <assets dir>...

writes <build>/GFXn.BIN, one per group below, and <build>/gfx_table.c with
the bitmap table, the file list, the label table and the palette. The labels
are every string game.c draws, rendered offline: see tools/labels.py.

**Each file is taken from the first assets directory that has it**, as
name.png or, failing that, name.gif; a later directory's PNGs are ignored. The
Makefile passes assets/ and then MegaStranded's, so Stranded's art stands in
for whatever has not been drawn yet. A new picture replaces the old one by
having its name.

**There is one palette**, the VIC-IV's, and it is assets/palette.gif's: a
palette image whose pixels do not matter. Every GIF in the first assets
directory must be drawn in exactly that palette. A PNG is painted in RGB,
in Krita say, and each of its colours becomes the palette's nearest; alpha
below 128 is transparent, colour 255. So a PNG needs a palette.gif.
A placeholder from any
later directory that was drawn in another is remapped to it, each colour to
the nearest, so the game keeps building while the art is replaced. Without
a palette.gif, the first GIF read sets the palette and every other must
share it, which is how the game built before it had colours of its own.

**Every bitmap is cropped, and nothing is scaled.** The art is fundamentally
320x200: the twelve 320x240 interiors are 200 rows of picture between two
20-row black bars, and everything the game places on the beach lies between
rows 20 and 220 of its 240. So the MEGA65 shows rows 20..219 of the original
screen (YCROP in src/gfx.h), and an image taller than 200 -- which the game
only ever draws at y = 0 -- loses the rows outside that window here. Then
every bitmap is cut to the bounding box of its non-transparent pixels, and
remembers where in the original file the box was, so that drawing it at the
original's coordinates puts it exactly where the original did.

**Animations share one box**: the union over all their cels, so a cel is
just an offset of w*h from the first and the game's `cel % cels` needs no
table.

**Pixels are stored column-major**, a column of h bytes after another. The
display is column strips of characters (src/vic4.h), in which a vertical run
of pixels is one address stepping by 8 however many character rows it
crosses -- so a sprite column is one DMA job, and a sprite is w of them.

**A group is one exomizer stream**, decrunched to consecutive attic RAM.
That matters most for the eight sack_* scenes: crunched one after another
they reference the scene before, 64000 bytes back and inside the cruncher's
65535 window, and take 75 KB instead of 292. The hut pairs do the same.
"""

import os
import shutil
import struct
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image

import labels

TRANSPARENT = 255
SCREEN_H = 200
YCROP = 20        # rows of the original's 240 above the MEGA65's screen
ATTIC_GFX = 0x8000000
PALETTE_GIF = "palette.gif"

# Animations whose PNG cels are drawn as dos-like's sources were: a small
# frame, not the whole 320x200 screen, and this is where on the screen the
# frame's top-left goes. Santa's cels (Stranded's guy's) are 64x100 at
# (132, 60) of the GIFs the game was built from, the sea lion's (Stranded's
# penguin's) 64x100 at (132, 90). A cel already 320x200 is taken as it is.
CEL_ORIGIN = {
    "santa_jump_%04d": (132, 60),
    "santa_walk_left_%04d": (132, 60),
    "santa_walk_right_%04d": (132, 60),
    "sealion_walk_%04d": (132, 90),
}

# The order of stranded.c's enum bitmap_t, and each one's file stem. A stem
# with %04d is an animation; its cels run from 0001 until a file is missing.
#
# Not quite that enum: see SCENES for the bitmaps that were folded into WORLD
# and TITLE.
BITMAPS = [
    ("WORLD", None),
    ("TITLE", None),
    ("FACE", "face"),
    ("LOGO", "logo"),
    ("BOAT", "boat"),
    ("SANTA_CHAIR", "santachair"),
    ("SANTA_IDLE", "santa_idle"),
    ("SANTA_JUMP", "santa_jump_%04d"),
    ("SANTA_LEFT", "santa_walk_left_%04d"),
    ("SANTA_RIGHT", "santa_walk_right_%04d"),
    ("HUT_BOOK", "hut_book"),
    ("HUT_BOOK_NOBULB", "hut_book_nobulb"),
    ("HUT_TV", "hut_tv"),
    ("HUT_TV_NODISH", "hut_tv_nodish"),
    ("SEALION", "sealion"),
    ("SEALION_BATTERY", "penguin_battery"),  # the placeholder's name
    ("SEALION_WALK", "sealion_walk_%04d"),
    ("SACK_BATTERY", "sack_battery"),
    ("SACK_BATTERY_DISH", "sack_battery_dish"),
    ("SACK_BULB", "sack_bulb"),
    ("SACK_BULB_BATTERY", "sack_bulb_battery"),
    ("SACK_BULB_DISH", "sack_bulb_dish"),
    ("SACK_DISH", "sack_dish"),
    ("SACK_FULL", "sack_full"),
    ("SACK_NONE", "sack_none"),
]

# **The beach is one picture.** The original draws the background, the
# treeline, the radio, both huts, both trees and the deckchair as separate
# sprites every frame, all of them scrolling with the camera and none of them
# ever hidden, moved or drawn over by anything earlier in the sprite table
# than the sea lion. So they composite to the same pixels every frame, and
# here they are composited once: WORLD is that picture, 1480 wide and opaque.
#
# It is a speed decision before it is a disk one. DMA out of attic RAM costs
# 16 cycles a byte on a real MEGA65 (measured in ../SearchAndRescue), so the
# original's layers -- background, treeline and four props, 150 KB a frame --
# would be four frames' worth of DMA each frame. One opaque window of WORLD
# is 64000 bytes.
#
# TITLE is the same for the title screen, which shows the background and the
# treeline alone at x = -1050, with none of the props.
#
# Positions are stranded.c's: sprite(x, y) with y including its `o = 30`, and
# the layers in the order the sprite table draws them.
SCENES = {
    "WORLD": (1480, [("background", 0, 0), ("treeline", 0, 0),
                     ("sack_spr", 240, 40), ("hut1", 540, 40),
                     ("hut2", 850, 50), ("tree2", 180, 0),
                     ("palmtree", 120, 20), ("chair", 65, 55)]),
    "TITLE": (320, [("background", -1050, 0), ("treeline", -1050, 0)]),
}

# One file each, in the order the loader reads them. Near-identical scenes
# go together and in sequence, which is what lets exomizer reference one from
# the next; the rest are grouped by when the game wants them.
GROUPS = [
    ["WORLD"],
    ["TITLE", "FACE", "LOGO", "BOAT"],
    ["SANTA_CHAIR", "SANTA_IDLE", "SANTA_JUMP", "SANTA_LEFT", "SANTA_RIGHT",
     "SEALION", "SEALION_BATTERY", "SEALION_WALK"],
    ["HUT_BOOK", "HUT_BOOK_NOBULB"],
    ["HUT_TV", "HUT_TV_NODISH"],
    ["SACK_NONE", "SACK_BULB", "SACK_DISH", "SACK_BATTERY",
     "SACK_BULB_DISH", "SACK_BULB_BATTERY", "SACK_BATTERY_DISH",
     "SACK_FULL"],
]

# Matches src/exo_asm.s, which is SearchAndRescue's port of the ozmoo-z6
# decruncher: raw stream, no load address, forwards, exomizer-2 layout.
EXO_FLAGS = ["raw", "-q", "-C", "-P0", "-c", "-m", "65535"]

# The Kernal reports EOF on a SEQ file 256 bytes early whatever its size, so
# the loader reads a known length and every file is padded past the tail.
TAIL_PAD = b"\0" * 512


def find_exomizer():
    for c in (os.environ.get("EXOMIZER"), "tools/exomizer",
              shutil.which("exomizer")):
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    sys.exit("exomizer not found: set $EXOMIZER or put it on PATH")


def crunch(exomizer, data):
    with tempfile.TemporaryDirectory() as d:
        raw, out = os.path.join(d, "raw"), os.path.join(d, "exo")
        with open(raw, "wb") as f:
            f.write(data)
        r = subprocess.run([exomizer] + EXO_FLAGS + [raw, "-o", out],
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        if r.returncode:
            sys.exit(f"exomizer failed: {r.stderr.decode()[:400]}")
        with open(out, "rb") as f:
            return f.read()


def find(assets, name):
    """The path of name in the first of the assets directories to have it."""
    for d in assets:
        path = os.path.join(d, name)
        if os.path.exists(path):
            return path
    return None


def gif_palette(im):
    """A palette image's 256 colours, r, g, b interleaved, 8 bits a channel."""
    pal = im.getpalette()[:768]
    return pal + [0] * (768 - len(pal))


def game_palette(assets):
    """assets/palette.gif's palette, or None if the game has none yet."""
    path = os.path.join(assets[0], PALETTE_GIF)
    if not os.path.exists(path):
        return None
    im = Image.open(path)
    if im.mode != "P":
        sys.exit(f"{PALETTE_GIF} is not a palette image")
    # A GIF's colour table can be shorter, and a tool that saves only the
    # colours the pixels use will make it so.
    if len(im.getpalette()) < 768:
        sys.exit(f"{PALETTE_GIF} has fewer than 256 colours")
    return gif_palette(im)


def remap(px, pal, palette):
    """Pixels drawn in pal, in the nearest colours of palette. Transparent
    stays transparent, and nothing else becomes it."""
    lut = nearest(np.array(pal, np.int32).reshape(256, 3), palette)
    lut[TRANSPARENT] = TRANSPARENT
    return lut[px]


def nearest(rgb, palette):
    """The index in palette of each of rgb's colours, an (n, 3) array: its
    own colour if the palette has it, else the nearest."""
    dst = np.array(palette, np.int32).reshape(256, 3)[:TRANSPARENT]
    out = np.empty(len(rgb), np.uint8)
    # In slices: a painted picture can have tens of thousands of colours.
    for i in range(0, len(rgb), 4096):
        d = rgb[i:i + 4096, None, :].astype(np.int32) - dst[None, :, :]
        # Squared distance weighted for the eye, which sees green best. The
        # first of equals wins, so black is 0 and never 254, the outline.
        out[i:i + 4096] = (d * d * [2, 4, 3]).sum(axis=2).argmin(axis=1)
    return out


def find_picture(assets, stem):
    """stem.png or stem.gif in the first directory to have either; the PNG
    if it has both. Only the game's own directory is searched for PNGs: the
    placeholders' are dos-like's sources, opaque on a pink background."""
    for d in assets:
        for ext in (".png", ".gif") if d == assets[0] else (".gif",):
            path = os.path.join(d, stem + ext)
            if os.path.exists(path):
                return path
    return None


def load_png(path, palette):
    """A PNG's pixels in palette."""
    if palette is None:
        sys.exit(f"{os.path.basename(path)}: a PNG needs {PALETTE_GIF}")
    rgba = np.array(Image.open(path).convert("RGBA"))
    opaque = rgba[:, :, 3] >= 128
    colours, inverse = np.unique(rgba[:, :, :3][opaque], axis=0,
                                 return_inverse=True)
    px = np.full(opaque.shape, TRANSPARENT, np.uint8)
    px[opaque] = nearest(colours, palette)[inverse.reshape(-1)]
    return px


def load_cels(assets, stem, palette):
    """The decoded cels of one bitmap, in palette, and palette: the first
    cel's if palette is None. An animation's cels all come from one
    directory and one format, those of cel 1."""
    if "%04d" not in stem:
        paths = [find_picture(assets, stem)]
        if not paths[0]:
            sys.exit(f"no {stem}.png or {stem}.gif")
    else:
        first = find_picture(assets, stem % 1)
        if not first:
            sys.exit(f"no cels for {stem}")
        d, ext = os.path.dirname(first), os.path.splitext(first)[1]
        paths = []
        while os.path.exists(os.path.join(d, stem % (len(paths) + 1)
                                          + ext)):
            paths.append(os.path.join(d, stem % (len(paths) + 1) + ext))
    own = os.path.normpath(os.path.dirname(paths[0])) == \
        os.path.normpath(assets[0])
    cels, remapped = [], False
    for path in paths:
        n = os.path.basename(path)
        if n.endswith(".png"):
            px = load_png(path, palette)
            if stem in CEL_ORIGIN and px.shape != (SCREEN_H, 320):
                x, y = CEL_ORIGIN[stem]
                h, w = px.shape
                screen = np.full((SCREEN_H, 320), TRANSPARENT, np.uint8)
                screen[y:y + h, x:x + w] = px
                px = screen
            cels.append(px)
            continue
        im = Image.open(path)
        if im.mode != "P":
            sys.exit(f"{n} is not a palette image")
        pal = gif_palette(im)
        px = np.array(im)
        if palette is None:
            palette = pal
        elif pal != palette:
            if own:
                sys.exit(f"{n} has a palette of its own: draw it in "
                         f"{PALETTE_GIF}'s, or save it as PNG")
            px = remap(px, pal, palette)
            remapped = True
        cels.append(px)
    if len({c.shape for c in cels}) != 1:
        sys.exit(f"{stem}: cels differ in size")
    if remapped:
        print(f"{stem}: placeholder, remapped to the game's palette")
    return cels, palette


def scene(assets, ident, palette):
    """Composite one of SCENES onto an opaque canvas of the original's 240
    rows, as the sprite table would draw it."""
    width, layers = SCENES[ident]
    canvas = np.full((240, width), TRANSPARENT, np.uint8)
    for stem, x, y in layers:
        (img,), palette = load_cels(assets, stem, palette)
        h, w = img.shape
        x0, y0 = max(x, 0), max(y, 0)
        x1, y1 = min(x + w, width), min(y + h, 240)
        src = img[y0 - y:y1 - y, x0 - x:x1 - x]
        dst = canvas[y0:y1, x0:x1]
        np.copyto(dst, src, where=src != TRANSPARENT)
    if (canvas[YCROP:YCROP + SCREEN_H] == TRANSPARENT).any():
        sys.exit(f"{ident} has holes in it")
    return [canvas], palette


def crop(cels):
    """Cut the cels to their shared box. Returns (cels, x, y): the box's
    origin in the original file."""
    top = 0
    if cels[0].shape[0] > SCREEN_H:
        # Only ever drawn at y = 0, so only these rows are ever on screen.
        top = YCROP
        cels = [c[YCROP:YCROP + SCREEN_H] for c in cels]
    mask = np.zeros(cels[0].shape, bool)
    for c in cels:
        mask |= c != TRANSPARENT
    ys, xs = np.nonzero(mask)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    return [c[y0:y1, x0:x1] for c in cels], int(x0), int(y0 + top)


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    build, source, assets = sys.argv[1], sys.argv[2], sys.argv[3:]
    exomizer = find_exomizer()

    # The labels are bitmaps like any other, TEXT_0 on, in a group of their
    # own: transparent around the letters, so they crunch to almost nothing.
    font_path = find(assets, "volter.fnt")
    if not font_path:
        sys.exit("no volter.fnt")
    font = labels.Font(font_path)
    texts = labels.strings(source)
    text_ids = [f"TEXT_{n}" for n in range(len(texts))]
    rendered = {i: labels.label_bitmap(font, *t)
                for i, t in zip(text_ids, texts)}
    groups = GROUPS + [text_ids]

    ids = [b[0] for b in BITMAPS] + text_ids
    grouped = [i for g in groups for i in g]
    if sorted(grouped) != sorted(ids):
        sys.exit("GROUPS and BITMAPS disagree")

    table, files = {}, []
    palette = game_palette(assets)
    addr = ATTIC_GFX
    raw_total = disk_total = 0
    for n, group in enumerate(groups):
        start = addr
        blob = bytearray()
        for ident in group:
            if ident in rendered:
                px, x, y = rendered[ident]
                h, w = px.shape
                table[ident] = (addr + len(blob), w, h, x, y, 1, 0)
                blob += np.ascontiguousarray(px.T).tobytes()
                continue
            if ident in SCENES:
                cels, palette = scene(assets, ident, palette)
            else:
                cels, palette = load_cels(assets, dict(BITMAPS)[ident],
                                          palette)
            cels, x, y = crop(cels)
            h, w = cels[0].shape
            if h > 255:
                sys.exit(f"{ident} is {h} tall; the table holds a byte")
            # Opaque: nothing behind it shows, so if it also covers the
            # screen, drawsprites need not draw anything under it.
            opaque = int(all((c != TRANSPARENT).all() for c in cels))
            table[ident] = (addr + len(blob), w, h, x, y, len(cels), opaque)
            for c in cels:
                blob += np.ascontiguousarray(c.T).tobytes()
        crunched = crunch(exomizer, bytes(blob))
        name = f"GFX{n}.BIN"
        with open(os.path.join(build, name.lower()), "wb") as f:
            f.write(struct.pack("<I", len(crunched)) + crunched + TAIL_PAD)
        files.append((name, start, len(crunched)))
        addr += len(blob)
        addr = (addr + 255) & ~255
        raw_total += len(blob)
        disk_total += len(crunched)
        print(f"{name}: {len(blob):7d} -> {len(crunched):6d}  "
              f"{' '.join(group)}")
    print(f"total: {raw_total // 1024} KB in attic RAM, "
          f"{disk_total // 1024} KB on disk")

    with open(os.path.join(build, "gfx_table.c"), "w") as f:
        f.write("// Generated by tools/convgfx.py. Do not edit.\n")
        f.write('#include "gfx.h"\n\n')
        f.write("const struct bitmap bitmaps[BITMAP_COUNT] = {\n")
        f.write("  {0},  // INVALID\n")
        for ident in ids:
            a, w, h, x, y, c, o = table[ident]
            f.write(f"  {{0x{a:07X}UL, {w}, {h}, {x}, {y}, {c}, {o}}},"
                    f"  // {ident}\n")
        f.write("};\n\n")
        # Looked up by the text, colour and wrap drawsprites is asked for,
        # so the game keeps its strings exactly as they are.
        f.write("const struct gfx_text gfx_texts[GFX_TEXTS] = {\n")
        for text, color, wrap in texts:
            quoted = text.replace("\\", "\\\\").replace('"', '\\"')
            f.write(f'  {{"{quoted}", {color}, {wrap}}},\n')
        f.write("};\n\n")
        f.write("const struct gfx_file gfx_files[GFX_FILES] = {\n")
        for name, start, length in files:
            f.write(f'  {{"{name}", 0x{start:07X}UL}},  // {length} bytes\n')
        f.write("};\n\n")
        # dos-like's own palette: r, g, b interleaved and 6 bits a channel,
        # which is what loadgif hands stranded.c and what setpal, getpal and
        # the ending's fade all work in.
        f.write("const uint8_t gfx_palette6[768] = {\n")
        for i in range(0, 768, 16):
            f.write("  " + ", ".join(f"{v >> 2}" for v in palette[i:i + 16])
                    + ",\n")
        f.write("};\n")

    with open(os.path.join(build, "gfx_ids.h"), "w") as f:
        f.write("// Generated by tools/convgfx.py. Do not edit.\n")
        f.write("#ifndef GFX_IDS_H\n#define GFX_IDS_H\n\n")
        f.write("enum bitmap_t {\n  INVALID,\n")
        for ident in ids:
            f.write(f"  {ident},\n")
        f.write("  BITMAP_COUNT\n};\n\n")
        f.write(f"#define GFX_TEXTS {len(texts)}\n")
        f.write(f"#define GFX_FILES {len(files)}\n")
        f.write(f"#define GFX_END   0x{addr:07X}UL\n\n#endif\n")


if __name__ == "__main__":
    main()
