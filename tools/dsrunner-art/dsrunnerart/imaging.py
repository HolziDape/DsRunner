"""Image conversion to the launcher's artwork and tile formats and generated artworks.

Artworks (docs/CONFIG.md, "Artworks"): Windows BMP, 256x192, 8 bpp with palette, BI_RGB.
Tile images (docs/CONFIG.md, "Kachel-Bilder"): the same, but 64x64.
The DS shows palette index 0 as transparent, so the written files never use it.
"""
from __future__ import annotations

import io
import struct
from dataclasses import dataclass, field
from typing import Optional

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps

WIDTH = 256
HEIGHT = 192
TILE_SIZE = 64
# the launcher darkens the art from line 92 on and draws the title at line 122
VISIBLE_HEIGHT = 92
MAX_COLORS = 255
BACKGROUND = (15, 17, 23)  # #0F1117, the theme background
ACCENT = (76, 194, 255)    # #4CC2FF
# written into the reserved fields of the BMP file header: marks files this tool wrote, so they are
# recognized even when the manifest got lost (the launcher ignores these fields)
SIGNATURE = (0x5244, 1)  # "DR", format 1

# NDS and (western) GBA box fronts carry a band with the console name on the left: system ->
# (share of the width, aspect range of the boxes that have it), measured on libretro boxarts
SPINES = {"nds": (0.135, 1.0, 1.3), "gba": (0.145, 0.95, 1.05)}
# a screen half counts as empty when one color covers more than this share of it
FLAT_SHARE = 0.6

_RESAMPLE = Image.Resampling.LANCZOS


# --- fitting ---------------------------------------------------------------------------------

def _flatten(image: Image.Image) -> Image.Image:
    """RGB image, transparent parts on the theme background."""
    if image.mode in ("RGBA", "LA", "PA") or (image.mode == "P" and "transparency" in image.info):
        rgba = image.convert("RGBA")
        background = Image.new("RGBA", rgba.size, BACKGROUND + (255,))
        background.alpha_composite(rgba)
        return background.convert("RGB")
    if image.mode in ("I", "I;16", "I;16B", "I;16L", "F"):
        # 16 bit grayscale PNGs: scale down to 8 bit first (a direct conversion clips to white)
        image = image.point(lambda value: value / 256).convert("L")
    return image.convert("RGB")


def is_dual_screen(image: Image.Image) -> bool:
    """NDS screenshots in libretro-thumbnails show both screens stacked (256x384)."""
    width, height = image.size
    return width > 0 and abs(height / width - 1.5) < 0.04


def is_side_by_side(image: Image.Image) -> bool:
    """Games played with the DS turned sideways (Brain Age ...) show both screens next to each other (384x256)."""
    width, height = image.size
    return height > 0 and abs(width / height - 1.5) < 0.04


