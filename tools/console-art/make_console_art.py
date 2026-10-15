"""Draws DsRunner's built-in console images: our own stylized silhouettes, no logos.

Writes `_pico/themes/dsrunner/consoles/<KEY>.bmp` (top screen, 256x192), `<KEY>-tile.bmp` (3x2 grid,
64x64), `<KEY>-icon.bmp` (4x3 grid and top screen icon, 32x32, 4 bpp) and `<KEY>-cover.bmp` (cover flow,
128x96) for every key of ART_KEYS. See "Eingebaute Konsolen-Bilder" in docs/CONFIG.md.

The images are templates: every palette color is a gray level that the launcher maps onto the color
of the console (DsRunnerConsoleArt::Tint): 0 = the theme background, 128 = the console color,
255 = white. So one image fits every color, also consoles of /_dsrunner/systems.json. Palette index 0
is transparent (tile, icon) or unused (top screen, cover).

Usage: python3 tools/console-art/make_console_art.py [--preview preview.png]
The output is reproducible (no randomness), run it again after changing a drawing.
"""
from __future__ import annotations

import argparse
import math
import struct
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

REPO = Path(__file__).resolve().parents[2]
OUT_DIR = REPO / "_pico" / "themes" / "dsrunner" / "consoles"
sys.path.insert(0, str(REPO / "tools" / "dsrunner-art"))
from dsrunnerart.consoles import ART_KEYS, CONSOLES, UNKNOWN_HERO, UNKNOWN_TILE  # noqa: E402

# DsRunnerColors (arm9/source/themes/dsrunner/DsRunnerColorScheme.h)
BG_TOP = (15, 17, 23)
WHITE = (255, 255, 255)
# written into the reserved fields of the BMP file header like DsRunner Art does ("DR"), format 1
SIGNATURE = (0x5244, 1)

# gray levels of the drawings (template values, see the module doc)
BODY = 222       # the device itself: light console color
BODY2 = 196      # second, slightly darker part of the body
FACE = 150       # faces, stripes: between body and console color
ACCENT = 128     # buttons: exactly the console color
SCREEN = 46      # screens, dark faces: dark console color
SCREEN_IN = 92   # the picture inside a screen bezel
WHITE_TONE = 246 # highlights (the inner star of the favorites)
HOLE = -1        # cuts a hole into the shape (transparent / background)

SS = 8           # supersampling of the drawings


# --- drawing --------------------------------------------------------------------------------------

