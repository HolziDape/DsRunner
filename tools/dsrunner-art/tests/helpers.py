"""Test helpers: generated ROM headers and images (no real game data)."""
from __future__ import annotations

import io
import os
import struct
import sys
from pathlib import Path
from typing import Optional

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from PIL import Image, ImageDraw  # noqa: E402


def nds_rom(title: str, code: str, banner_title: Optional[str] = None, icon: bool = True) -> bytes:
    header = bytearray(0x200)
    header[0:len(title)] = title.encode("ascii")
    header[0x0C:0x10] = code.encode("ascii")
    header[0x10:0x12] = b"01"
    if banner_title is None:
        return bytes(header)
    struct.pack_into("<I", header, 0x68, 0x200)
    banner = bytearray(0x840)
    struct.pack_into("<H", banner, 0, 1)
    if icon:
        struct.pack_into("<H", banner, 0x222, 0x001F)  # palette 1: red
        struct.pack_into("<H", banner, 0x224, 0x7FFF)  # palette 2: white
        # tile 0: first row = pixels 1,2,1,2,..., rest 0; tile 5 completely color 1
        for i in range(4):
            banner[0x20 + i] = 0x21
        for i in range(32):
            banner[0x20 + 5 * 32 + i] = 0x11
    text = banner_title.encode("utf-16-le")
    banner[0x340:0x340 + len(text)] = text
    return bytes(header + banner)


def gba_rom(title: str, code: str, valid: bool = True) -> bytes:
    header = bytearray(0xC0)
    header[0xA0:0xA0 + len(title)] = title.encode("ascii")
    header[0xAC:0xB0] = code.encode("ascii")
    header[0xB0:0xB2] = b"01"
    header[0xB2] = 0x96 if valid else 0
    return bytes(header)


def write(path: Path, data: bytes) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return path


def png_bytes(size=(256, 384), color=(200, 40, 40), mode="RGB") -> bytes:
    image = Image.new(mode, size, color if mode == "RGB" else color + (255,))
    draw = ImageDraw.Draw(image)
    # some structure so the quantizer has work to do
    for x in range(0, size[0], 16):
        draw.line((x, 0, size[0] - x, size[1]), fill=(x % 256, 255 - x % 256, 128), width=3)
    draw.ellipse((size[0] // 4, size[1] // 8, size[0] * 3 // 4, size[1] // 2), fill=(30, 200, 90))
    buffer = io.BytesIO()
    image.save(buffer, "PNG")
    return buffer.getvalue()


DAT_NDS = '''clrmamepro (
\tname "Nintendo - Nintendo DS"
)

game (
\tname "Mario Kart DS (Europe) (En,Fr,De,Es,It)"
\tregion "Europe"
\tserial "AMCP"
\trom ( name "Mario Kart DS (Europe) (En,Fr,De,Es,It).nds" size 33554432 crc 94E8127E serial "AMCP" )
)
game (
\tname "Mario Kart DS (USA, Australia)"
\tregion "USA"
\tserial "AMCE"
\trom ( name "Mario Kart DS (USA, Australia).nds" size 33554432 crc 11111111 serial "AMCE" )
)
game (
\tname "New Super Mario Bros. (Europe) (En,Fr,De,Es,It)"
\tregion "Europe"
\tserial "A2DP"
\trom ( name "New Super Mario Bros. (Europe) (En,Fr,De,Es,It).nds" size 33554432 crc 22222222 serial "A2DP" )
)
game (
\tname "Mario Party DS (Europe) (En,Fr,De,Es,It)"
\tserial "AMPP"
\trom ( name "Mario Party DS (Europe) (En,Fr,De,Es,It).nds" size 33554432 crc 33333333 serial "AMPP" )
)
game (
\tname "Pokemon - Black Version (USA, Europe) (NDSi Enhanced)"
\tregion "USA"
\tserial "IRBO"
\trom ( name "Pokemon - Black Version (USA, Europe) (NDSi Enhanced).nds" size 268435456 crc 4F6E5580 serial "IRBO" )
)
game (
\tname "Pokemon - Black Version 2 (USA, Europe) (NDSi Enhanced)"
\tregion "USA"
\tserial "IREO"
\trom ( name "Pokemon - Black Version 2 (USA, Europe) (NDSi Enhanced).nds" size 536870912 crc D4427FD1 serial "IREO" )
)
game (
\tname "Plants vs. Zombies (Europe) (En,Fr,De,Es,It,Nl) (MSL)"
\tregion "Europe"
\tserial "BWBY"
\trom ( name "Plants vs. Zombies (Europe) (En,Fr,De,Es,It,Nl) (MSL).nds" size 67108864 crc 50FA9E93 serial "BWBY" )
)
game (
\tname "Adventure Time - Hey Ice King! Why'd You Steal Our Garbage!! (USA)"
\tregion "USA"
\tserial "TATE"
\trom ( name "Adventure Time - Hey Ice King! Why'd You Steal Our Garbage!! (USA).nds" size 33554432 crc D9D7E611 serial "TATE" )
)
game (
\tname "Up (Europe) (Fr,De,Nl)"
\tregion "Europe"
\tserial "CUYX"
\trom ( name "Up (Europe) (Fr,De,Nl).nds" size 33554432 crc DCD56D67 serial "CUYX" )
)
'''

# the games of the owner's bug report: (file name, header title, game code, banner title)
REPORTED_GAMES = [
    ("Pokemon - Black Version (DSi Enhanced)(USA) (E)(SweeTnDs).nds", "POKEMON B", "IRBO",
     "Pokémon Black Version\nNintendo"),
    ("Pokemon - Black Version 2 (USA, Europe) (NDSi Enhanced) [b].nds", "POKEMON B2", "IREO",
     "Pokémon Black Version 2\nNintendo"),
    ("Plants vs. Zombies (Europe) (En,Fr,De,Es,It,Nl) (MSL).nds", "PVZDS", "BWBY", "Plants vs. Zombies\nPopCap"),
    ("Adventure Time - Hey Ice King! Why'd You Steal Our Garbage!! (USA).nds", "ADVENTURETIM", "TATE",
     "Adventure Time™ HIK!\nD3Publisher"),
]

DAT_GB = '''clrmamepro (
\tname "Nintendo - Game Boy"
)

game (
\tname "Tetris (World) (Rev 1)"
\trom ( name "Tetris (World) (Rev 1).gb" size 32768 crc 46DF91AD )
)
game (
\tname "Tetris (Japan) (En)"
\trom ( name "Tetris (Japan) (En).gb" size 32768 crc 63F9407D )
)
'''


def cleanup_env() -> None:
    """Keep the user's real cache out of the tests."""
    os.environ.pop("LOCALAPPDATA", None)
