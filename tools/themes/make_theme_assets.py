"""Makes the theme list images of DsRunner themes: icon.bmp (32x32, 4 bpp) and preview.bin (256x192, 15 bpp).

The icon is drawn from the colors in theme.json: four small console tiles on the theme background, the
first one selected with the accent ring. The preview shows both DS screens, taken from a screenshot of
the theme (both screens stacked, 256x384 or 2x / 3x of it, e.g. from tools/desmume/screenshot.sh
without -trim). Without a screenshot only the icon is written.

Usage:
  python3 tools/themes/make_theme_assets.py _pico/themes/dsrunner-hell [--screenshot hell.png]
  python3 tools/themes/make_theme_assets.py _pico/themes/dsrunner* --screenshots shots/
      (shots/<folder name>.png for every theme folder that has one)

Needs Pillow (pip install pillow). See docs/Themes.md.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from PIL import Image, ImageDraw

# DsRunnerColors::Defaults (arm9/source/themes/dsrunner/DsRunnerColorScheme.h)
DEFAULTS = {
    "background": (15, 17, 23),
    "backgroundBottom": (24, 28, 38),
    "tile": (35, 40, 56),
    "accent": (76, 194, 255),
    "text": (235, 238, 245),
    "pill": (44, 50, 68),
}
# tile colors of NDS, GBA, GB and NES (DsRunnerConsoles::kConsoles)
CONSOLE_TILES = [(58, 92, 190), (112, 64, 184), (96, 144, 64), (190, 48, 48)]


def parse_color(value) -> tuple[int, int, int] | None:
    if isinstance(value, str):
        text = value.lstrip("#")
        if len(text) == 6:
            return tuple(int(text[i:i + 2], 16) for i in (0, 2, 4))
    if isinstance(value, list) and len(value) == 3:
        return tuple(int(v) for v in value)
    if isinstance(value, dict) and all(k in value for k in "rgb"):
        return int(value["r"]), int(value["g"]), int(value["b"])
    return None


def lerp(a, b, t: int, t_max: int):
    """RgbMixer::Lerp"""
    return tuple((a[i] * (t_max - t) + b[i] * t + t_max // 2) // t_max for i in range(3))


def theme_colors(theme_dir: Path) -> dict[str, tuple[int, int, int]]:
    """The main colors like DsRunnerColors::LoadFromThemeFolder sees them."""
    data = json.loads((theme_dir / "theme.json").read_text(encoding="utf-8"))
    colors = data.get("colors")
    if not isinstance(colors, dict):
        return dict(DEFAULTS)
    result = {}
    for key in ("background", "text", "tile", "accent"):
        result[key] = parse_color(colors.get(key)) or DEFAULTS[key]
    result["backgroundBottom"] = parse_color(colors.get("backgroundBottom")) or lerp(result["background"], result["tile"], 1, 2)
    result["pill"] = parse_color(colors.get("pill")) or lerp(result["tile"], result["text"], 1, 20)
    return result


# --- icon.bmp ------------------------------------------------------------------------------------

def draw_icon(colors: dict) -> Image.Image:
    image = Image.new("RGB", (32, 32), (255, 0, 255))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((0, 1, 31, 30), radius=6, fill=colors["pill"])
    draw.rounded_rectangle((1, 2, 30, 29), radius=5, fill=colors["background"])
    positions = [(5, 6), (17, 6), (5, 17), (17, 17)]
    for (x, y), tile in zip(positions, CONSOLE_TILES):
        draw.rounded_rectangle((x, y, x + 9, y + 8), radius=2, fill=tile)
        # a light screen on every tile
        draw.rectangle((x + 3, y + 2, x + 6, y + 4), fill=lerp(tile, (255, 255, 255), 5, 8))
    x, y = positions[0]
    draw.rounded_rectangle((x - 2, y - 2, x + 11, y + 10), radius=3, outline=colors["accent"], width=1)
    return image


def rgb_pixels(image: Image.Image) -> list[tuple[int, int, int]]:
    data = image.convert("RGB").tobytes()
    return [tuple(data[i:i + 3]) for i in range(0, len(data), 3)]


def encode_icon(image: Image.Image) -> bytes:
    """4 bpp bottom-up BMP, palette index 0 (magenta) is transparent."""
    pixels = rgb_pixels(image)
    palette = [(255, 0, 255)]
    for color in pixels:
        if color not in palette:
            palette.append(color)
    if len(palette) > 16:
        raise SystemExit("icon has more than 16 colors")
    index = {color: i for i, color in enumerate(palette)}
    rows = []
    for y in range(32):
        row = [index[pixels[y * 32 + x]] for x in range(32)]
        rows.append(bytes((row[x] << 4) | row[x + 1] for x in range(0, 32, 2)))
    pixel_bytes = b"".join(reversed(rows))
    palette_bytes = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette + [(0, 0, 0)] * (16 - len(palette)))
    offset = 14 + 40 + len(palette_bytes)
    return (struct.pack("<2sIHHI", b"BM", offset + len(pixel_bytes), 0, 0, offset)
            + struct.pack("<IiiHHIIiiII", 40, 32, 32, 1, 4, 0, len(pixel_bytes), 2835, 2835, 16, 0)
            + palette_bytes + pixel_bytes)


# --- preview.bin ---------------------------------------------------------------------------------

SCREEN_W, SCREEN_H = 120, 90


def make_preview(colors: dict, screenshot: Path) -> Image.Image:
    shot = Image.open(screenshot).convert("RGB")
    scale = shot.width / 256
    if shot.width % 256 or shot.height != round(384 * scale):
        raise SystemExit(f"{screenshot}: expected both screens stacked, 256x384 or a multiple, got {shot.size}")
    top = shot.crop((0, 0, shot.width, shot.height // 2))
    bottom = shot.crop((0, shot.height // 2, shot.width, shot.height))
    preview = Image.new("RGB", (256, 192))
    draw = ImageDraw.Draw(preview)
    for y in range(192):
        draw.line((0, y, 255, y), fill=lerp(colors["background"], colors["backgroundBottom"], y, 191))
    x = (256 - SCREEN_W) // 2
    for screen, y in ((top, 4), (bottom, 192 - 4 - SCREEN_H)):
        draw.rectangle((x - 1, y - 1, x + SCREEN_W, y + SCREEN_H), outline=colors["pill"])
        preview.paste(screen.resize((SCREEN_W, SCREEN_H), Image.LANCZOS), (x, y))
    return preview


def encode_preview(image: Image.Image) -> bytes:
    """15 bpp direct color, bit 15 set (opaque), like topbg.bin."""
    out = bytearray()
    for r, g, b in rgb_pixels(image):
        value = 0x8000 | ((b * 31 + 127) // 255) << 10 | ((g * 31 + 127) // 255) << 5 | ((r * 31 + 127) // 255)
        out += struct.pack("<H", value)
    return bytes(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("themes", nargs="+", type=Path, help="theme folders (with theme.json)")
    parser.add_argument("--screenshot", type=Path, help="screenshot for the preview (one theme)")
    parser.add_argument("--screenshots", type=Path, help="folder with <theme folder name>.png screenshots")
    parser.add_argument("--png", type=Path, help="also save icon and preview as PNG into this folder (to look at)")
    args = parser.parse_args()
    for theme_dir in args.themes:
        if not (theme_dir / "theme.json").is_file():
            continue
        colors = theme_colors(theme_dir)
        icon = draw_icon(colors)
        (theme_dir / "icon.bmp").write_bytes(encode_icon(icon))
        written = ["icon.bmp"]
        shot = args.screenshot if args.screenshot and len(args.themes) == 1 else None
        if args.screenshots and (args.screenshots / f"{theme_dir.name}.png").is_file():
            shot = args.screenshots / f"{theme_dir.name}.png"
        preview = None
        if shot:
            preview = make_preview(colors, shot)
            (theme_dir / "preview.bin").write_bytes(encode_preview(preview))
            written.append("preview.bin")
        if args.png:
            args.png.mkdir(parents=True, exist_ok=True)
            icon.resize((128, 128), Image.NEAREST).save(args.png / f"{theme_dir.name}-icon.png")
            if preview:
                preview.save(args.png / f"{theme_dir.name}-preview.png")
        print(f"{theme_dir}: {', '.join(written)}")


if __name__ == "__main__":
    main()
