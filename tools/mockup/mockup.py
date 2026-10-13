"""Renders DsRunner UI mockups at native DS resolution (2 x 256x192).

Placeholder art is generated procedurally; real artwork comes later from the PC
art tool. Output: PNGs scaled up with nearest neighbour so pixels stay visible.

Usage: python mockup.py <output dir>
"""
import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 256, 192
SCALE = 3
FONT_DIR = os.path.join(os.environ.get("WINDIR", "C:\\Windows"), "Fonts")

# dark theme palette
BG_TOP = (15, 17, 23)
BG_BOTTOM = (24, 28, 38)
TILE = (35, 40, 56)
TEXT = (235, 238, 245)
TEXT_DIM = (150, 158, 178)
ACCENT = (76, 194, 255)
PILL = (44, 50, 68)


def font(size, bold=False):
    name = "segoeuib.ttf" if bold else "segoeui.ttf"
    return ImageFont.truetype(os.path.join(FONT_DIR, name), size)


def gradient(size, c1, c2, diagonal=True):
    w, h = size
    img = Image.new("RGB", size)
    px = img.load()
    for y in range(h):
        for x in range(w):
            t = ((x / w) + (y / h)) / 2 if diagonal else y / h
            px[x, y] = tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3))
    return img


def to_ds_color(img):
    # DS screens show 15 bit colour (5 bits per channel)
    return img.point(lambda v: (v >> 3) << 3)


def placeholder_art(size, c1, c2, seed, label=None, label_size=28):
    """Abstract hero art: gradient plus soft shapes, optional big title text."""
    rnd = random.Random(seed)
    img = gradient(size, c1, c2)
    overlay = Image.new("RGBA", size, (0, 0, 0, 0))
    d = ImageDraw.Draw(overlay)
    w, h = size
    for _ in range(7):
        r = rnd.randint(h // 6, h // 2)
        x, y = rnd.randint(-r, w), rnd.randint(-r, h)
        col = tuple(min(255, c + rnd.randint(30, 90)) for c in c2)
        d.ellipse((x, y, x + r * 2, y + r * 2), fill=col + (rnd.randint(25, 60),))
    for i in range(0, w + h, 18):
        d.line((i, 0, i - h, h), fill=(255, 255, 255, 10), width=4)
    overlay = overlay.filter(ImageFilter.GaussianBlur(2))
    img = Image.alpha_composite(img.convert("RGBA"), overlay)
    if label:
        d = ImageDraw.Draw(img)
        f = font(label_size, bold=True)
        tw = d.textlength(label, font=f)
        x, y = (w - tw) / 2, h / 2 - label_size * 0.7
        d.text((x + 2, y + 2), label, font=f, fill=(0, 0, 0, 160))
        d.text((x, y), label, font=f, fill=(255, 255, 255, 255))
    return img.convert("RGB")


def rounded_paste(base, tile, xy, radius):
    mask = Image.new("L", tile.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, tile.size[0] - 1, tile.size[1] - 1), radius, fill=255)
    base.paste(tile, xy, mask)


def status_pill(img):
    d = ImageDraw.Draw(img, "RGBA")
    x0, y0, x1, y1 = W - 66, 5, W - 5, 19
    d.rounded_rectangle((x0, y0, x1, y1), 7, fill=(0, 0, 0, 140))
    d.text((x0 + 7, y0 + 1), "17:42", font=font(10, True), fill=TEXT)
    # battery
    bx = x1 - 22
    d.rounded_rectangle((bx, y0 + 4, bx + 14, y0 + 10), 1, outline=TEXT)
    d.rectangle((bx + 15, y0 + 6, bx + 16, y0 + 8), fill=TEXT)
    d.rectangle((bx + 2, y0 + 6, bx + 10, y0 + 8), fill=(120, 230, 140))


