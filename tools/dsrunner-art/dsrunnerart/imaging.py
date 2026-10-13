"""Image conversion to the launcher's artwork, tile, cover and icon formats and generated artworks.

Artworks (docs/CONFIG.md, "Artworks"): Windows BMP, 256x192, 8 bpp with palette, BI_RGB.
Tile images (docs/CONFIG.md, "Kachel-Bilder"): the same, but 64x64, in a simplified style (at most
24 colors, no dithering).
The DS shows palette index 0 as transparent, so these files never use it.
Covers for the cover flow (docs/Customization.md, "Cover format"): 128x96, 8 bpp, the picture in the
left 106x96 pixels, the rest is padding. Realistic: 255 colors with dithering.
Icons for the 4x3 grid (docs/Customization.md, "Icon format"): 32x32, 4 bpp, 16 colors, palette
index 0 is transparent. Minimalist: at most 15 colors, no dithering, single pixels cleaned up.
"""
from __future__ import annotations

import io
import struct
from dataclasses import dataclass, field
from typing import Optional

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont, ImageOps

WIDTH = 256
HEIGHT = 192
TILE_SIZE = 64
# tile images: few colors and no dithering, so they look clean at 64 px
TILE_COLORS = 24
# covers: the texture is 128x96, the cover flow shows the left 106x96 pixels
COVER_WIDTH = 128
COVER_HEIGHT = 96
COVER_PICTURE_WIDTH = 106
COVER_BACKGROUND = (18, 20, 28)  # #12141C, around the picture when its aspect differs
# icons: 32x32, 4 bpp; index 0 is transparent, so 15 colors are left
ICON_SIZE = 32
ICON_COLORS = 15
# transparent rounded corners of the icons (radius in pixels)
ICON_CORNER_RADIUS = 4
# the launcher darkens the art from line 92 on and draws the title at line 122
VISIBLE_HEIGHT = 92
MAX_COLORS = 255
BACKGROUND = (15, 17, 23)  # #0F1117, the theme background
ACCENT = (76, 194, 255)    # #4CC2FF
# written into the reserved fields of the BMP file header: marks files this tool wrote, so they are
# recognized even when the manifest got lost (the launcher ignores these fields)
SIGNATURE = (0x5244, 1)  # "DR", format 1

# NDS and (western) GBA box fronts carry a band with the console name on the left: system ->
# (share of the width, aspect range of the boxes that have it), measured on libretro boxarts.
# GBA boxes have a silver frame next to the band, the picture starts at about 22 %
SPINES = {"nds": (0.135, 1.0, 1.3), "gba": (0.225, 0.95, 1.05)}
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
        # zoomed in on logo and main character: rating labels and publisher logos in the corners
        # are only noise at 64 pixels
        if image.width >= image.height * 0.8:
            image = salient_square(image, 0.82, (0.5, 0.3))
        # the logo sits at the top of tall boxes
        anchor = 0.3
    tile = cover_fit(image, anchor, (TILE_SIZE, TILE_SIZE))
    # less detail: half of a 3x3 median smooths fine texture but keeps edges and letters
    tile = Image.blend(tile, tile.filter(ImageFilter.MedianFilter(3)), 0.5)
    return simplify(tile)


def simplify(image: Image.Image, contrast: float = 1.08, color: float = 1.12, sharpen: int = 60) -> Image.Image:
    """A bit more contrast and color and slightly sharper edges: the small pictures read better
    once their colors are reduced (without dithering)."""
    image = ImageEnhance.Contrast(image).enhance(contrast)
    image = ImageEnhance.Color(image).enhance(color)
    if sharpen:
        image = image.filter(ImageFilter.UnsharpMask(radius=1, percent=sharpen, threshold=2))
    return image


