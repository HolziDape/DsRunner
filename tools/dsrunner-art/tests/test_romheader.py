import tempfile
import unittest
import zlib
from pathlib import Path

from tests.helpers import gba_rom, nds_rom, write

from dsrunnerart.romheader import (copier_header_size, crc32_of_file, parse_gba_header, parse_nds_header,
                                   read_header)


class NdsHeaderTests(unittest.TestCase):
    def test_title_and_code(self):
        header = parse_nds_header(nds_rom("MARIOKART DS", "AMCP"))
        self.assertEqual(header.title, "MARIOKART DS")
        self.assertEqual(header.game_code, "AMCP")
        self.assertEqual(header.maker_code, "01")
        self.assertEqual(header.region_char, "P")

    def test_homebrew_codes_are_ignored(self):
        self.assertIsNone(parse_nds_header(nds_rom("HOMEBREW", "####")).game_code)
        self.assertIsNone(parse_nds_header(nds_rom("HOMEBREW", "\0\0\0\0")).game_code)
        self.assertIsNone(parse_nds_header(nds_rom("HOMEBREW", "ab12")).game_code)

    def test_too_short(self):
        self.assertIsNone(parse_nds_header(b"\0" * 16))

    def test_banner_title_and_icon(self):
        with tempfile.TemporaryDirectory() as temp:
            path = write(Path(temp) / "game.nds", nds_rom("POKEMON PL", "CPUD", "Pokémon Platin-Edition\nNintendo"))
            header = read_header(path, "nds")
        self.assertEqual(header.game_code, "CPUD")
        self.assertEqual(header.banner_title, "Pokémon Platin-Edition\nNintendo")
        icon = header.icon
        self.assertIsNotNone(icon)
        self.assertEqual(icon.size, (32, 32))
        # tile 0, first row: low nibble first -> pixel 0 = color 1 (red), pixel 1 = color 2 (white)
        self.assertEqual(icon.getpixel((0, 0)), (255, 0, 0, 255))
        self.assertEqual(icon.getpixel((1, 0)), (255, 255, 255, 255))
        # color 0 is transparent
        self.assertEqual(icon.getpixel((0, 1))[3], 0)
        # tile 5 is the second tile of the second tile row (x 8..15, y 8..15)
        self.assertEqual(icon.getpixel((12, 12)), (255, 0, 0, 255))

    def test_banner_without_icon(self):
        with tempfile.TemporaryDirectory() as temp:
            path = write(Path(temp) / "game.nds", nds_rom("X", "AMCP", "Title", icon=False))
            header = read_header(path, "nds")
        self.assertEqual(header.banner_title, "Title")
        self.assertIsNone(header.icon)


class GbaHeaderTests(unittest.TestCase):
    def test_title_and_code(self):
        header = parse_gba_header(gba_rom("GOLDEN_SUN_A", "AGSD"))
        self.assertEqual(header.title, "GOLDEN_SUN_A")
        self.assertEqual(header.game_code, "AGSD")

    def test_requires_fixed_value(self):
        self.assertIsNone(parse_gba_header(gba_rom("GOLDEN_SUN_A", "AGSD", valid=False)))
        self.assertIsNone(parse_gba_header(b"\0" * 0x40))

    def test_read_header_by_system(self):
        with tempfile.TemporaryDirectory() as temp:
            path = write(Path(temp) / "game.gba", gba_rom("GOLDEN_SUN_A", "AGSD"))
            self.assertEqual(read_header(path, "gba").game_code, "AGSD")
            self.assertIsNone(read_header(path, "gb"))
            self.assertIsNone(read_header(Path(temp) / "missing.gba", "gba"))


class ChecksumTests(unittest.TestCase):
    def test_crc_and_copier_headers(self):
        with tempfile.TemporaryDirectory() as temp:
            data = bytes(range(256)) * 64
            nes = write(Path(temp) / "game.nes", b"NES\x1a" + b"\0" * 12 + data)
            self.assertEqual(copier_header_size(nes, "nes", nes.stat().st_size), 16)
            self.assertEqual(crc32_of_file(nes, 16), zlib.crc32(data))
            snes = write(Path(temp) / "game.smc", b"\0" * 512 + data)
            self.assertEqual(copier_header_size(snes, "snes", snes.stat().st_size), 512)
            plain = write(Path(temp) / "game.sfc", data)
            self.assertEqual(copier_header_size(plain, "snes", plain.stat().st_size), 0)


if __name__ == "__main__":
    unittest.main()
