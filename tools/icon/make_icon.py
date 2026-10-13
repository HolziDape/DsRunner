"""Draws the DsRunner banner icon: a dark rounded tile with two stylised screens
(artwork on top, tile grid below) in the launcher's accent colour.

Writes icon.bmp as 32x32, 8 bpp with a 16 colour palette (the format ndstool
expects for banner icons; palette entry 0 is transparent).

Usage: python make_icon.py <out.bmp>
"""
import struct
import sys

from PIL import Image, ImageDraw

SS = 8  # supersampling
SIZE = 32
BG = (24, 28, 38)
TILE = (44, 50, 68)
ACCENT = (76, 194, 255)
TRANSPARENT = (255, 0, 255)


def draw():
    s = SIZE * SS
    img = Image.new("RGB", (s, s), TRANSPARENT)
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


def to_bmp16(img, path):
    small = img.resize((SIZE, SIZE), Image.LANCZOS)
    # keep the corners exactly transparent
    mask = Image.new("L", (SIZE * SS, SIZE * SS), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, SIZE * SS - 1, SIZE * SS - 1), 7 * SS, fill=255)
    mask = mask.resize((SIZE, SIZE), Image.LANCZOS).point(lambda v: 255 if v > 127 else 0)
    quant = small.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = quant.getpalette()[: 15 * 3]
    palette = [TRANSPARENT] + [tuple(pal[i:i + 3]) for i in range(0, len(pal), 3)]
    palette += [(0, 0, 0)] * (16 - len(palette))
    q = quant.load()
    m = mask.load()
    rows = []
    for y in range(SIZE - 1, -1, -1):  # bmp rows are bottom-up
        rows.append(bytes(0 if m[x, y] == 0 else q[x, y] + 1 for x in range(SIZE)))
    pixels = b"".join(rows)
    pal_bytes = b"".join(bytes((b, g, r, 0)) for r, g, b in palette)
    offset = 14 + 40 + len(pal_bytes)
    header = b"BM" + struct.pack("<IHHI", offset + len(pixels), 0, 0, offset)
    info = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE, 1, 8, 0, len(pixels), 2835, 2835, 16, 16)
    with open(path, "wb") as f:
        f.write(header + info + pal_bytes + pixels)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "icon.bmp"
    to_bmp16(draw(), out)
    preview = Image.open(out).convert("RGB").resize((SIZE * 8, SIZE * 8), Image.NEAREST)
    preview.save(out.replace(".bmp", "_preview.png"))
