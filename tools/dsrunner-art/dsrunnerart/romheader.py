"""ROM header parsing for NDS and GBA cartridges (game code, title, NDS banner)."""
from __future__ import annotations

import re
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO, Optional

from PIL import Image

_GAME_CODE_RE = re.compile(r"^[A-Z0-9]{4}$")
# codes homebrew uses instead of a real game code
_PLACEHOLDER_CODES = {"####", "0000", "HOME", "PASS", "ABXA"}

NDS_HEADER_SIZE = 0x200
GBA_HEADER_SIZE = 0xC0
BANNER_SIZE = 0x840


@dataclass
class RomHeader:
    """What the header tells about a ROM. Fields are None when unknown."""
    title: str = ""
    game_code: Optional[str] = None
    maker_code: Optional[str] = None
    # NDS only: the English banner title (first line = game name) and the 32x32 icon
    banner_title: Optional[str] = None
    icon: Optional[Image.Image] = None

    @property
    def region_char(self) -> Optional[str]:
        return self.game_code[3] if self.game_code else None


def _ascii(data: bytes) -> str:
    return data.split(b"\0", 1)[0].decode("ascii", "replace").strip()


def _valid_code(code: str) -> Optional[str]:
    if _GAME_CODE_RE.match(code) and code not in _PLACEHOLDER_CODES:
        return code
    return None


def parse_nds_header(data: bytes) -> Optional[RomHeader]:
    """Parses the first 0x200 bytes of an NDS ROM (title at 0x00, game code at 0x0C)."""
    if len(data) < 0x70:
        return None
    title = _ascii(data[0x00:0x0C])
    code = data[0x0C:0x10].decode("ascii", "replace")
    maker = data[0x10:0x12].decode("ascii", "replace")
    return RomHeader(title=title, game_code=_valid_code(code), maker_code=maker if maker.isalnum() else None)


def parse_gba_header(data: bytes) -> Optional[RomHeader]:
    """Parses the first 0xC0 bytes of a GBA ROM (title at 0xA0, game code at 0xAC)."""
    if len(data) < GBA_HEADER_SIZE:
        return None
    # fixed value in every GBA header, tells real ROMs from other files
    if data[0xB2] != 0x96:
        return None
    title = _ascii(data[0xA0:0xAC])
    code = data[0xAC:0xB0].decode("ascii", "replace")
    maker = data[0xB0:0xB2].decode("ascii", "replace")
    return RomHeader(title=title, game_code=_valid_code(code), maker_code=maker if maker.isalnum() else None)


def decode_nds_icon(banner: bytes) -> Optional[Image.Image]:
    """The 32x32 banner icon (4x4 tiles of 8x8 pixels, 4 bpp, palette of 16 BGR555 colors) as RGBA."""
    if len(banner) < 0x240:
        return None
    pixels = banner[0x20:0x220]
    colors = struct.unpack("<16H", banner[0x220:0x240])
    palette = []
    for index, color in enumerate(colors):
        r, g, b = color & 31, (color >> 5) & 31, (color >> 10) & 31
        palette.append(((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), 0 if index == 0 else 255))
    image = Image.new("RGBA", (32, 32))
    put = image.load()
    for tile in range(16):
        tile_x, tile_y = (tile % 4) * 8, (tile // 4) * 8
        for i in range(32):
            byte = pixels[tile * 32 + i]
            y = i // 4
            x = (i % 4) * 2
            put[tile_x + x, tile_y + y] = palette[byte & 15]
            put[tile_x + x + 1, tile_y + y] = palette[byte >> 4]
    # an icon without visible pixels or with a single color is no icon
    if image.getchannel("A").getextrema()[1] == 0:
        return None
    colors = image.getcolors(1024)
    if colors is not None and len(colors) < 2:
        return None
    return image


def parse_nds_banner(banner: bytes) -> tuple[Optional[str], Optional[Image.Image]]:
    """English title (offset 0x340, UTF-16) and icon of an NDS banner."""
    if len(banner) < 0x440:
        return None, None
    version = struct.unpack_from("<H", banner, 0)[0]
    if version == 0 or version == 0xFFFF:
        return None, None
    title = banner[0x340:0x440].decode("utf-16-le", "replace").split("\0", 1)[0].strip()
    return title or None, decode_nds_icon(banner)


def read_header(path: Path, system_key: str) -> Optional[RomHeader]:
    """Reads the header of a ROM file. Only NDS and GBA have useful headers."""
    try:
        with open(path, "rb") as file:
            if system_key == "nds":
                return _read_nds(file)
            if system_key == "gba":
                return parse_gba_header(file.read(GBA_HEADER_SIZE))
    except OSError:
        return None
    return None


def _read_nds(file: BinaryIO) -> Optional[RomHeader]:
    data = file.read(NDS_HEADER_SIZE)
    header = parse_nds_header(data)
    if not header:
        return None
    banner_offset = struct.unpack_from("<I", data, 0x68)[0]
    if banner_offset >= NDS_HEADER_SIZE:
        try:
            file.seek(banner_offset)
            banner = file.read(BANNER_SIZE)
            header.banner_title, header.icon = parse_nds_banner(banner)
        except (OSError, ValueError, OverflowError):
            pass
    return header


def crc32_of_file(path: Path, skip: int = 0, chunk_size: int = 1 << 20) -> int:
    crc = 0
    with open(path, "rb") as file:
        if skip:
            file.seek(skip)
        while True:
            chunk = file.read(chunk_size)
            if not chunk:
                break
            crc = zlib.crc32(chunk, crc)
    return crc & 0xFFFFFFFF


def copier_header_size(path: Path, system_key: str, size: int) -> int:
    """Size of a header that some dumps carry in front of the ROM data (iNES, SNES copier)."""
    if system_key in ("nes", "fds"):
        # iNES header, fwNES header of Famicom Disk System images
        try:
            with open(path, "rb") as file:
                if file.read(4) in (b"NES\x1a", b"FDS\x1a"):
                    return 16
        except OSError:
            return 0
    if system_key == "snes" and size % 1024 == 512:
        return 512
    return 0
