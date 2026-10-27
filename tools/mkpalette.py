"""The game's palette: assets/palette.gif, and assets/palette.gpl for Krita.

    python3 tools/mkpalette.py

Sixteen rows of sixteen, each row a ramp from dark to light, so a colour's
index is row * 16 + column in the swatch grid. Every colour is quantised to
the 6 bits a channel the engine keeps (src/dos.c), so what is drawn is what
the machine shows.

**Change a colour, never the order**: a picture stores indices, so moving a
colour changes every picture drawn in it. After a change, every assets/*.kra
is reconverted by `make`; a GIF drawn without a .kra has to be redone.
"""

import os
import sys

from PIL import Image

ASSETS = os.path.join(os.path.dirname(__file__), "..", "assets")

# (name, colours): either 16 colours, or (dark, mid, light) to run 16 through.
RAMPS = [
    # Black to 222: the whites are the snow's, at the end of the next row.
    ("greys", ((0, 0, 0), (111, 111, 111), (222, 222, 222))),
    # Eight cool shadows for snow out of the light, then the eight whites
    # the machine can tell apart, 4 apart: background.kra's snow is 227..255.
    ("snow",
     [(24, 30, 58), (37, 46, 77), (49, 61, 96), (62, 77, 116),
      (74, 92, 135), (86, 107, 154), (98, 120, 166), (122, 140, 186),
      (226, 226, 226), (230, 230, 230), (234, 234, 234), (238, 238, 238),
      (242, 242, 242), (246, 246, 246), (250, 250, 250), (255, 255, 255)]),
    # background.kra's sky, its fourteen steps top to horizon, and two
    # lighter for haze.
    ("sky",
     [(48, 77, 147), (84, 103, 135), (89, 111, 143), (94, 118, 144),
      (102, 129, 151), (109, 146, 160), (116, 161, 167), (119, 173, 173),
      (119, 181, 179), (119, 189, 181), (119, 198, 188), (119, 209, 194),
      (114, 222, 198), (120, 228, 204), (160, 236, 218), (200, 246, 234)]),
    ("pine forest", ((4, 16, 14), (28, 82, 58), (150, 200, 150))),
    ("bright greens", ((8, 36, 8), (40, 150, 40), (200, 255, 150))),
    ("reds", ((40, 4, 8), (204, 24, 32), (255, 160, 150))),
    ("pinks", ((48, 8, 36), (210, 60, 150), (255, 205, 235))),
    ("purples", ((20, 10, 42), (112, 62, 172), (224, 204, 255))),
    ("skin", ((60, 26, 20), (204, 124, 92), (255, 230, 210))),
    ("wood", ((26, 12, 6), (122, 72, 36), (226, 186, 134))),
    ("oranges, glow", ((60, 20, 0), (240, 120, 20), (255, 224, 160))),
    ("yellows, gold", ((48, 34, 0), (232, 192, 32), (255, 255, 204))),
    ("sea, cyan", ((0, 20, 32), (20, 130, 164), (184, 244, 255))),
    ("sand, stone", ((40, 34, 28), (152, 136, 116), (242, 232, 212))),
    ("brick, rust", ((40, 14, 10), (152, 62, 42), (240, 168, 156))),
]

# Row 15: 240 and 241 the BASIC splash, 242..253 tinted near-blacks for
# outlines and deep shadow, 254 the labels' outline, 255 transparent.
ROW15 = [
    (0, 0, 170), (255, 255, 255),
    (10, 12, 26), (8, 20, 16), (26, 8, 10), (22, 14, 8), (18, 10, 26),
    (6, 18, 24), (16, 16, 20), (30, 22, 14), (14, 22, 36), (36, 16, 28),
    (20, 26, 14), (30, 30, 36),
    (0, 0, 0), (255, 0, 255),
]

NAMES = {240: "splash background", 241: "splash text",
         254: "label outline (do not use)", 255: "transparent (do not use)"}


def q(v):
    """8 bits to the 6 the engine keeps, and back, so >> 2 is exact."""
    v6 = round(max(0, min(255, v)) * 63 / 255)
    return (v6 << 2) | (v6 >> 4)


def ramp(colours, n=16):
    if len(colours) == n:
        return [tuple(q(c) for c in rgb) for rgb in colours]
    dark, mid, light = colours
    out = []
    for i in range(n):
        t = i / (n - 1) * 2
        a, b, t = (dark, mid, t) if t <= 1 else (mid, light, t - 1)
        out.append(tuple(q(x + (y - x) * t) for x, y in zip(a, b)))
    return out


def main():
    colours = []
    for name, spec in RAMPS:
        colours += ramp(spec)
    colours += [tuple(q(c) for c in rgb) for rgb in ROW15]
    assert len(colours) == 256
    # The reserved slots may repeat a ramp's colour (254 is 0's black, 241
    # 31's white): they exist to be changed on their own. Nothing else may.
    ramps = colours[:240] + colours[242:254]
    if len(ramps) != len(set(ramps)):
        seen = {}
        for i, c in enumerate(colours[:254]):
            if i not in (240, 241) and c in seen:
                print(f"{i} repeats {seen[c]}: {c}")
            seen.setdefault(c, i)
        sys.exit("duplicate colours")

    S = 16
    im = Image.new("P", (16 * S, 16 * S))
    im.putpalette([c for rgb in colours for c in rgb])
    im.putdata([(y // S) * 16 + x // S
                for y in range(16 * S) for x in range(16 * S)])
    im.save(os.path.join(ASSETS, "palette.gif"), optimize=False)

    with open(os.path.join(ASSETS, "palette.gpl"), "w") as f:
        f.write("GIMP Palette\nName: Sleigh Overflow\nColumns: 16\n#\n")
        for i, (r, g, b) in enumerate(colours):
            f.write(f"{r:3d} {g:3d} {b:3d}\t{i:3d} {NAMES.get(i, '')}"
                    .rstrip() + "\n")


if __name__ == "__main__":
    main()
