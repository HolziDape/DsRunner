"""Built-in console images of the DsRunner theme: the tool stops generating folder artworks for them."""
import json
import re
import tempfile
import unittest
from pathlib import Path

from tests.helpers import cleanup_env, nds_rom, write

from dsrunnerart.consoles import ART_KEYS, art_key, parse_systems_json, set_custom_systems
from dsrunnerart.imaging import console_art, to_ds_bmp
from dsrunnerart.sdcard import builtin_console_art_folder, read_theme
from dsrunnerart.updater import Options, Updater

REPO = Path(__file__).resolve().parents[3]
CONSOLE_ART_H = REPO / "arm9" / "source" / "themes" / "dsrunner" / "DsRunnerConsoleArt.h"
THEME_CONSOLES = REPO / "_pico" / "themes" / "dsrunner" / "consoles"


def _unescape(text: str) -> str:
    data = re.sub(r"\\x([0-9A-Fa-f]{2})", lambda m: chr(int(m.group(1), 16)), text)
    return data.encode("latin-1").decode("utf-8")


class ArtKeyTests(unittest.TestCase):
    def tearDown(self):
        set_custom_systems([])

    def test_keys_match_header(self):
        if not CONSOLE_ART_H.is_file():
            raise unittest.SkipTest("launcher sources not found")
        header = CONSOLE_ART_H.read_text(encoding="utf-8")
        table = header[header.index("kArtKeys[]"):]
        table = table[:table.index("};")]
        launcher = {_unescape(folder): key for folder, key in re.findall(r'\{\s*"([^"]+)",\s*"(\w+)"\s*\}', table)}
        self.assertEqual(launcher, ART_KEYS)
        self.assertIn('kCustomConsoleKey = "CART"', header)
        self.assertIn('kFolderKey = "FOLDER"', header)

    def test_every_key_has_its_images(self):
        if not THEME_CONSOLES.is_dir():
            raise unittest.SkipTest("theme folder not found")
        for key in set(ART_KEYS.values()) | {"CART", "FOLDER"}:
            for suffix in ("", "-tile", "-icon", "-cover"):
                with self.subTest(key=key, suffix=suffix):
                    self.assertTrue((THEME_CONSOLES / f"{key}{suffix}.bmp").is_file())

    def test_art_key(self):
        self.assertEqual(art_key("nds"), "NDS")
        self.assertEqual(art_key("Megadrive"), "MD")
        self.assertEqual(art_key("★ Favoriten"), "FAV")
        self.assertEqual(art_key("Rennen"), "FOLDER")
        customs, _ = parse_systems_json({"systems": {"WS": {"extensions": ["ws"]}, "SMS": {"label": "MS"}}})
        set_custom_systems(customs)
        self.assertEqual(art_key("ws"), "CART")
        # an override of a built-in console keeps its image
        self.assertEqual(art_key("SMS"), "SMS")


class BuiltinArtTests(unittest.TestCase):
    def setUp(self):
        cleanup_env()
        self.temp = tempfile.TemporaryDirectory()
        base = Path(self.temp.name)
        self.sd = base / "sd"
        self.cache = base / "cache"
        write(self.sd / "_pico" / "settings.json", json.dumps({"libraryPath": "/Games", "theme": "dsrunner"}).encode())
        write(self.sd / "Games" / "NDS" / "Homebrew.nds", nds_rom("HOMEBREW", "####", "My Homebrew\nby me"))
        write(self.sd / "Games" / "GBA" / "Rennen" / "x.txt", b"")
        write(self.sd / "Games" / "Misc" / "x.txt", b"")

    def tearDown(self):
        self.temp.cleanup()

    def install_theme(self, keys=("NDS", "GBA", "FOLDER")) -> None:
        folder = self.sd / "_pico" / "themes" / "dsrunner" / "consoles"
        write(folder / "FOLDER-icon.bmp", b"BM")
        for key in keys:
            write(folder / f"{key}.bmp", b"BM")

    def run_updater(self, **options):
        updater = Updater(self.sd, Options(cache_dir=self.cache, workers=1, download=False, tiles=False,
                                           covers=False, icons=False, **options))
        return updater.run()

    def art(self, relative: str) -> Path:
        return self.sd / "_dsrunner" / "art" / relative

    def test_theme_lookup(self):
        self.assertEqual(read_theme(self.sd), "dsrunner")
        self.assertIsNone(builtin_console_art_folder(self.sd))
        self.install_theme()
        self.assertEqual(builtin_console_art_folder(self.sd), self.sd / "_pico" / "themes" / "dsrunner" / "consoles")
        # another theme folder in the settings: its images count
        write(self.sd / "_pico" / "settings.json", json.dumps({"theme": "mine"}).encode())
        self.assertIsNone(builtin_console_art_folder(self.sd))

    def test_old_card_still_gets_generated_artworks(self):
        stats = self.run_updater()
        self.assertEqual(stats.generated, 4)  # NDS, GBA, GBA/Rennen, Misc
        self.assertEqual(stats.builtin, 0)
        self.assertTrue(self.art("Games/NDS.bmp").is_file())

    def test_no_artworks_for_folders_with_a_builtin_image(self):
        self.install_theme()
        stats = self.run_updater()
        self.assertEqual((stats.generated, stats.builtin, stats.builtin_removed), (0, 4, 0))
        for relative in ("Games/NDS.bmp", "Games/GBA.bmp", "Games/GBA/Rennen.bmp", "Games/Misc.bmp"):
            self.assertFalse(self.art(relative).exists(), relative)

    def test_generated_artworks_are_replaced_on_cards_where_the_tool_ran(self):
        self.run_updater()
        self.assertTrue(self.art("Games/NDS.bmp").is_file())
        # a hand-placed folder artwork stays
        own = to_ds_bmp(console_art("Mine", (10, 20, 30)))
        own = own[:6] + b"\0\0\0\0" + own[10:]  # without the tool's signature
        write(self.art("Games/Misc.bmp"), own)
        self.install_theme()
        stats = self.run_updater()
        self.assertEqual((stats.builtin, stats.builtin_removed, stats.foreign), (3, 3, 1))
        self.assertFalse(self.art("Games/NDS.bmp").exists())
        self.assertFalse(self.art("Games/GBA/Rennen.bmp").exists())
        self.assertEqual(self.art("Games/Misc.bmp").read_bytes(), own)
        manifest = json.loads(self.art(".dsrunner-art.json").read_text(encoding="utf-8"))["artworks"]
        self.assertNotIn("/_dsrunner/art/games/nds.bmp", manifest)

    def test_missing_key_image_keeps_generating(self):
        # the theme has no GBA image: that folder still gets a generated artwork
        self.install_theme(keys=("NDS", "FOLDER"))
        stats = self.run_updater()
        self.assertEqual((stats.generated, stats.builtin), (1, 3))
        self.assertTrue(self.art("Games/GBA.bmp").is_file())

    def test_override_wins_over_the_builtin_image(self):
        from tests.helpers import png_bytes
        self.install_theme()
        write(self.sd / "_dsrunner" / "art-override" / "Games" / "NDS.png", png_bytes())
        stats = self.run_updater()
        self.assertEqual((stats.overrides, stats.builtin), (1, 3))
        self.assertTrue(self.art("Games/NDS.bmp").is_file())

    def test_dry_run_deletes_nothing(self):
        self.run_updater()
        self.install_theme()
        stats = self.run_updater(dry_run=True)
        self.assertEqual(stats.builtin_removed, 4)
        self.assertTrue(self.art("Games/NDS.bmp").is_file())


if __name__ == "__main__":
    unittest.main()