class Painter:
    """Draws a glyph given in design units into a supersampled tone + coverage image pair."""

    def __init__(self, width: int, height: int, ox: float, oy: float, scale: float):
        self.alpha = Image.new("L", (width * SS, height * SS), 0)
        self.tone = Image.new("L", (width * SS, height * SS), 0)
        self._a = ImageDraw.Draw(self.alpha)
        self._t = ImageDraw.Draw(self.tone)
        self.ox, self.oy, self.scale = ox, oy, scale
        # one unit is one pixel: small details snap to whole pixels
        self.pixel_art = scale == 1.0 and float(ox).is_integer() and float(oy).is_integer()

    def _p(self, x: float, y: float) -> tuple[float, float]:
        return (self.ox + x * self.scale) * SS, (self.oy + y * self.scale) * SS

    def _box(self, x0, y0, x1, y1):
        a = self._p(x0, y0)
        b = self._p(x1, y1)
        return a[0], a[1], b[0] - 1, b[1] - 1

    def _fill(self, method: str, shape, tone: int, **kwargs):
        if tone == HOLE:
            getattr(self._a, method)(shape, fill=0, **kwargs)
            return
        getattr(self._a, method)(shape, fill=255, **kwargs)
        getattr(self._t, method)(shape, fill=tone, **kwargs)

    def rect(self, x0, y0, x1, y1, tone, radius: float = 0):
        r = radius * self.scale * SS
        if r > 0:
            self._fill("rounded_rectangle", self._box(x0, y0, x1, y1), tone, radius=r)
        else:
            self._fill("rectangle", self._box(x0, y0, x1, y1), tone)

    def ellipse(self, x0, y0, x1, y1, tone):
        self._fill("ellipse", self._box(x0, y0, x1, y1), tone)

    def circle(self, cx, cy, r, tone):
        if self.pixel_art and r < 2.2:
            # small buttons of the icons: a clean block of whole pixels (2x2 or 3x3)
            size = 3 if r >= 1.4 else (2 if r >= 0.9 else 1)
            x0 = round(cx - size / 2)
            y0 = round(cy - size / 2)
            self.rect(x0, y0, x0 + size, y0 + size, tone, radius=0.5 if size == 3 else 0)
            return
        self.ellipse(cx - r, cy - r, cx + r, cy + r, tone)

    def poly(self, points, tone):
        self._fill("polygon", [self._p(x, y) for x, y in points], tone)

    def cross(self, cx, cy, size, arm, tone):
        """A d-pad: plus sign `size` wide with arms `arm` thick."""
        if self.pixel_art:
            # whole pixels: odd sizes around the center pixel
            size = max(3, round(size) | 1)
            arm = 1 if size <= 5 else 3
            cx, cy = math.floor(cx) + 0.5, math.floor(cy) + 0.5
        h, a = size / 2, arm / 2
        self.rect(cx - h, cy - a, cx + h, cy + a, tone)
        self.rect(cx - a, cy - h, cx + a, cy + h, tone)

    def pill(self, x0, y0, x1, y1, tone):
        self.rect(x0, y0, x1, y1, tone, radius=min(x1 - x0, y1 - y0) / 2)

    def done(self, width: int, height: int) -> tuple[Image.Image, Image.Image]:
        """Tone (premultiplied, then divided again) and coverage at the target size."""
        black = Image.new("L", self.tone.size, 0)
        premul = Image.composite(self.tone, black, self.alpha)
        small_alpha = self.alpha.resize((width, height), Image.Resampling.BOX)
        small_premul = premul.resize((width, height), Image.Resampling.BOX)
        tone = Image.new("L", (width, height), 0)
        a, p, t = small_alpha.load(), small_premul.load(), tone.load()
        for y in range(height):
            for x in range(width):
                t[x, y] = min(255, round(p[x, y] * 255 / a[x, y])) if a[x, y] else 0
        return tone, small_alpha

    def done_crisp(self, width: int, height: int) -> tuple[Image.Image, Image.Image]:
        """Pixel art without smoothing: every pixel takes the tone (or the transparency) that covers
        most of it. For the 4 bpp icons, where in-between levels would only look blurry."""
        alpha, tone = self.alpha.load(), self.tone.load()
        out_tone = Image.new("L", (width, height), 0)
        out_alpha = Image.new("L", (width, height), 0)
        ot, oa = out_tone.load(), out_alpha.load()
        for y in range(height):
            for x in range(width):
                counts: dict[int, int] = {}
                for sy in range(y * SS, (y + 1) * SS):
                    for sx in range(x * SS, (x + 1) * SS):
                        value = tone[sx, sy] if alpha[sx, sy] >= 128 else -1
                        counts[value] = counts.get(value, 0) + 1
                value = max(counts.items(), key=lambda item: (item[1], item[0]))[0]
                if value >= 0:
                    ot[x, y] = value
                    oa[x, y] = 255
        return out_tone, out_alpha


def star_points(cx, cy, outer, inner, points=5, rotation=-90.0):
    result = []
    for i in range(points * 2):
        radius = outer if i % 2 == 0 else inner
        angle = math.radians(rotation + i * 180.0 / points)
        result.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
    return result


# Every glyph: (width, height) in design units and a function that draws it. One unit is one pixel
# of the 32x32 icon, so the icons stay crisp; the other sizes scale it up. `big` adds details that
# only work from the tile size on.

