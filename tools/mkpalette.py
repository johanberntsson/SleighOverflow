"""The game's palette, fitted to the art: assets/palette.gif, and
assets/palette.gpl for Krita.

    python3 tools/mkpalette.py

The Makefile runs it whenever a picture changes, before convgfx.py.

**Every picture is a PNG painted in RGB**, so no picture stores palette
indices and the palette is free to be whatever 256 colours suit the art
best. It is fitted by k-means over every opaque pixel of every picture the
game uses (BITMAPS and SCENES in convgfx.py), with the weighted distance
convgfx.py maps by. Each picture counts half by its pixels and half as one
picture among all, so that a small sprite -- a fox's chest, a present --
gets colours of its own and is not outvoted by the backdrops.

The palette used to be sixteen hand-chosen ramps. Paintings fall between
ramps, and a crimson couch came out brown and a cream chest grey.

**FIXED keeps its colours and its index**: the code and the labels name
those. The rest are sorted dark to light. Every colour is quantised to the
6 bits a channel the engine keeps (src/dos.c), so what is fitted is what
the machine shows. The fit is seeded, so the same art gives the same
palette.
"""

import glob
import os
import sys

import numpy as np
from PIL import Image

import convgfx

ASSETS = os.path.join(os.path.dirname(__file__), "..", "assets")

FIXED = {
    0: (0, 0, 0),           # black: convgfx maps ties to it, never to 254
    31: (255, 255, 255),    # COLOR_WHITE
    75: (113, 199, 93),     # COLOR_GREEN
    167: (227, 113, 20),    # COLOR_ORANGE
    185: (235, 207, 65),    # COLOR_YELLOW
    202: (73, 170, 195),    # COLOR_CYAN
    240: (0, 0, 170),       # splash background
    241: (255, 255, 255),   # splash text
    254: (0, 0, 0),         # label outline (do not use)
    255: (255, 0, 255),     # transparent (do not use)
}

NAMES = {31: "COLOR_WHITE", 75: "COLOR_GREEN", 167: "COLOR_ORANGE",
         185: "COLOR_YELLOW", 202: "COLOR_CYAN", 240: "splash background",
         241: "splash text", 254: "label outline (do not use)",
         255: "transparent (do not use)"}

# convgfx.nearest's weights, as a scale on each channel.
SCALE = np.sqrt([2.0, 4.0, 3.0])
ITERATIONS = 30


def q(v):
    """8 bits to the 6 the engine keeps, and back, so >> 2 is exact."""
    v = np.clip(np.asarray(v, float), 0, 255)
    v6 = np.round(v * 63 / 255).astype(int)
    return (v6 << 2) | (v6 >> 4)


def pictures():
    """Every PNG the game's bitmaps and scenes are made of."""
    stems = {s for _, s in convgfx.BITMAPS if s}
    for _, layers in convgfx.SCENES.values():
        stems |= {layer[0] for layer in layers}
    files = []
    for s in sorted(stems):
        if "%04d" in s:
            files += sorted(glob.glob(os.path.join(
                ASSETS, s.replace("%04d", "[0-9]" * 4) + ".png")))
        elif os.path.exists(os.path.join(ASSETS, s + ".png")):
            files.append(os.path.join(ASSETS, s + ".png"))
    return files


def weighted_colours(files):
    """Each opaque colour, at 6 bits a channel, and its weight."""
    total = {}
    pixels = []
    for f in files:
        a = np.array(Image.open(f).convert("RGBA"))
        pixels.append(q(a[:, :, :3][a[:, :, 3] >= 128]))
    n = sum(len(p) for p in pixels)
    for p in pixels:
        u, c = np.unique(p, axis=0, return_counts=True)
        w = 0.5 * c / n + 0.5 * c / len(p) / len(pixels)
        for col, wt in zip(map(tuple, u), w):
            total[col] = total.get(col, 0.0) + wt
    return (np.array(list(total.keys()), float),
            np.array(list(total.values())))


def assign(x, centres):
    """The nearest of centres to each of x."""
    out = np.empty(len(x), int)
    for i in range(0, len(x), 4096):
        d = x[i:i + 4096, None, :] - centres[None, :, :]
        out[i:i + 4096] = (d * d).sum(axis=2).argmin(axis=1)
    return out


def fit(colours, weights, k, fixed):
    """k centres, by k-means++ and Lloyd's, in the weighted space. The
    fixed colours take part in the assignment but do not move."""
    x = colours * SCALE
    fixed = np.array(fixed, float) * SCALE
    rng = np.random.default_rng(65)
    d = ((x[:, None, :] - fixed[None, :, :]) ** 2).sum(axis=2).min(axis=1)
    centres = []
    for _ in range(k):
        p = d * weights
        c = x[rng.choice(len(x), p=p / p.sum())]
        centres.append(c)
        d = np.minimum(d, ((x - c) ** 2).sum(axis=1))
    centres = np.array(centres)
    for _ in range(ITERATIONS):
        label = assign(x, np.vstack([centres, fixed]))
        for j in range(k):
            m = label == j
            if m.any():
                centres[j] = (x[m] * weights[m, None]).sum(axis=0) \
                    / weights[m].sum()
    return centres / SCALE


def main():
    colours, weights = weighted_colours(pictures())
    usable = [rgb for i, rgb in FIXED.items() if i not in (254, 255)]
    free = [i for i in range(256) if i not in FIXED]
    fitted = [tuple(int(v) for v in q(c))
              for c in fit(colours, weights, len(free), usable)]
    # Two centres can round to one colour, and a fitted one to a fixed one;
    # a duplicate is harmless, but nudge it so each entry is its own.
    seen = set(FIXED.values())
    for i, c in enumerate(fitted):
        while c in seen:
            c = (c[0], c[1], min(255, c[2] + 4) if c[2] < 252 else c[2] - 4)
        seen.add(c)
        fitted[i] = c
    fitted.sort(key=lambda c: (2 * c[0] + 4 * c[1] + 3 * c[2], c))

    palette = [None] * 256
    for i, rgb in FIXED.items():
        palette[i] = rgb
    for i, rgb in zip(free, fitted):
        palette[i] = rgb

    S = 16
    im = Image.new("P", (16 * S, 16 * S))
    im.putpalette([c for rgb in palette for c in rgb])
    im.putdata([(y // S) * 16 + x // S
                for y in range(16 * S) for x in range(16 * S)])
    im.save(os.path.join(ASSETS, "palette.gif"), optimize=False)

    with open(os.path.join(ASSETS, "palette.gpl"), "w") as f:
        f.write("GIMP Palette\nName: Sleigh Overflow\nColumns: 16\n#\n")
        for i, (r, g, b) in enumerate(palette):
            f.write(f"{r:3d} {g:3d} {b:3d}\t{i:3d} {NAMES.get(i, '')}"
                    .rstrip() + "\n")


if __name__ == "__main__":
    sys.exit(main())
