"""A PNG painted in the game's colours, as a GIF in the game's palette.

    python3 tools/png2gif.py <picture.png|.kra> [<picture.gif>] [--nearest]

Krita paints in RGB, and its GIF export quantises to the web-safe colours
whatever the palette, so the art is painted with assets/palette.gpl's
swatches, saved as .kra or exported as PNG, and turned into what
tools/convgfx.py wants here: a palette-mode GIF whose colour table is
assets/palette.gif's, exactly. A .kra is read through the flattened
mergedimage.png that Krita keeps inside it.

**Every pixel must be one of the palette's colours.** A brush with
anti-aliasing, a soft eraser or a scaled layer makes colours in between, and
this refuses those and says where the first of them are, so they are not
quietly turned into something else; --nearest maps them to the nearest
colour instead. Alpha below 128 is transparent, colour 255. Colour 254 is
black, like colour 0, and a black pixel becomes 0: 254 is the labels'
outline, which no picture needs.

The output defaults to the input's name with .gif, beside it.
"""

import os
import sys
import zipfile

import numpy as np
from PIL import Image

TRANSPARENT = 255
PALETTE_GIF = os.path.join(os.path.dirname(__file__), "..", "assets",
                           "palette.gif")


def main(args):
    nearest = "--nearest" in args
    args = [a for a in args if a != "--nearest"]
    if not 1 <= len(args) <= 2:
        sys.exit(__doc__.strip().splitlines()[2].strip())
    src = args[0]
    dst = args[1] if len(args) == 2 else os.path.splitext(src)[0] + ".gif"

    pal = Image.open(PALETTE_GIF).getpalette()[:768]
    colours = np.array(pal, int).reshape(256, 3)
    # The first index of each colour wins, so 254's black is 0.
    index = {}
    for i, rgb in enumerate(map(tuple, colours[:TRANSPARENT])):
        index.setdefault(rgb, i)

    if src.lower().endswith(".kra"):
        with zipfile.ZipFile(src) as kra:
            rgba = np.array(Image.open(kra.open("mergedimage.png"))
                            .convert("RGBA"))
    else:
        rgba = np.array(Image.open(src).convert("RGBA"))
    h, w = rgba.shape[:2]
    opaque = rgba[:, :, 3] >= 128
    rgb = rgba[:, :, :3].reshape(-1, 3)
    px = np.full(h * w, TRANSPARENT, np.uint8)
    stray = []
    for i in np.flatnonzero(opaque.reshape(-1)):
        c = tuple(int(v) for v in rgb[i])
        if c in index:
            px[i] = index[c]
        elif nearest:
            d = colours[:TRANSPARENT] - c
            px[i] = (d * d * [2, 4, 3]).sum(axis=1).argmin()
            stray.append(i)
        else:
            stray.append(i)
    if stray and not nearest:
        where = ", ".join(f"({i % w},{i // w})" for i in stray[:8])
        sys.exit(f"{src}: {len(stray)} pixels are not in the palette, "
                 f"first at {where}; --nearest maps them")
    if stray:
        print(f"{src}: {len(stray)} pixels mapped to the nearest colour")

    im = Image.new("P", (w, h))
    im.putpalette(pal)
    im.frombytes(px.tobytes())
    im.save(dst, optimize=False)
    print(f"{dst}: {w}x{h}")


if __name__ == "__main__":
    main(sys.argv[1:])