def to_cover(image: Image.Image, art_type: str, system_key: Optional[str] = None) -> Image.Image:
    """Turns a boxart, screen or user image into a 128x96 RGB cover for the cover flow.

    The whole picture is fitted into the left 106x96 pixels (pictures with nearly the same aspect
    fill it, the others get dark bars), the 22 pixels on the right are black padding."""
    image = _flatten(image)
    if art_type in ("title", "snap"):
        image, _ = pick_screen(image, system_key)
    width, height = image.size
    area = (COVER_PICTURE_WIDTH, COVER_HEIGHT)
    aspect = width / height
    target = area[0] / area[1]
    if abs(aspect / target - 1) <= 0.06:
        picture = cover_fit(image, 0.5, area)
    else:
        scale = min(area[0] / width, area[1] / height)
        size = (max(1, round(width * scale)), max(1, round(height * scale)))
        picture = Image.new("RGB", area, COVER_BACKGROUND)
        picture.paste(image.resize(size, _RESAMPLE), ((area[0] - size[0]) // 2, (area[1] - size[1]) // 2))
    cover = Image.new("RGB", (COVER_WIDTH, COVER_HEIGHT), (0, 0, 0))
    cover.paste(picture, (0, 0))
    return cover


def _detail_map(image: Image.Image, short_side: int = 48) -> tuple[list[float], int, int]:
    """Edge strength plus a bit of saturation per pixel of a small copy (row by row), and its size."""
    width, height = image.size
    scale = short_side / min(width, height)
    size = (max(1, round(width * scale)), max(1, round(height * scale)))
    small = image.resize(size, Image.Resampling.BILINEAR)
    edges = small.convert("L").filter(ImageFilter.FIND_EDGES).tobytes()
    saturation = small.convert("HSV").getchannel("S").tobytes()
    return [edge + 0.25 * sat for edge, sat in zip(edges, saturation)], size[0], size[1]


def salient_square(image: Image.Image, share: float, anchor: tuple[float, float]) -> Image.Image:
    """The square crop (share of the short side) with the most detail, preferring the anchor.

    Simple heuristic for "the most recognizable part": logos and characters have many edges and
    strong colors, empty sky or plain backgrounds have none. A distance penalty keeps the crop near
    the anchor (where boxes and title screens usually have the logo) and away from rating labels and
    publisher logos in the corners."""
    width, height = image.size
    values, map_width, map_height = _detail_map(image)
    window = max(1, round(min(map_width, map_height) * share))
    # summed area table: sums of any window in constant time
    table = [[0.0] * (map_width + 1) for _ in range(map_height + 1)]
    for y in range(map_height):
        row_sum = 0.0
        row = values[y * map_width:(y + 1) * map_width]
        above, current = table[y], table[y + 1]
        for x in range(map_width):
            row_sum += row[x]
            current[x + 1] = above[x + 1] + row_sum
    free_x, free_y = map_width - window, map_height - window
    best, best_x, best_y = -1.0, 0, 0
    for y in range(free_y + 1):
        for x in range(free_x + 1):
            total = table[y + window][x + window] - table[y][x + window] - table[y + window][x] + table[y][x]
            dx = (x / free_x - anchor[0]) if free_x else 0.0
            dy = (y / free_y - anchor[1]) if free_y else 0.0
            score = total * (1.0 - 0.6 * min(1.0, (dx * dx + dy * dy) ** 0.5))
            if score > best:
                best, best_x, best_y = score, x, y
    scale = min(width, height) / min(map_width, map_height)
    side = round(min(width, height) * share)
    left = min(width - side, round(best_x * scale))
    top = min(height - side, round(best_y * scale))
    return image.crop((left, top, left + side, top + side))


def to_icon(image: Image.Image, art_type: str, system_key: Optional[str] = None) -> Image.Image:
    """Turns a boxart, screen or user image into a minimalist 32x32 RGBA icon (corners transparent).

    The most detailed square around the logo area is cut out, scaled down with a good filter and
    given a bit more contrast; to_icon_bmp then reduces it to 15 flat colors."""
    image = _flatten(image)
    if art_type in ("title", "snap"):
        image, _ = pick_screen(image, system_key)
        # title screens have the logo in the upper middle
        share, anchor = 1.0, (0.5, 0.35)
    elif art_type == "boxart":
        image = trim_spine(image, system_key)
        # the logo and the main character sit in the upper part of the box
        share, anchor = 0.85, (0.5, 0.3)
    else:
        # the user's own pictures: the middle
        share, anchor = 1.0, (0.5, 0.5)
    crop = salient_square(image, share, anchor)
    icon = crop.resize((ICON_SIZE, ICON_SIZE), _RESAMPLE, reducing_gap=3.0)
    icon = simplify(icon, contrast=1.12, color=1.2, sharpen=40).convert("RGBA")
    mask = Image.new("L", icon.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, ICON_SIZE - 1, ICON_SIZE - 1), ICON_CORNER_RADIUS, fill=255)
    icon.putalpha(mask)
    return icon


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
    """Reduces an RGB image to at most `colors` DS colors, with Floyd-Steinberg dithering or flat.

    Returns the pixel indices (0-based, row by row, top to bottom) and the palette.
    """
    rgb = image.convert("RGB")
    # build the palette from the colors the DS can show, then dither against exactly those colors
    lut = [_to_555(value) for value in range(256)]
    reduced = rgb.point(lut * 3)
    # few colors: refine the median cut palette (k-means), every color counts there
    palette_image = reduced.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE,
                                     kmeans=2 if colors <= 32 else 0)
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


