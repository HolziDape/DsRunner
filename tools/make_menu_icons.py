"""Generates small menu icons for the display settings (16x16, 4 bpp, like Pico's icons).

The palette index is the coverage (0 = transparent, 15 = full): the launcher draws the icons with a
gradient palette from the background to the icon color.

Usage: python3 tools/make_menu_icons.py   (writes arm9/gfx/clockIcon.png and .grit)
"""
import math
import os
from PIL import Image, ImageDraw

SIZE = 16
SCALE = 16  # supersampling


def to_indexed(mask):
    """Turns an 'L' coverage mask (0..255) into a 16 level palette image."""
    small = mask.resize((SIZE, SIZE), Image.LANCZOS)
    out = Image.new("P", (SIZE, SIZE), 0)
    palette = []
    for i in range(16):
        palette.extend([i * 17] * 3)
    out.putpalette(palette + [0] * (768 - len(palette)))
    src = small.load()
    dst = out.load()
    for y in range(SIZE):
        for x in range(SIZE):
            dst[x, y] = min(15, int(src[x, y] * 15 / 255 + 0.5))
    return out


def make_clock_icon():
    s = SIZE * SCALE
    mask = Image.new("L", (s, s), 0)
    d = ImageDraw.Draw(mask)
    c = s / 2
    r = 6.5 * SCALE
    w = int(1.6 * SCALE)
    d.ellipse((c - r, c - r, c + r, c + r), outline=255, width=w)
    # hands: 12 o'clock (long) and 3 o'clock (short)
    hand = int(1.5 * SCALE)
    d.line((c, c, c, c - 4.2 * SCALE), fill=255, width=hand)
    d.line((c, c, c + 3.2 * SCALE, c), fill=255, width=hand)
    d.ellipse((c - hand * 0.7, c - hand * 0.7, c + hand * 0.7, c + hand * 0.7), fill=255)
    return to_indexed(mask)


if __name__ == "__main__":
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "arm9", "gfx")
    make_clock_icon().save(os.path.join(out_dir, "clockIcon.png"), bits=4)
    with open(os.path.join(out_dir, "clockIcon.grit"), "w") as f:
        f.write("# tile format\n-gt\n\n# graphics bit depth is 4 (16 color)\n-gB4\n\n-p!")
    print("clockIcon generated")