def draw_nds(p: Painter, big: bool):
    # DS Lite, opened: lid with the top screen, hinge gap, base with screen, d-pad and buttons
    p.rect(1, 0, 23, 11, BODY, radius=2)
    p.rect(6, 2, 18, 9, SCREEN, radius=0.5)
    p.rect(0, 12, 24, 24, BODY, radius=2)
    p.rect(0, 11.6, 24, 13.2, BODY2)
    p.rect(7, 14, 17, 22, SCREEN, radius=0.5)
    p.cross(3.5, 18, 4.2, 1.4, ACCENT)
    for dx, dy in ((0, -1.6), (1.6, 0), (0, 1.6), (-1.6, 0)):
        p.circle(20.5 + dx, 18 + dy, 0.8, ACCENT)
    if big:
        p.rect(7.5, 2.6, 17.5, 8.4, SCREEN_IN, radius=0.3)
        p.rect(7.6, 14.6, 16.4, 21.4, SCREEN_IN, radius=0.3)
        p.circle(3.5, 4, 0.6, BODY2)
        p.circle(20.5, 4, 0.6, BODY2)


def draw_gba(p: Painter, big: bool):
    # wide body with round wings, screen in the middle, d-pad left, A/B right
    p.rect(0, 1, 30, 18, BODY, radius=7)
    p.rect(5, 0, 25, 4, BODY, radius=2)
    p.rect(9, 2.5, 21, 11.5, SCREEN, radius=1)
    p.cross(4.6, 9.5, 4.6, 1.5, ACCENT)
    p.circle(23.6, 10.6, 1.25, ACCENT)
    p.circle(26.4, 8.6, 1.25, ACCENT)
    p.pill(12, 14.2, 14.4, 15.2, FACE)
    p.pill(15.6, 14.2, 18, 15.2, FACE)
    if big:
        p.rect(10.2, 3.6, 19.8, 10.4, SCREEN_IN, radius=0.4)


def _dmg_body(p: Painter, x0, x1, y1, corner):
    """Game Boy body: small radius, a large one bottom right."""
    p.rect(x0, 0, x1, y1, BODY, radius=1)
    p.rect(x1 - corner * 2, y1 - corner * 2, x1, y1, HOLE)
    p.rect(x1 - corner * 2, y1 - corner * 2, x1, y1, BODY, radius=corner)
    p.rect(x0, y1 - corner * 2, x1 - corner, y1, BODY, radius=1)
    p.rect(x1 - corner * 2, 0, x1, y1 - corner, BODY, radius=1)


def draw_gb(p: Painter, big: bool):
    # the original brick: big screen bezel, d-pad, A/B, start/select, speaker
    _dmg_body(p, 0, 17, 24, 4)
    p.rect(1.5, 1.5, 15.5, 11.5, SCREEN, radius=0.8)
    p.rect(4, 3, 13, 10, SCREEN_IN)
    p.cross(4.2, 15.6, 4.6, 1.5, SCREEN)
    p.circle(11.6, 16.6, 1.25, ACCENT)
    p.circle(14.4, 14.8, 1.25, ACCENT)
    p.pill(5.6, 20, 7.8, 21, FACE)
    p.pill(8.8, 20, 11, 21, FACE)
    if big:
        for i in range(4):
            x = 12.2 + i * 1.1
            p.poly([(x, 22.6), (x + 0.5, 22.6), (x + 1.9, 19.8), (x + 1.4, 19.8)], FACE)


