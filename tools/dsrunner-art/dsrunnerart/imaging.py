"""Image conversion to the launcher's artwork format and generated artworks.

Format (docs/CONFIG.md, "Artworks"): Windows BMP, 256x192, 8 bpp with palette, BI_RGB. The DS shows
palette index 0 as transparent, so the written files never use it.
"""
from __future__ import annotations

import io
import struct
from dataclasses import dataclass, field
from typing import Optional

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps

WIDTH = 256
HEIGHT = 192
# the launcher darkens the art from line 92 on and draws the title at line 122
VISIBLE_HEIGHT = 92
MAX_COLORS = 255
BACKGROUND = (15, 17, 23)  # #0F1117, the theme background
ACCENT = (76, 194, 255)    # #4CC2FF

_RESAMPLE = Image.Resampling.LANCZOS


# --- fitting ---------------------------------------------------------------------------------

def _flatten(image: Image.Image) -> Image.Image:
    """RGB image, transparent parts on the theme background."""
    if image.mode in ("RGBA", "LA") or (image.mode == "P" and "transparency" in image.info):
        rgba = image.convert("RGBA")
        background = Image.new("RGBA", rgba.size, BACKGROUND + (255,))
        background.alpha_composite(rgba)
        return background.convert("RGB")
    return image.convert("RGB")


def is_dual_screen(image: Image.Image) -> bool:
    """NDS screenshots in libretro-thumbnails show both screens stacked (256x384)."""
    width, height = image.size
    return width > 0 and abs(height / width - 1.5) < 0.04


def cover_fit(image: Image.Image, anchor_y: float = 0.35) -> Image.Image:
    """Scales to fill 256x192 and crops; anchor_y 0 keeps the top, 0.5 crops evenly."""
    width, height = image.size
    scale = max(WIDTH / width, HEIGHT / height)
    new_size = (max(WIDTH, round(width * scale)), max(HEIGHT, round(height * scale)))
    resized = image.resize(new_size, _RESAMPLE)
    left = (new_size[0] - WIDTH) // 2
    top = round((new_size[1] - HEIGHT) * anchor_y)
    return resized.crop((left, top, left + WIDTH, top + HEIGHT))


def blurred_fit(image: Image.Image) -> Image.Image:
    """The whole image at full height, in front of a blurred and darkened copy that fills the rest."""
    background = cover_fit(image, 0.5).filter(ImageFilter.GaussianBlur(14))
    background = Image.blend(background, Image.new("RGB", background.size, BACKGROUND), 0.45)
    width, height = image.size
    scale = HEIGHT / height
    front = image.resize((max(1, round(width * scale)), HEIGHT), _RESAMPLE)
    background.paste(front, ((WIDTH - front.width) // 2, 0))
    return background


def to_hero(image: Image.Image, art_type: str) -> Image.Image:
    """Turns a downloaded or user image into a 256x192 RGB hero image.

    art_type: "title" / "snap" (screens: fill the screen), "boxart" (keep the top of the box in the
    visible upper part) or "custom" (user images: fill, centered a bit towards the top).
    """
    image = _flatten(image)
    width, height = image.size
    if art_type in ("title", "snap") and is_dual_screen(image):
        # top screen of a stacked DS screenshot, scaled if the snap is larger than 256x384
        image = image.crop((0, 0, width, height // 2))
        width, height = image.size
    if (width, height) == (WIDTH, HEIGHT):
        return image
    if art_type == "boxart":
        aspect = width / height
        if aspect < 0.9:
            # tall boxes (Mega Drive, US SNES ...): show the whole box
            return blurred_fit(image)
        # square or wide boxes: fill the width, the logo at the top stays visible
        return cover_fit(image, 0.0)
    if art_type == "custom":
        return cover_fit(image, 0.3)
    return cover_fit(image, 0.35)


# --- palette reduction and BMP ---------------------------------------------------------------

def _to_555(value: int) -> int:
    """8 bit channel value as the DS shows it (5 bits, expanded back to 8 bits)."""
    five = value >> 3
    return (five << 3) | (five >> 2)


def quantize(image: Image.Image, colors: int = MAX_COLORS, dither: bool = True) -> tuple[bytes, list[tuple[int, int, int]]]:
    """Reduces an RGB image to at most `colors` DS colors with Floyd-Steinberg dithering.

    Returns the pixel indices (0-based, row by row, top to bottom) and the palette.
    """
    if image.size != (WIDTH, HEIGHT):
        raise ValueError(f"expected {WIDTH}x{HEIGHT}, got {image.size[0]}x{image.size[1]}")
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


def encode_bmp(pixels: bytes, palette: list[tuple[int, int, int]], skip_index_zero: bool = True) -> bytes:
    """Writes an 8 bpp bottom-up BMP. With skip_index_zero all indices move up by one (max 255 colors)."""
    if len(pixels) != WIDTH * HEIGHT:
        raise ValueError("wrong pixel count")
    if skip_index_zero:
        if len(palette) > 255:
            raise ValueError("at most 255 colors when index 0 stays unused")
        pixels = pixels.translate(bytes(min(i + 1, 255) for i in range(256)))
        palette = [(0, 0, 0)] + list(palette)
    palette = list(palette) + [(0, 0, 0)] * (256 - len(palette))
    palette_bytes = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette)
    rows = [pixels[y * WIDTH:(y + 1) * WIDTH] for y in range(HEIGHT)]
    pixel_bytes = b"".join(reversed(rows))
    offset = 14 + 40 + len(palette_bytes)
    file_size = offset + len(pixel_bytes)
    file_header = struct.pack("<2sIHHI", b"BM", file_size, 0, 0, offset)
    info_header = struct.pack("<IiiHHIIiiII", 40, WIDTH, HEIGHT, 1, 8, 0, len(pixel_bytes), 2835, 2835, 256, 0)
    return file_header + info_header + palette_bytes + pixel_bytes


def to_ds_bmp(image: Image.Image, dither: bool = True) -> bytes:
    """256x192 RGB image -> artwork BMP bytes."""
    pixels, palette = quantize(image, MAX_COLORS, dither)
    return encode_bmp(pixels, palette)


# --- validation ------------------------------------------------------------------------------

@dataclass
class Validation:
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not self.errors


def validate_bmp(data: bytes) -> Validation:
    """Checks a file like DsRunnerTopArt::Load does (and warns about palette index 0)."""
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
    if width != WIDTH or height not in (HEIGHT, -HEIGHT):
        result.errors.append(f"Größe {width}x{abs(height)} statt {WIDTH}x{HEIGHT}")
    if bpp != 8:
        result.errors.append(f"{bpp} bit pro Pixel statt 8")
    if compression != 0:
        result.errors.append("komprimiert (nur BI_RGB geht)")
    if result.errors:
        return result
    if 14 + dib_size + colors * 4 > len(data):
        result.errors.append("Palette unvollständig")
    if pixel_offset + WIDTH * HEIGHT > len(data):
        result.errors.append("Bilddaten unvollständig")
    if result.errors:
        return result
    pixels = data[pixel_offset:pixel_offset + WIDTH * HEIGHT]
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
