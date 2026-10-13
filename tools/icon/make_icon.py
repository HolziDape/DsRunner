"""Draws the DsRunner banner icon: a dark rounded tile with two stylised screens
(artwork on top, tile grid below) in the launcher's accent colour.

Writes icon.png as 32x32 RGBA with at most 15 opaque colours. The BlocksDS
ndstool builds the banner palette from the colours itself and only treats
transparent pixels (alpha) as transparent: a palette entry 0 in a .bmp is
not kept, which made the corners show up magenta.

Usage: python make_icon.py <out.png>
"""
import sys

from PIL import Image, ImageDraw

SS = 8  # supersampling
SIZE = 32
BG = (24, 28, 38)
TILE = (44, 50, 68)
ACCENT = (76, 194, 255)


def draw():
    s = SIZE * SS
    # the corners are cut off with a mask later; drawing them in the tile colour keeps
    # the downscaled edge pixels free of a coloured fringe
    img = Image.new("RGB", (s, s), BG)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, s - 1, s - 1), 7 * SS, fill=BG)
    d.rounded_rectangle((SS, SS, s - SS - 1, s - SS - 1), 6 * SS, outline=ACCENT, width=SS)
    # top screen: artwork
    d.rounded_rectangle((6 * SS, 5 * SS, 26 * SS, 14 * SS), 2 * SS, fill=ACCENT)
    d.polygon(((9 * SS, 13 * SS), (14 * SS, 8 * SS), (18 * SS, 12 * SS), (21 * SS, 10 * SS), (24 * SS, 13 * SS)), fill=(200, 240, 255))
    # bottom screen: tile grid, first tile selected
    for row in range(2):
        for col in range(3):
            x = 6 * SS + col * 7 * SS
            y = 17 * SS + row * 5 * SS
            fill = ACCENT if row == 0 and col == 0 else TILE
            d.rounded_rectangle((x, y, x + 6 * SS, y + 4 * SS), SS, fill=fill)
    return img


def to_png(img, path):
    small = img.resize((SIZE, SIZE), Image.LANCZOS)
    # keep the corners exactly transparent
    mask = Image.new("L", (SIZE * SS, SIZE * SS), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, SIZE * SS - 1, SIZE * SS - 1), 7 * SS, fill=255)
    mask = mask.resize((SIZE, SIZE), Image.LANCZOS).point(lambda v: 255 if v > 127 else 0)
    # 15 colours: the banner palette has 16 entries and entry 0 is the transparent one
    quant = small.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    out = quant.convert("RGBA")
    out.putalpha(mask)
    # fully transparent pixels all get the same colour, so they count as one palette entry
    out.putdata([p if p[3] else (0, 0, 0, 0) for p in out.getdata()])
    out.save(path)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "icon.png"
    to_png(draw(), out)