def draw_gbc(p: Painter, big: bool):
    # smaller and rounder than the Game Boy, round bottom, screen with a wide bezel
    p.rect(0.5, 0, 16.5, 23, BODY, radius=1.5)
    p.rect(0.5, 12, 16.5, 23, HOLE)
    p.ellipse(0.5, 6, 16.5, 23, BODY)
    p.rect(0.5, 10, 16.5, 16, BODY)
    p.rect(2, 1.5, 15, 10.5, SCREEN, radius=1.5)
    p.rect(4.2, 2.8, 12.8, 9.2, SCREEN_IN)
    p.cross(4.4, 14.6, 4.4, 1.4, SCREEN)
    p.circle(11.6, 15.6, 1.2, ACCENT)
    p.circle(14.1, 13.9, 1.2, ACCENT)
    p.pill(6.2, 19, 8.2, 20, FACE)
    p.pill(8.8, 19, 10.8, 20, FACE)


def draw_nes(p: Painter, big: bool):
    # the rectangular pad: dark face, d-pad, select/start on stripes, two round buttons
    p.rect(0, 0, 30, 13, BODY, radius=1)
    p.rect(1.5, 1.5, 28.5, 11.5, SCREEN, radius=0.5)
    p.cross(6.5, 6.5, 6.4, 2, FACE)
    p.rect(11.5, 2.5, 18.5, 10.5, BODY2)
    p.rect(11.5, 4.5, 18.5, 5.5, FACE)
    p.pill(12.2, 7.4, 14.6, 8.6, SCREEN)
    p.pill(15.4, 7.4, 17.8, 8.6, SCREEN)
    p.circle(21.8, 7.4, 1.6, ACCENT)
    p.circle(26.2, 7.4, 1.6, ACCENT)


def draw_snes(p: Painter, big: bool):
    # dog bone shape, d-pad left, four buttons in a diamond right
    p.circle(7, 7, 7, BODY)
    p.circle(23, 7, 7, BODY)
    p.rect(7, 1, 23, 13, BODY)
    p.rect(9, 0.6, 21, 2, HOLE)
    p.ellipse(7, 0.4, 23, 4, BODY)
    p.cross(7, 7, 6.6, 2, SCREEN)
    for (dx, dy), tone in zip(((0, -2.8), (2.8, 0), (0, 2.8), (-2.8, 0)), (FACE, ACCENT, ACCENT, FACE)):
        p.circle(23 + dx, 7 + dy, 1.4, tone)
    p.poly([(12.4, 9), (13.4, 7.6), (14.6, 7.6), (13.6, 9)], SCREEN)
    p.poly([(15.6, 9), (16.6, 7.6), (17.8, 7.6), (16.8, 9)], SCREEN)


def draw_sms(p: Painter, big: bool):
    # Master System pad: plain rectangle, square d-pad with a center stick, buttons 1 and 2
    p.rect(0, 1, 28, 14, BODY, radius=2)
    p.rect(2.5, 3, 11, 12, SCREEN, radius=1.5)
    p.cross(6.75, 7.5, 6.4, 2.2, FACE)
    p.circle(6.75, 7.5, 1.0, SCREEN)
    p.circle(18.2, 8.4, 2.0, ACCENT)
    p.circle(23.6, 8.4, 2.0, ACCENT)
    p.rect(13, 0, 15, 1.4, BODY2)


def draw_md(p: Painter, big: bool):
    # crescent shaped pad, round d-pad left, three buttons in a row right
    p.ellipse(0, 0, 30, 17, BODY)
    p.ellipse(9, 12.5, 21, 22, HOLE)
    p.circle(7, 7.2, 4.4, SCREEN)
    p.cross(7, 7.2, 5.8, 1.8, FACE)
    p.circle(18.6, 9.4, 1.3, SCREEN)
    p.circle(22.0, 7.8, 1.3, SCREEN)
    p.circle(25.4, 6.2, 1.3, SCREEN)
    p.pill(13.6, 3.2, 16.4, 4.4, ACCENT)


def draw_gg(p: Painter, big: bool):
    # Game Gear: wide body with round ends and a big screen in the middle
    p.rect(0, 1, 30, 17, BODY, radius=6)
    p.rect(8.5, 2, 21.5, 15, SCREEN, radius=1)
    p.rect(10, 3.5, 20, 11.5, SCREEN_IN)
    p.circle(4.4, 9, 2.8, SCREEN)
    p.cross(4.4, 9, 3.8, 1.3, FACE)
    p.circle(24.4, 10.2, 1.3, ACCENT)
    p.circle(27.0, 8.2, 1.3, ACCENT)
    p.pill(23.2, 4.2, 25.6, 5.2, FACE)