def top_screen(art, title, subtitle):
    img = art.copy().convert("RGBA")
    shade = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shade)
    for y in range(H // 2, H):
        a = int(200 * (y - H / 2) / (H / 2))
        sd.line((0, y, W, y), fill=(0, 0, 0, a))
    img = Image.alpha_composite(img, shade)
    d = ImageDraw.Draw(img)
    d.text((12, H - 46), title, font=font(18, True), fill=TEXT)
    d.text((12, H - 22), subtitle, font=font(11), fill=TEXT_DIM)
    status_pill(img)
    return img.convert("RGB")


def bottom_screen(header, sort_label, tiles, cols, rows, selected, back=False):
    img = gradient((W, H), BG_TOP, BG_BOTTOM, diagonal=False).convert("RGBA")
    d = ImageDraw.Draw(img)
    # header bar
    hx = 10
    if back:
        d.line((12, 7, 8, 11), fill=TEXT, width=2)
        d.line((8, 11, 12, 15), fill=TEXT, width=2)
        hx = 18
    d.text((hx, 4), header, font=font(12, True), fill=TEXT)
    sw = d.textlength(sort_label, font=font(10)) + 9
    d.rounded_rectangle((W - sw - 42, 5, W - 26, 19), 7, fill=PILL)
    d.text((W - sw - 36, 5), sort_label, font=font(10), fill=TEXT_DIM)
    ax = W - 36
    d.polygon(((ax, 11), (ax + 6, 11), (ax + 3, 14)), fill=TEXT_DIM)
    d.ellipse((W - 21, 5, W - 7, 19), fill=PILL)
    d.text((W - 18, 4), "⚙", font=ImageFont.truetype(os.path.join(FONT_DIR, "seguisym.ttf"), 11), fill=TEXT_DIM)

    gap = 8 if cols == 3 else 6
    size = 72 if cols == 3 else 51
    radius = 9 if cols == 3 else 7
    grid_w = cols * size + (cols - 1) * gap
    grid_h = rows * size + (rows - 1) * gap
    ox = (W - grid_w) // 2
    oy = 24 + (H - 24 - grid_h) // 2
    for i, tile in enumerate(tiles[: cols * rows]):
        cx, cy = i % cols, i // cols
        x, y = ox + cx * (size + gap), oy + cy * (size + gap)
        art = tile.resize((size, size), Image.LANCZOS)
        if i == selected:
            glow = Image.new("RGBA", (W, H), (0, 0, 0, 0))
            ImageDraw.Draw(glow).rounded_rectangle((x - 4, y - 4, x + size + 3, y + size + 3), radius + 4, fill=ACCENT + (170,))
            img = Image.alpha_composite(img, glow.filter(ImageFilter.GaussianBlur(3)))
            d = ImageDraw.Draw(img)
        else:
            art = Image.blend(art, Image.new("RGB", art.size, BG_TOP), 0.25)
        rounded_paste(img, art, (x, y), radius)
        if i == selected:
            d.rounded_rectangle((x - 2, y - 2, x + size + 1, y + size + 1), radius + 2, outline=ACCENT, width=2)
            d.rounded_rectangle((x - 1, y - 1, x + size, y + size), radius + 1, outline=(255, 255, 255), width=1)
    # page dots, only where the grid leaves room
    for p in range(3 if rows == 2 else 0):
        c = TEXT if p == 0 else (90, 96, 115)
        d.ellipse((W // 2 - 13 + p * 10, H - 6, W // 2 - 9 + p * 10, H - 2), fill=c)
    return img.convert("RGB")


def console_tile(label, c1, c2, seed):
    t = placeholder_art((96, 96), c1, c2, seed)
    d = ImageDraw.Draw(t)
    f = font(26 if len(label) <= 3 else 20, True)
    if not label.isascii():
        f = ImageFont.truetype(os.path.join(FONT_DIR, "seguisym.ttf"), 34)
    tw = d.textlength(label, font=f)
    d.text(((96 - tw) / 2 + 1, 33), label, font=f, fill=(0, 0, 0))
    d.text(((96 - tw) / 2, 32), label, font=f, fill=(255, 255, 255))
    return t


def cover_tile(name, c1, c2, seed):
    t = placeholder_art((96, 96), c1, c2, seed)
    d = ImageDraw.Draw(t, "RGBA")
    d.rectangle((0, 70, 96, 96), fill=(0, 0, 0, 120))
    initials = "".join(w[0] for w in name.split()[:2]).upper()
    f = font(30, True)
    tw = d.textlength(initials, font=f)
    d.text(((96 - tw) / 2, 22), initials, font=f, fill=(255, 255, 255))
    d.text((6, 74), name[:14], font=font(12, True), fill=(255, 255, 255))
    return t


CONSOLES = [
    ("NDS", "Nintendo DS", (40, 60, 120), (120, 160, 255), 34),
    ("GBA", "Game Boy Advance", (70, 30, 120), (190, 110, 255), 9),
    ("GBC", "Game Boy Color", (120, 40, 60), (255, 140, 90), 0),
    ("GB", "Game Boy", (40, 80, 40), (150, 200, 90), 0),
    ("NES", "NES", (90, 20, 20), (230, 70, 60), 0),
    ("SNES", "Super Nintendo", (50, 40, 90), (170, 140, 230), 0),
    ("SMS", "Master System", (20, 50, 90), (60, 150, 230), 0),
    ("GG", "Game Gear", (30, 30, 30), (110, 110, 130), 0),
    ("MD", "Mega Drive", (10, 10, 10), (90, 90, 100), 0),
    ("PCE", "PC Engine", (80, 80, 90), (200, 200, 210), 0),
    ("APP", "Apps", (20, 90, 90), (80, 220, 200), 0),
    ("★", "Favoriten", (100, 80, 10), (255, 210, 80), 0),
]

GAMES = [
    "Kart Racer DS", "Pocket Monsters", "Sky Knights", "Puzzle Quest", "Dragon Tales",
    "Ninja Edge", "Brain Trainer", "Star Pilot", "Castle Shadows", "Island Life",
    "Soccer Stars", "Retro Blaster",
]


def main(out):
    os.makedirs(out, exist_ok=True)
    rnd = random.Random(7)
    console_tiles = [console_tile(c[0], c[2], c[3], i) for i, c in enumerate(CONSOLES)]
    game_tiles = []
    for i, g in enumerate(GAMES):
        h = rnd.random()
        c1 = tuple(int(40 + 60 * math.sin(h * 6.28 + k)) for k in (0, 2, 4))
        c2 = tuple(min(255, int(150 + 100 * math.sin(h * 6.28 + k + 1))) for k in (0, 2, 4))
        game_tiles.append(cover_tile(g, c1, c2, i + 100))

    nds = CONSOLES[0]
    screens = {
        "1_konsolen_3x2": (
            top_screen(placeholder_art((W, H), nds[2], nds[3], 1, "NINTENDO DS", 26), nds[1], "34 Spiele  ·  zuletzt: Kart Racer DS"),
            bottom_screen("Konsolen", "Zuletzt", console_tiles, 3, 2, 0),
        ),
        "2_konsolen_4x3": (
            top_screen(placeholder_art((W, H), CONSOLES[1][2], CONSOLES[1][3], 2, "GAME BOY ADVANCE", 18), CONSOLES[1][1], "9 Spiele"),
            bottom_screen("Konsolen", "A–Z", console_tiles, 4, 3, 1),
        ),
        "3_spiele_4x3": (
            top_screen(placeholder_art((W, H), (20, 20, 40), (230, 90, 60), 5, "KART RACER", 26), "Kart Racer DS", "Nintendo DS  ·  zuletzt gespielt: heute"),
            bottom_screen("Nintendo DS", "Zuletzt", game_tiles, 4, 3, 0, back=True),
        ),
        "4_spiele_3x2": (
            top_screen(placeholder_art((W, H), (10, 10, 20), (200, 30, 40), 6, "NINJA EDGE", 26), "Ninja Edge", "Nintendo DS  ·  2 Spielstände"),
            bottom_screen("Nintendo DS", "Zuletzt", game_tiles[3:], 3, 2, 2, back=True),
        ),
    }

    shots = []
    for name, (top, bottom) in screens.items():
        top, bottom = to_ds_color(top), to_ds_color(bottom)
        gap = 10
        frame = Image.new("RGB", (W + 20, H * 2 + gap + 20), (205, 205, 210))
        frame.paste(top, (10, 10))
        frame.paste(bottom, (10, 10 + H + gap))
        big = frame.resize((frame.width * SCALE, frame.height * SCALE), Image.NEAREST)
        big.save(os.path.join(out, name + ".png"))
        shots.append(big)

    sheet = Image.new("RGB", (sum(s.width for s in shots) + 20 * (len(shots) + 1), shots[0].height + 40), (40, 42, 48))
    x = 20
    for s in shots:
        sheet.paste(s, (x, 20))
        x += s.width + 20
    sheet.save(os.path.join(out, "0_uebersicht.png"))
    print("saved to", out)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "mockups")
