"""Generates small menu icons for the display settings (16x16, 4 bpp, like Pico's icons).

The palette index is the coverage (0 = transparent, 15 = full): the launcher draws the icons with a
gradient palette from the background to the icon color.

Usage: python3 tools/make_menu_icons.py   (writes the icons below to arm9/gfx/<name>.png and .grit)
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


def new_mask():
    s = SIZE * SCALE
    return Image.new("L", (s, s), 0)


def make_chevron_icon(direction):
    """Page switch of the display settings: "›" (direction 1) or "‹" (direction -1)."""
    mask = new_mask()
    d = ImageDraw.Draw(mask)
    c = SIZE * SCALE / 2
    w = int(1.8 * SCALE)
    tip = c + direction * 2.5 * SCALE
    back = c - direction * 2.5 * SCALE
    points = [(back, c - 5 * SCALE), (tip, c), (back, c + 5 * SCALE)]
    d.line(points, fill=255, width=w, joint="curve")
    for x, y in (points[0], points[2]):
        d.ellipse((x - w / 2, y - w / 2, x + w / 2, y + w / 2), fill=255)
    return to_indexed(mask)


def make_radio_icon(selected):
    """Choice of the key combination and the language: a ring, with a dot when selected."""
    mask = new_mask()
    d = ImageDraw.Draw(mask)
    c = SIZE * SCALE / 2
    r = 6.2 * SCALE
    d.ellipse((c - r, c - r, c + r, c + r), outline=255, width=int(1.5 * SCALE))
    if selected:
        r = 3.3 * SCALE
        d.ellipse((c - r, c - r, c + r, c + r), fill=255)
    return to_indexed(mask)


def make_in_game_reset_icon():
    """Return from games: a house outline (the launcher is "home")."""
    mask = new_mask()
    d = ImageDraw.Draw(mask)
    k = SCALE
    w = int(1.5 * k)
    roof = [(1.8 * k, 8.2 * k), (8 * k, 2.2 * k), (14.2 * k, 8.2 * k)]
    d.line(roof, fill=255, width=w, joint="curve")
    d.line([(4 * k, 7 * k), (4 * k, 13.5 * k), (12 * k, 13.5 * k), (12 * k, 7 * k)], fill=255, width=w, joint="curve")
    # door
    d.rectangle((6.8 * k, 9.5 * k, 9.2 * k, 13.5 * k), fill=255)
    for x, y in (roof[0], roof[2]):
        d.ellipse((x - w / 2, y - w / 2, x + w / 2, y + w / 2), fill=255)
    return to_indexed(mask)


ICONS = {
    "clockIcon": make_clock_icon,
    "pageNextIcon": lambda: make_chevron_icon(1),
    "pagePrevIcon": lambda: make_chevron_icon(-1),
    "radioOnIcon": lambda: make_radio_icon(True),
    "radioOffIcon": lambda: make_radio_icon(False),
    "inGameResetIcon": make_in_game_reset_icon,
}


if __name__ == "__main__":
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "arm9", "gfx")
    for name, make in ICONS.items():
        make().save(os.path.join(out_dir, name + ".png"), bits=4)
        with open(os.path.join(out_dir, name + ".grit"), "w") as f:
            f.write("# tile format\n-gt\n\n# graphics bit depth is 4 (16 color)\n-gB4\n\n-p!")
        print(name + " generated")