def draw_pce(p: Painter, big: bool):
    # PC Engine: small box seen from the front, top face, card slot and vents
    p.poly([(3, 0), (21, 0), (24, 5), (0, 5)], BODY)
    p.rect(0, 5, 24, 18, BODY2, radius=0.6)
    p.rect(3.5, 8.5, 20.5, 10.5, SCREEN)
    for i in range(5):
        p.rect(4 + i * 3.4, 13.2, 5.6 + i * 3.4, 15.6, FACE)
    p.rect(6, 1.6, 18, 3.4, FACE)
    p.rect(0, 18, 24, 19, SCREEN)


def draw_apps(p: Painter, big: bool):
    # apps: four tiles, one of them a plus
    p.rect(0, 0, 10, 10, BODY, radius=2.5)
    p.rect(13, 0, 23, 10, BODY, radius=2.5)
    p.rect(0, 13, 10, 23, BODY, radius=2.5)
    p.rect(13, 13, 23, 23, ACCENT, radius=2.5)
    p.cross(18, 18, 6, 2, BODY)


def draw_fav(p: Painter, big: bool):
    p.poly(star_points(12, 12.6, 12.4, 5.2), BODY)
    if big:
        p.poly(star_points(12, 12.6, 7.4, 3.1), WHITE_TONE)


def draw_search(p: Painter, big: bool):
    # the search: a magnifying glass, lens top left, handle to the bottom right
    p.circle(9.5, 9.5, 9.5, BODY)
    p.circle(9.5, 9.5, 6.2, SCREEN_IN if big else SCREEN)
    if big:
        # a light reflection on the glass
        p.poly([(5.2, 9.6), (6.2, 6.4), (9.4, 4.8), (9.6, 6.2), (7.4, 7.4), (6.6, 9.8)], WHITE_TONE)
    p.poly([(14.2, 17.0), (17.0, 14.2), (23.4, 20.6), (20.6, 23.4)], BODY)
    p.circle(22.0, 22.0, 2.0, BODY)
    if big:
        p.poly([(15.6, 18.4), (18.4, 15.6), (19.6, 16.8), (16.8, 19.6)], ACCENT)


def draw_folder(p: Painter, big: bool):
    p.rect(0, 0, 11, 6, BODY2, radius=1.5)
    p.rect(0, 2, 26, 20, BODY2, radius=2)
    p.rect(0, 5.5, 26, 20, BODY, radius=2)


def draw_cart(p: Painter, big: bool):
    # a generic cartridge: label area, grip ridges
    p.rect(0, 0, 20, 24, BODY, radius=1.2)
    p.rect(14, 0, 20, 3, HOLE)
    p.rect(14, 0, 20, 3.6, BODY2)
    p.rect(3, 5, 17, 16, ACCENT, radius=1)
    for i in range(4):
        p.rect(4 + i * 3.4, 19, 5.6 + i * 3.4, 22.4, BODY2)



GLYPHS = {
    "NDS": ((24, 24), draw_nds),
    "GBA": ((30, 18), draw_gba),
    "GB": ((17, 24), draw_gb),
    "GBC": ((17, 23), draw_gbc),
    "NES": ((30, 13), draw_nes),
    "SNES": ((30, 14), draw_snes),
    "SMS": ((28, 14), draw_sms),
    "MD": ((30, 17), draw_md),
    "GG": ((30, 17), draw_gg),
    "PCE": ((24, 19), draw_pce),
    "APPS": ((23, 23), draw_apps),
    "FAV": ((24, 24), draw_fav),
    "SEARCH": ((24, 24), draw_search),
    "FOLDER": ((26, 20), draw_folder),
    "CART": ((20, 24), draw_cart),
}


