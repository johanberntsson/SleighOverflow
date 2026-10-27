"""Every string the game draws, rendered the way dos-like draws it.

The original draws each label nine or more times a frame -- eight 1-pixel
offsets and five more for a drop shadow, all in colour 254, then the text in
its own colour on top -- through centertextxy and the proportional font
volter.fnt. Every one of those strings is a constant in src/game.c, so here
they are drawn once, offline, into ordinary bitmaps, and the MEGA65 blits
them like any sprite. The font never reaches the machine.

`render` is pixelfont_blit from dos-like's libs/pixelfont.h, cut down to what
centertextxy asks of it: centre alignment, no bold, italic or underline,
spacing 0. The font has no kerning pairs and every pixel is 0 or 1, both
checked when it loads.
"""

import re
import struct
import sys

import numpy as np

TRANSPARENT = 255
OUTLINE = 254  # stranded.c's setcolor( 254 ) in drawsprites
LABEL_DY = 30  # drawsprites' `yp += 30`

# drawsprites' outline passes, in its order, then the text itself at (0, 0).
OUTLINE_OFFSETS = [(x, y) for y in (-1, 0, 1) for x in (-1, 0, 1)
                   if (x, y) != (0, 0)] + [(2, 0), (2, 1), (-1, 2), (0, 2),
                                           (1, 2)]


class Font:
    def __init__(self, path):
        d = open(path, "rb").read()
        # struct pixelfont_t: a u32, three u8s, then the u32 offsets table
        # aligned to 4, then the glyphs.
        _, self.height, self.line_spacing, self.baseline = struct.unpack_from(
            "<IBBB", d, 0)
        offsets = struct.unpack_from("<256I", d, 8)
        base = 8 + 1024
        self.glyphs = {}
        for ch in range(256):
            o = base + offsets[ch]
            if o + 2 > len(d):
                continue
            lead = struct.unpack_from("b", d, o)[0]
            w = d[o + 1]
            px = np.frombuffer(d, np.uint8, self.height * w, o + 2)
            px = px.reshape(self.height, w)
            o += 2 + self.height * w
            trail = struct.unpack_from("b", d, o)[0]
            kern = d[o + 1]
            if kern:
                sys.exit(f"{path}: kerning is not supported")
            if px.size and px.max() > 1:
                sys.exit(f"{path}: anti-aliased pixels are not supported")
            self.glyphs[ch] = (lead, px, trail)

    def render(self, canvas, x, y, text, color, wrap):
        """pixelfont_blit, centred. Draws into canvas, a 2-D uint8 array."""
        s = text.encode("latin-1")
        h = self.height
        xp = x
        i = 0
        while i < len(s):
            count = width = last_space_count = last_space_width = 0
            t = i
            while (t < len(s) and s[t] != 10
                   and (wrap <= 0 or width <= wrap)):
                if s[t] <= 32:
                    last_space_count, last_space_width = count, width
                lead, _, trail = self.glyphs[s[t]]
                width += lead + trail
                t += 1
                count += 1
            skip_space = False
            if wrap > 0 and width > wrap:
                if last_space_count > 0:
                    count = last_space_count
                width = last_space_width
                skip_space = True
            # C division truncates towards zero.
            if wrap > 0:
                x += int((wrap - width) / 2)
            else:
                x -= int(width / 2)
            for _ in range(count):
                lead, px, trail = self.glyphs[s[i]]
                x += lead
                ys, xs = np.nonzero(px)
                canvas[y + ys, x + xs] = color  # color + col - 1, col is 1
                x += trail
                i += 1
            x = xp
            y += self.line_spacing
            if i < len(s) and s[i] == 10:
                i += 1
            if i < len(s) and skip_space and s[i] <= 32:
                i += 1


def label_bitmap(font, text, color, wrap):
    """One label as drawsprites draws it, cropped. Returns (pixels, x, y):
    the pixels' top left relative to the label sprite's position, with the
    `yp += 30` folded into y."""
    ox, oy = 320, 40
    canvas = np.full((200, 960), TRANSPARENT, np.uint8)
    for dx, dy in OUTLINE_OFFSETS:
        font.render(canvas, ox + dx, oy + dy, text, OUTLINE, wrap)
    font.render(canvas, ox, oy, text, color, wrap)
    ys, xs = np.nonzero(canvas != TRANSPARENT)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    return canvas[y0:y1, x0:x1], int(x0 - ox), int(y0 - oy + LABEL_DY)


def strings(source):
    """Every (text, colour, wrap) the source draws, in order of appearance."""
    c = open(source, encoding="latin-1").read()
    colors = {m[1]: int(m[2])
              for m in re.finditer(r"#define\s+(COLOR_\w+)\s+(\d+)", c)}
    speakers = {m[1]: (colors[m[2]], int(m[3])) for m in re.finditer(
        r'\{\s*"(\w+)"\s*,\s*-?\d+\s*,\s*-?\d+\s*,\s*(COLOR_\w+)\s*,'
        r'\s*(\d+)\s*\}', c)}
    found = []
    # The dialogue: { actor, line }, where an actor starting with # is an
    # event and not speech.
    for m in re.finditer(r'\{\s*"([^"]+)"\s*,\s*"([^"]*)"\s*\}', c):
        if not m[1].startswith("#"):
            if m[1] not in speakers:
                sys.exit(f"{source}: no speech settings for {m[1]}")
            found.append((m[2],) + speakers[m[1]])
    # The interaction prompts, on a label made in white with no wrap.
    for m in re.finditer(r'set_interact\(\s*\w+\s*,\s*"([^"]+)"', c):
        found.append((m[1], colors["COLOR_WHITE"], 0))
    # Labels made with text of their own: the title's credits.
    for m in re.finditer(r'label\([^,]+,[^,]+,[^,]+,\s*"([^"]+)"\s*,\s*'
                         r'(COLOR_\w+)\s*\)', c):
        found.append((m[1], colors[m[2]], 0))
    unique = []
    for f in found:
        if f not in unique:
            unique.append(f)
    return unique