def to_ds_bmp(image: Image.Image, dither: bool = True, colors: int = MAX_COLORS) -> bytes:
    """RGB image (256x192 artwork or 128x96 cover) -> BMP bytes in the launcher's format."""
    pixels, palette = quantize(image, colors, dither)
    return encode_bmp(pixels, palette, size=image.size)


def remove_specks(pixels: bytes, width: int, height: int, majority: int = 4) -> bytes:
    """Light mode filter: a pixel whose color none of its 8 neighbors has takes the color most of
    them have (when at least `majority` of them, or half of them at the edge, agree)."""
    result = bytearray(pixels)
    for y in range(height):
        for x in range(width):
            own = pixels[y * width + x]
            counts: dict[int, int] = {}
            neighbors = 0
            for ny in range(max(0, y - 1), min(height, y + 2)):
                for nx in range(max(0, x - 1), min(width, x + 2)):
                    if nx == x and ny == y:
                        continue
                    neighbors += 1
                    value = pixels[ny * width + nx]
                    counts[value] = counts.get(value, 0) + 1
            if own in counts:
                continue
            value, count = max(counts.items(), key=lambda item: (item[1], -item[0]))
            if count >= min(majority, neighbors // 2 + 1):
                result[y * width + x] = value
    return bytes(result)


def to_tile_bmp(image: Image.Image) -> bytes:
    """64x64 RGB tile image -> BMP: at most TILE_COLORS flat colors (no dithering, no specks)."""
    pixels, palette = quantize(image, TILE_COLORS, dither=False)
    pixels = remove_specks(pixels, image.width, image.height, majority=5)
    return encode_bmp(pixels, palette, size=image.size)


def to_cover_bmp(image: Image.Image) -> bytes:
    """128x96 RGB cover -> 8 bpp BMP for the cover flow (realistic: 255 colors, dithered)."""
    return to_ds_bmp(image, dither=True)


def encode_icon_bmp(pixels: bytes, palette: list[tuple[int, int, int]]) -> bytes:
    """Writes a 32x32 4 bpp bottom-up BMP with a 16 color palette (as BmpFileIconData reads it).

    pixels: one palette index (0-15) per pixel, row by row from the top; index 0 is transparent."""
    if len(pixels) != ICON_SIZE * ICON_SIZE or max(pixels) > 15:
        raise ValueError("32x32 pixels with indices 0-15 expected")
    palette = (list(palette) + [(0, 0, 0)] * 16)[:16]
    palette_bytes = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette)
    rows = []
    for y in range(ICON_SIZE):
        row = pixels[y * ICON_SIZE:(y + 1) * ICON_SIZE]
        # the left pixel is in the high nibble; 16 bytes per row need no padding
        rows.append(bytes((row[x] << 4) | row[x + 1] for x in range(0, ICON_SIZE, 2)))
    pixel_bytes = b"".join(reversed(rows))
    offset = 14 + 40 + len(palette_bytes)
    file_header = struct.pack("<2sIHHI", b"BM", offset + len(pixel_bytes), SIGNATURE[0], SIGNATURE[1], offset)
    info_header = struct.pack("<IiiHHIIiiII", 40, ICON_SIZE, ICON_SIZE, 1, 4, 0, len(pixel_bytes), 2835, 2835, 16, 0)
    return file_header + info_header + palette_bytes + pixel_bytes