def render_glyph(key: str, box: tuple[float, float, float, float], canvas: tuple[int, int], big: bool,
                 max_scale: float = 99.0, rotate: float = 0.0):
    """The glyph fitted into box (x, y, width, height) of a canvas: (tone, coverage) images."""
    (gw, gh), draw = GLYPHS[key]
    bx, by, bw, bh = box
    scale = min(bw / gw, bh / gh, max_scale)
    ox = bx + (bw - gw * scale) / 2
    oy = by + (bh - gh * scale) / 2
    painter = Painter(canvas[0], canvas[1], ox, oy, scale)
    draw(painter, big)
    if rotate:
        center = ((ox + gw * scale / 2) * SS, (oy + gh * scale / 2) * SS)
        painter.alpha = painter.alpha.rotate(rotate, Image.Resampling.BICUBIC, center=center)
        painter.tone = painter.tone.rotate(rotate, Image.Resampling.BICUBIC, center=center)
    return painter.done(*canvas)


# --- images --------------------------------------------------------------------------------------

ART_SIZE = (256, 192)
# the launcher's gradient fades to the background until this line (DsRunnerSubBackground.cpp)
GRADIENT_BAND_START = 118


def gradient_level(y: int, top: float = 128.0) -> float:
    """Template level of the launcher's top screen gradient at line y (eased, 128 = console color)."""
    if y >= GRADIENT_BAND_START:
        return 0.0
    t = y / GRADIENT_BAND_START
    return top * (1 - t * t)