def flat_share(image: Image.Image) -> float:
    """Share of the most common color after a rough color reduction: close to 1 for empty screens."""
    small = image.convert("RGB").resize((64, 48), Image.Resampling.BILINEAR)
    reduced = small.quantize(32, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    counts = [count for count, _ in reduced.getcolors(64) or []]
    return max(counts) / sum(counts) if counts else 1.0


def pick_screen(image: Image.Image, system_key: Optional[str] = None) -> tuple[Image.Image, bool]:
    """The screen to show from a DS title or snap that has both screens in one image.

    Returns (image, portrait). Heuristic (docs/ART-TOOL.md, "Titelbild oder Cover"):
    - stacked screens (256x384): the top screen, unless it is (nearly) empty and the bottom screen is
      not. Title screens show the logo on the top screen in almost every game.
    - side by side (384x256, NDS only, e.g. Brain Age): the left screen, upright.
    """
    width, height = image.size
    if is_dual_screen(image):
        top = image.crop((0, 0, width, height // 2))
        bottom = image.crop((0, height // 2, width, height))
        if flat_share(top) > FLAT_SHARE and flat_share(bottom) <= FLAT_SHARE:
            return bottom, False
        return top, False
    if system_key == "nds" and is_side_by_side(image):
        return image.crop((0, 0, width // 2, height)), True
    return image, False


def trim_spine(image: Image.Image, system_key: Optional[str]) -> Image.Image:
    """Cuts the "NINTENDO DS" / "GAME BOY ADVANCE" band off the left of a box front."""
    spine = SPINES.get(system_key or "")
    width, height = image.size
    if spine and height and spine[1] <= width / height <= spine[2]:
        return image.crop((round(width * spine[0]), 0, width, height))
    return image


def cover_fit(image: Image.Image, anchor_y: float = 0.35, size: tuple[int, int] = (WIDTH, HEIGHT)) -> Image.Image:
    """Scales to fill `size` and crops; anchor_y 0 keeps the top, 0.5 crops evenly."""
    target_width, target_height = size
    width, height = image.size
    scale = max(target_width / width, target_height / height)
    new_size = (max(target_width, round(width * scale)), max(target_height, round(height * scale)))
    resized = image.resize(new_size, _RESAMPLE)
    left = (new_size[0] - target_width) // 2
    top = round((new_size[1] - target_height) * anchor_y)
    return resized.crop((left, top, left + target_width, top + target_height))


def blurred_fit(image: Image.Image) -> Image.Image:
    """The whole image at full height, in front of a blurred and darkened copy that fills the rest."""
    background = cover_fit(image, 0.5).filter(ImageFilter.GaussianBlur(14))
    background = Image.blend(background, Image.new("RGB", background.size, BACKGROUND), 0.45)
    width, height = image.size
    scale = HEIGHT / height
    front = image.resize((max(1, round(width * scale)), HEIGHT), _RESAMPLE)
    background.paste(front, ((WIDTH - front.width) // 2, 0))
    return background


def to_hero(image: Image.Image, art_type: str, system_key: Optional[str] = None) -> Image.Image:
    """Turns a downloaded or user image into a 256x192 RGB hero image.

    art_type: "title" / "snap" (screens: fill the screen), "boxart" (keep the top of the box in the
    visible upper part) or "custom" (user images: fill, centered a bit towards the top).
    """
    image = _flatten(image)
    # the aspect of the whole box decides how it is fitted (before the band is cut off)
    aspect = image.width / image.height
    if art_type in ("title", "snap"):
        image, portrait = pick_screen(image, system_key)
        if portrait:
            return blurred_fit(image)
    elif art_type == "boxart":
        image = trim_spine(image, system_key)
    width, height = image.size
    if (width, height) == (WIDTH, HEIGHT):
        return image
    if art_type == "boxart":
        if aspect < 0.9:
            # tall boxes (Mega Drive, US SNES ...): show the whole box
            return blurred_fit(image)
        # square or wide boxes: fill the width, the logo at the top stays visible
        return cover_fit(image, 0.0)
    if art_type == "custom":
        return cover_fit(image, 0.3)
    return cover_fit(image, 0.35)


def to_tile(image: Image.Image, art_type: str, system_key: Optional[str] = None) -> Image.Image:
    """Turns a boxart, screen or user image into a 64x64 RGB tile image (cover fit)."""
    image = _flatten(image)
    anchor = 0.5
    if art_type in ("title", "snap"):
        image, portrait = pick_screen(image, system_key)
        # landscape screens lose their sides, upright screens keep the upper part with the logo
        anchor = 0.3 if portrait else 0.5
    elif art_type == "boxart":
        image = trim_spine(image, system_key)
        # the logo sits at the top of tall boxes
        anchor = 0.3
    return cover_fit(image, anchor, (TILE_SIZE, TILE_SIZE))


def icon_tile(icon: Image.Image) -> Image.Image:
    """Tile image from the 32x32 NDS banner icon: doubled without smoothing (pixel art) on the dark background."""
    icon = icon.convert("RGBA")
    scale = max(1, TILE_SIZE // max(icon.size))
    big = icon.resize((icon.width * scale, icon.height * scale), Image.Resampling.NEAREST)
    base = Image.new("RGBA", (TILE_SIZE, TILE_SIZE), BACKGROUND + (255,))
    base.alpha_composite(big, ((TILE_SIZE - big.width) // 2, (TILE_SIZE - big.height) // 2))
    return base.convert("RGB")


# --- palette reduction and BMP ---------------------------------------------------------------

def _to_555(value: int) -> int:
    """8 bit channel value as the DS shows it (5 bits, expanded back to 8 bits)."""
    five = value >> 3
    return (five << 3) | (five >> 2)


def quantize(image: Image.Image, colors: int = MAX_COLORS, dither: bool = True) -> tuple[bytes, list[tuple[int, int, int]]]:
    """Reduces an RGB image to at most `colors` DS colors with Floyd-Steinberg dithering.

    Returns the pixel indices (0-based, row by row, top to bottom) and the palette.
    """
    rgb = image.convert("RGB")
    # build the palette from the colors the DS can show, then dither against exactly those colors
    lut = [_to_555(value) for value in range(256)]
    reduced = rgb.point(lut * 3)
    palette_image = reduced.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    used = sorted(index for _, index in (palette_image.getcolors(256) or []))
    raw_palette = palette_image.getpalette() or []
    palette = [tuple(_to_555(c) for c in raw_palette[i * 3:i * 3 + 3]) for i in used if i * 3 + 3 <= len(raw_palette)]
    palette = list(dict.fromkeys(palette))[:colors] or [(0, 0, 0)]
    count = len(palette)
    # pad with copies of the first color: pixels that end up there are mapped back to entry 0
    padded = palette + [palette[0]] * (256 - count)
    target = Image.new("P", (1, 1))
    target.putpalette([c for color in padded for c in color])
    result = rgb.quantize(palette=target, dither=Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE)
    pixels = result.tobytes()
    if count < 256:
        remap = bytes(i if i < count else 0 for i in range(256))
        pixels = pixels.translate(remap)
    return pixels, palette


def encode_bmp(pixels: bytes, palette: list[tuple[int, int, int]], skip_index_zero: bool = True,
               size: tuple[int, int] = (WIDTH, HEIGHT)) -> bytes:
    """Writes an 8 bpp bottom-up BMP. With skip_index_zero all indices move up by one (max 255 colors)."""
    width, height = size
    if width % 4:
        raise ValueError("the width must be a multiple of 4 (no row padding)")
    if len(pixels) != width * height:
        raise ValueError("wrong pixel count")
    if skip_index_zero:
        if len(palette) > 255:
            raise ValueError("at most 255 colors when index 0 stays unused")
        pixels = pixels.translate(bytes(min(i + 1, 255) for i in range(256)))
        palette = [(0, 0, 0)] + list(palette)
    palette = list(palette) + [(0, 0, 0)] * (256 - len(palette))
    palette_bytes = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette)
    rows = [pixels[y * width:(y + 1) * width] for y in range(height)]
    pixel_bytes = b"".join(reversed(rows))
    offset = 14 + 40 + len(palette_bytes)
    file_size = offset + len(pixel_bytes)
    file_header = struct.pack("<2sIHHI", b"BM", file_size, SIGNATURE[0], SIGNATURE[1], offset)
    info_header = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 8, 0, len(pixel_bytes), 2835, 2835, 256, 0)
    return file_header + info_header + palette_bytes + pixel_bytes


def to_ds_bmp(image: Image.Image, dither: bool = True) -> bytes:
    """RGB image (256x192 artwork or 64x64 tile image) -> BMP bytes in the launcher's format."""
    pixels, palette = quantize(image, MAX_COLORS, dither)
    return encode_bmp(pixels, palette, size=image.size)


def is_tool_bmp(data: bytes) -> bool:
    """True for BMPs this tool wrote (signature in the reserved fields of the file header)."""
    return len(data) >= 14 and data[:2] == b"BM" and struct.unpack_from("<HH", data, 6) == SIGNATURE


# --- validation ------------------------------------------------------------------------------

@dataclass
class Validation:
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not self.errors


def validate_bmp(data: bytes, size: tuple[int, int] = (WIDTH, HEIGHT)) -> Validation:
    """Checks a file like DsRunnerTopArt::Load does (and warns about palette index 0).

    size: (256, 192) for artworks, (64, 64) for tile images."""
    expected_width, expected_height = size
    result = Validation()
    if len(data) < 54 or data[:2] != b"BM":
        result.errors.append("keine BMP-Datei")
        return result
    pixel_offset, dib_size = struct.unpack_from("<II", data, 0x0A)
    width, height = struct.unpack_from("<Ii", data, 0x12)
    bpp = struct.unpack_from("<H", data, 0x1C)[0]
    compression = struct.unpack_from("<I", data, 0x1E)[0]
    colors = struct.unpack_from("<I", data, 0x2E)[0]
    if colors == 0 or colors > 256:
        colors = 256
    if dib_size < 40:
        result.errors.append(f"unbekannter BMP-Kopf ({dib_size} Bytes)")
    if width != expected_width or height not in (expected_height, -expected_height):
        result.errors.append(f"Größe {width}x{abs(height)} statt {expected_width}x{expected_height}")
    if bpp != 8:
        result.errors.append(f"{bpp} bit pro Pixel statt 8")
    if compression != 0:
        result.errors.append("komprimiert (nur BI_RGB geht)")
    if result.errors:
        return result
    if 14 + dib_size + colors * 4 > len(data):
        result.errors.append("Palette unvollständig")
    pixel_count = expected_width * expected_height
    if pixel_offset + pixel_count > len(data):
        result.errors.append("Bilddaten unvollständig")
    if result.errors:
        return result
    pixels = data[pixel_offset:pixel_offset + pixel_count]
    if 0 in pixels:
        result.warnings.append("Palettenfarbe 0 wird benutzt (der DS ersetzt sie durch eine ähnliche Farbe)")
    if max(pixels) >= colors:
        result.warnings.append("Pixel verweisen auf Farben außerhalb der Palette")
    return result


def decode_bmp(data: bytes) -> Image.Image:
    """Opens an artwork BMP (for previews and tests)."""
    return Image.open(io.BytesIO(data)).convert("RGB")


# --- generated artworks ----------------------------------------------------------------------

_FONT_CANDIDATES = (
    "segoeuib.ttf", "arialbd.ttf", "seguisb.ttf", "Arial Bold.ttf", "DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "/Library/Fonts/Arial Bold.ttf",
)
_font_cache: dict[int, ImageFont.ImageFont] = {}


def load_font(size: int):
    font = _font_cache.get(size)
    if font:
        return font
    for name in _FONT_CANDIDATES:
        try:
            font = ImageFont.truetype(name, size)
            break
        except OSError:
            continue
    else:
        try:
            font = ImageFont.load_default(size)
        except (TypeError, OSError):
            font = ImageFont.load_default()
    _font_cache[size] = font
    return font


def _mix(a, b, t: float):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def gradient(top_color, bottom_color=BACKGROUND, size=(WIDTH, HEIGHT)) -> Image.Image:
    """Vertical gradient like the launcher's fallback (console color at the top, dark at the bottom)."""
    width, height = size
    column = Image.new("RGB", (1, height))
    for y in range(height):
        column.putpixel((0, y), _mix(top_color, bottom_color, min(1.0, y / (height * 0.85))))
    return column.resize(size, Image.Resampling.NEAREST)


def _lighten(color, amount: float):
    return _mix(color, (255, 255, 255), amount)


def _fit_text(draw: ImageDraw.ImageDraw, text: str, max_width: int, max_size: int, min_size: int = 14):
    size = max_size
    while size > min_size:
        font = load_font(size)
        if draw.textlength(text, font=font) <= max_width:
            return font, text
        size -= 4
    font = load_font(min_size)
    while text and draw.textlength(text + "…", font=font) > max_width:
        text = text[:-1]
    return font, text + "…"


def console_art(label: str, hero_color, tile_color=None) -> Image.Image:
    """Default artwork of a console folder: gradient in the console color with a large label.

    The launcher draws the full console name at line 122, so the label stays in the upper part.
    """
    tile_color = tile_color or hero_color
    image = gradient(hero_color)
    overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    # soft circles in the tile color for some depth
    draw.ellipse((120, -90, 360, 150), fill=_lighten(tile_color, 0.18) + (70,))
    draw.ellipse((170, -40, 300, 90), fill=_lighten(tile_color, 0.35) + (60,))
    overlay = overlay.filter(ImageFilter.GaussianBlur(2))
    image = Image.alpha_composite(image.convert("RGBA"), overlay)

    draw = ImageDraw.Draw(image)
    font, text = _fit_text(draw, label, 224, 76 if len(label) <= 4 else 44)
    box = draw.textbbox((0, 0), text, font=font)
    text_height = box[3] - box[1]
    x = 16 - box[0]
    y = max(20, 76 - text_height) - box[1]
    # shadow, then the label
    draw.text((x + 2, y + 3), text, font=font, fill=(0, 0, 0, 90))
    draw.text((x, y), text, font=font, fill=(255, 255, 255, 240))
    draw.rounded_rectangle((16, 84, 52, 88), radius=2, fill=ACCENT + (255,))
    return image.convert("RGB")


def icon_art(icon: Image.Image, hero_color) -> Image.Image:
    """Fallback artwork from the 32x32 NDS banner icon: the icon enlarged on a blurred backdrop."""
    icon = icon.convert("RGBA")
    backdrop = icon.resize((WIDTH, HEIGHT), Image.Resampling.BILINEAR).filter(ImageFilter.GaussianBlur(18))
    base = gradient(hero_color).convert("RGBA")
    backdrop.putalpha(150)
    base.alpha_composite(backdrop)
    base = Image.blend(base, Image.new("RGBA", base.size, BACKGROUND + (255,)), 0.25)
    big = icon.resize((96, 96), Image.Resampling.NEAREST)
    shadow = Image.new("RGBA", big.size, (0, 0, 0, 0))
    shadow.putalpha(big.getchannel("A").point(lambda a: 110 if a else 0))
    shadow = ImageOps.expand(shadow, 8, fill=(0, 0, 0, 0)).filter(ImageFilter.GaussianBlur(5))
    x, y = (WIDTH - 96) // 2, 2
    base.alpha_composite(shadow, (x - 8 + 3, y - 8 + 4))
    base.alpha_composite(big, (x, y))
    return base.convert("RGB")


def open_image(data: bytes) -> Optional[Image.Image]:
    """Opens image bytes; None when they are no image."""
    try:
        image = Image.open(io.BytesIO(data))
        image.load()
        return image
    except Exception:  # noqa: BLE001 - Pillow raises many different errors for broken files
        return None