def to_icon_bmp(icon: Image.Image) -> bytes:
    """32x32 RGBA icon -> 4 bpp BMP: at most 15 flat colors, transparent pixels get index 0."""
    icon = icon.convert("RGBA")
    if icon.size != (ICON_SIZE, ICON_SIZE):
        raise ValueError("32x32 icon expected")
    pixels, palette = quantize(icon.convert("RGB"), ICON_COLORS, dither=False)
    pixels = remove_specks(pixels, ICON_SIZE, ICON_SIZE)
    alpha = icon.getchannel("A").tobytes()
    indices = bytes(0 if a < 128 else p + 1 for p, a in zip(pixels, alpha))
    # entry 0 is never shown; the theme background makes previews on the PC look right
    return encode_icon_bmp(indices, [BACKGROUND] + list(palette))


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


def validate_bmp(data: bytes, size: tuple[int, int] = (WIDTH, HEIGHT), index_zero_warning: bool = True) -> Validation:
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
    if index_zero_warning and 0 in pixels:
        result.warnings.append("Palettenfarbe 0 wird benutzt (der DS ersetzt sie durch eine ähnliche Farbe)")
    if max(pixels) >= colors:
        result.warnings.append("Pixel verweisen auf Farben außerhalb der Palette")
    return result


def validate_cover_bmp(data: bytes) -> Validation:
    """Checks a cover like BmpFileCover does (BmpHeader::Validate: 128x96, 8 bpp, 40 byte header,
    0 or 256 palette entries). Covers are textures without transparency, index 0 is fine."""
    result = validate_bmp(data, (COVER_WIDTH, COVER_HEIGHT), index_zero_warning=False)
    if result.ok:
        dib_size = struct.unpack_from("<I", data, 0x0E)[0]
        colors = struct.unpack_from("<I", data, 0x2E)[0]
        if dib_size != 40:
            result.errors.append(f"BMP-Kopf mit {dib_size} statt 40 Bytes")
        if colors not in (0, 256):
            result.errors.append(f"{colors} Palettenfarben statt 256")
    return result


def validate_icon_bmp(data: bytes) -> Validation:
    """Checks an icon like BmpFileIconData::Load does: 32x32, 4 bpp, BI_RGB, 40 byte header,
    0 or 16 palette entries, pixel data after the 118 bytes of header and palette."""
    result = Validation()
    if len(data) < 118 or data[:2] != b"BM":
        result.errors.append("keine BMP-Datei")
        return result
    pixel_offset, dib_size = struct.unpack_from("<II", data, 0x0A)
    width, height = struct.unpack_from("<ii", data, 0x12)
    bpp = struct.unpack_from("<H", data, 0x1C)[0]
    compression = struct.unpack_from("<I", data, 0x1E)[0]
    colors = struct.unpack_from("<I", data, 0x2E)[0]
    if dib_size != 40:
        result.errors.append(f"BMP-Kopf mit {dib_size} statt 40 Bytes")
    if width != ICON_SIZE or abs(height) != ICON_SIZE:
        result.errors.append(f"Größe {width}x{abs(height)} statt 32x32")
    if bpp != 4:
        result.errors.append(f"{bpp} bit pro Pixel statt 4")
    if compression != 0:
        result.errors.append("komprimiert (nur BI_RGB geht)")
    if colors not in (0, 16):
        result.errors.append(f"{colors} Palettenfarben statt 16")
    if result.errors:
        return result
    if pixel_offset < 118:
        result.errors.append("Bilddaten beginnen vor dem Ende der Palette")
    elif pixel_offset + ICON_SIZE * ICON_SIZE // 2 > len(data):
        result.errors.append("Bilddaten unvollständig")
    return result


def decode_icon_bmp(data: bytes) -> tuple[list[list[int]], list[tuple[int, int, int]]]:
    """Reads an icon the way BmpFileIconData does: (rows of palette indices from the top, palette)."""
    palette = [(data[0x36 + i * 4 + 2], data[0x36 + i * 4 + 1], data[0x36 + i * 4]) for i in range(16)]
    pixel_offset = struct.unpack_from("<I", data, 0x0A)[0]
    top_down = struct.unpack_from("<i", data, 0x16)[0] < 0
    raw = data[pixel_offset:pixel_offset + ICON_SIZE * ICON_SIZE // 2]
    rows = []
    for y in range(ICON_SIZE):
        source = raw[(y if top_down else ICON_SIZE - 1 - y) * 16:][:16]
        rows.append([value for byte in source for value in (byte >> 4, byte & 0x0F)])
    return rows, palette


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