def top_art(key: str) -> Image.Image:
    """256x192: the launcher's gradient, a soft light, fine diagonal lines and the large glyph on the right."""
    width, height = ART_SIZE
    levels = [[gradient_level(y)] * width for y in range(height)]
    # soft light behind the glyph
    cx, cy, radius = 164, 66, 120
    for y in range(height):
        fade = 1.0 if y < 70 else max(0.0, 1 - (y - 70) / 60)
        for x in range(width):
            d = math.hypot((x - cx) / 1.25, y - cy) / radius
            if d < 1:
                levels[y][x] += 34 * (1 - d) ** 2 * fade
    # fine diagonal lines in the upper part, they fade out towards the text
    for y in range(height):
        fade = max(0.0, 1 - y / 110)
        for x in range(width):
            if (x + y) % 12 < 2:
                levels[y][x] += 7 * fade
    base = Image.new("L", ART_SIZE)
    base.putdata([max(0, min(255, round(v))) for row in levels for v in row])

    # the glyph, big and slightly tilted, a bit darker than in the icons so it stays calm; it keeps
    # clear of the status bar (top right, lines 4 to 20)
    tone, alpha = render_glyph(key, (98, 26, 132, 90), ART_SIZE, True, rotate=-8)
    tone = tone.point(lambda v: round(v * 0.78 + 40 * 0.22) if v else 0)
    shadow = alpha.filter(ImageFilter.GaussianBlur(6)).point(lambda v: v * 60 // 255)
    base = Image.composite(Image.new("L", ART_SIZE, 0), base, shadow)
    return Image.composite(tone, base, alpha)


def cover(key: str) -> Image.Image:
    """128x96 (the cover flow shows the left 106x96): gradient, a soft light and the glyph in the middle."""
    base = Image.new("L", (128, 96))
    base.putdata([round(36 + 64 * (1 - y / 95) ** 1.5) for y in range(96) for _ in range(128)])
    light = Image.new("L", (128, 96), 0)
    ImageDraw.Draw(light).ellipse((13, 8, 93, 88), fill=26)
    base = ImageChops.add(base, light.filter(ImageFilter.GaussianBlur(14)))
    tone, alpha = render_glyph(key, (14, 14, 78, 64), (128, 96), True)
    return Image.composite(tone, base, alpha)


def tile(key: str) -> tuple[Image.Image, Image.Image]:
    """64x64 with transparency: the glyph in the upper part, the launcher puts the label below it."""
    return render_glyph(key, (3, 2, 58, 44), (64, 64), True, max_scale=2.4)


ICON_BOX = (1, 4, 30, 24)


def icon(key: str) -> tuple[Image.Image, Image.Image]:
    """32x32 with transparency: crisp, one unit is one pixel (the glyphs are at most 30x24)."""
    (gw, gh), draw = GLYPHS[key]
    ox = (32 - gw) // 2
    oy = ICON_BOX[1] + (ICON_BOX[3] - gh) // 2
    painter = Painter(32, 32, ox, oy, 1.0)
    draw(painter, False)
    return painter.done_crisp(32, 32)


# --- encoding ------------------------------------------------------------------------------------

def _bmp(width: int, height: int, bpp: int, pixels: bytes, palette: list[int]) -> bytes:
    """Bottom-up BMP with a gray palette; pixels: one index per pixel, rows from the top."""
    colors = 1 << bpp
    palette = (palette + [0] * colors)[:colors]
    palette_bytes = b"".join(struct.pack("<BBBB", v, v, v, 0) for v in palette)
    rows = []
    for y in range(height):
        row = pixels[y * width:(y + 1) * width]
        if bpp == 4:
            row = bytes((row[x] << 4) | row[x + 1] for x in range(0, width, 2))
        rows.append(row)
    pixel_bytes = b"".join(reversed(rows))
    offset = 14 + 40 + len(palette_bytes)
    file_header = struct.pack("<2sIHHI", b"BM", offset + len(pixel_bytes), SIGNATURE[0], SIGNATURE[1], offset)
    info_header = struct.pack("<IiiHHIIiiII", 40, width, height, 1, bpp, 0, len(pixel_bytes), 2835, 2835, colors, 0)
    return file_header + info_header + palette_bytes + pixel_bytes


def encode_opaque(image: Image.Image) -> bytes:
    """8 bpp, index 0 unused: the gray levels 0..255 squeezed into 255 palette entries."""
    data = image.tobytes()
    levels = sorted(set(data))
    if len(levels) > 255:
        data = bytes(round(v * 254 / 255) for v in data)
        levels = sorted(set(data))
    index = {level: i + 1 for i, level in enumerate(levels)}
    return _bmp(image.width, image.height, 8, bytes(index[v] for v in data), [0] + levels)


def encode_transparent(tone: Image.Image, alpha: Image.Image, bpp: int) -> bytes:
    """Index 0 for pixels with less than half coverage, the others get their gray level."""
    tones = tone.tobytes()
    coverage = alpha.tobytes()
    max_levels = (1 << bpp) - 1
    used = sorted({t for t, a in zip(tones, coverage) if a >= 128})
    if len(used) > max_levels:
        # keep the levels of the drawing, edges inside the glyph snap to the closest one
        drawing = sorted({BODY, BODY2, FACE, ACCENT, SCREEN, SCREEN_IN, WHITE_TONE} & set(used)) or used
        used = drawing[:max_levels]
    index = {}
    for level in range(256):
        index[level] = 1 + min(range(len(used)), key=lambda i: abs(used[i] - level))
    pixels = bytes(index[t] if a >= 128 else 0 for t, a in zip(tones, coverage))
    return _bmp(tone.width, tone.height, bpp, pixels, [0] + used)


# --- preview -------------------------------------------------------------------------------------

def tint(level: int, color) -> tuple[int, int, int]:
    """Mirror of DsRunnerConsoleArt::Tint."""
    if level <= 128:
        return tuple((BG_TOP[i] * (128 - level) + color[i] * level + 64) // 128 for i in range(3))
    t = level - 128
    return tuple((color[i] * (127 - t) + WHITE[i] * t + 63) // 127 for i in range(3))


def decode(data: bytes) -> tuple[Image.Image, Image.Image]:
    """BMP from above -> (gray levels, mask of the transparent pixels)."""
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bpp = struct.unpack_from("<H", data, 28)[0]
    colors = 1 << bpp
    palette = [data[54 + i * 4] for i in range(colors)]
    stride = width * bpp // 8
    levels = Image.new("L", (width, height))
    mask = Image.new("L", (width, height))
    for y in range(height):
        row = data[offset + (height - 1 - y) * stride:offset + (height - y) * stride]
        for x in range(width):
            value = row[x] if bpp == 8 else (row[x // 2] >> (4 if x % 2 == 0 else 0)) & 15
            levels.putpixel((x, y), palette[value])
            mask.putpixel((x, y), 255 if value else 0)
    return levels, mask


def colorize(levels: Image.Image, color) -> Image.Image:
    lut = [tint(v, color) for v in range(256)]
    return Image.merge("RGB", [levels.point([c[i] for c in lut]) for i in range(3)])


def preview(files: dict[str, dict[str, bytes]], path: Path) -> None:
    """One row per key: top screen, cover, tile on its tinted body, icon on its tinted body (2x)."""
    colors = {console.folder_name: console for console in CONSOLES}
    key_color = {"FAV": ((196, 146, 40), (150, 108, 24)), "SEARCH": ((40, 150, 196), (28, 116, 156)),
                 "FOLDER": (UNKNOWN_TILE, UNKNOWN_HERO),
                 "CART": ((192, 128, 32), (156, 104, 26))}
    for folder, key in ART_KEYS.items():
        if folder in colors and key not in key_color:
            key_color[key] = (colors[folder].tile, colors[folder].hero)
    rows = []
    for key, images in files.items():
        tile_color, hero_color = key_color[key]
        row = Image.new("RGB", (256 + 8 + 106 + 8 + 76 + 8 + 55, 192), BG_TOP)
        top, _ = decode(images["top"])
        row.paste(colorize(top, hero_color), (0, 0))
        cov, _ = decode(images["cover"])
        row.paste(colorize(cov, tile_color).crop((0, 0, 106, 96)), (264, 0))
        body = tuple((tile_color[i] * 5 + BG_TOP[i] * 3) // 8 for i in range(3))
        x = 264 + 106 + 8
        tile_body = Image.new("RGB", (76, 70), body)
        levels, mask = decode(images["tile"])
        tile_body.paste(colorize(levels, tile_color), (6, 3), mask)
        row.paste(tile_body, (x, 0))
        icon_body = Image.new("RGB", (55, 44), body)
        levels, mask = decode(images["icon"])
        icon_body.paste(colorize(levels, tile_color), (11, 1), mask)
        row.paste(icon_body, (x + 84, 0))
        row.paste(icon_body.resize((110, 88), Image.Resampling.NEAREST), (x, 100))
        rows.append(row)
    sheet = Image.new("RGB", (rows[0].width, len(rows) * 196), BG_TOP)
    for i, row in enumerate(rows):
        sheet.paste(row, (0, i * 196))
    sheet.save(path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--preview", type=Path, help="also write a colored preview sheet (PNG)")
    parser.add_argument("--out", type=Path, default=OUT_DIR, help="output folder")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    files: dict[str, dict[str, bytes]] = {}
    for key in GLYPHS:
        images = {
            "top": encode_opaque(top_art(key)),
            "cover": encode_opaque(cover(key)),
            "tile": encode_transparent(*tile(key), 8),
            "icon": encode_transparent(*icon(key), 4),
        }
        suffixes = {"top": "", "cover": "-cover", "tile": "-tile", "icon": "-icon"}
        for kind, data in images.items():
            (args.out / f"{key}{suffixes[kind]}.bmp").write_bytes(data)
        files[key] = images
    if args.preview:
        preview(files, args.preview)
    print(f"{len(files) * 4} files in {args.out}")


if __name__ == "__main__":
    main()
