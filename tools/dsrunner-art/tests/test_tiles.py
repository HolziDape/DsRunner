"""Tile images, hand-placed files and the games of the bug report, on generated cards with a fake network."""
import http.client
import io
import json
import shutil
import tempfile
import unittest
from pathlib import Path
from typing import Optional
from unittest import mock

from PIL import Image

from tests.helpers import DAT_NDS, REPORTED_GAMES, cleanup_env, gba_rom, nds_rom, png_bytes, write
from tests.test_updater import FakeClient, dat_url, thumb

from dsrunnerart.imaging import is_tool_bmp, validate_bmp
from dsrunnerart.net import HttpClient, NetworkError
from dsrunnerart.sources import ArtSource, Candidate
from dsrunnerart.updater import Options, Updater, check_card

DS = "Nintendo_-_Nintendo_DS"
GBA = "Nintendo_-_Game_Boy_Advance"
MARIO_KART = "Mario Kart DS (Europe) (En,Fr,De,Es,It)"
DAT_GBA = '''clrmamepro (
\tname "Nintendo - Game Boy Advance"
)

game (
\tname "Golden Sun (Germany)"
\tserial "AGSD"
\trom ( name "Golden Sun (Germany).gba" size 8388608 crc 44444444 serial "AGSD" )
)
'''


class CardTestCase(unittest.TestCase):
    def setUp(self):
        cleanup_env()
        self.temp = tempfile.TemporaryDirectory()
        base = Path(self.temp.name)
        self.sd = base / "sd"
        self.cache = base / "cache"
        write(self.sd / "_pico" / "settings.json", json.dumps({"libraryPath": "/Games"}).encode())
        self.files = {
            dat_url("Nintendo - Nintendo DS"): DAT_NDS.encode(),
            dat_url("Nintendo - Game Boy Advance"): DAT_GBA.encode(),
        }
        self.logs: list[tuple[str, str]] = []

    def tearDown(self):
        self.temp.cleanup()

    def run_updater(self, offline=False, sources=None, **options):
        client = FakeClient(self.cache, self.files, offline=offline)
        self.logs = []
        updater = Updater(self.sd, Options(cache_dir=self.cache, workers=2, download=not offline, **options),
                          log=lambda message, level="info": self.logs.append((level, message)), client=client,
                          sources=sources)
        return updater.run(), client

    def art(self, relative: str) -> Path:
        return self.sd / "_dsrunner" / "art" / relative

    def tile(self, relative: str) -> Path:
        return self.sd / "_dsrunner" / "tiles" / relative

    def manifest(self, folder: str = "art", section: str = "artworks") -> dict:
        return json.loads((self.sd / "_dsrunner" / folder / ".dsrunner-art.json").read_text(encoding="utf-8"))[section]

    def tile_source(self, relative: str) -> str:
        return self.manifest("tiles", "tiles")["/_dsrunner/tiles/" + relative.lower()]["source"]

    def art_source(self, relative: str) -> str:
        return self.manifest()["/_dsrunner/art/" + relative.lower()]["source"]

    def messages(self, level: Optional[str] = None) -> str:
        return "\n".join(message for item_level, message in self.logs if level is None or item_level == level)

    def game(self, name: str, title: str, code: str, banner: Optional[str] = None, folder: str = "NDS") -> Path:
        return write(self.sd / "Games" / folder / name, nds_rom(title, code, banner))


class TileTests(CardTestCase):
    def setUp(self):
        super().setUp()
        self.game("Mario Kart DS.nds", "MARIOKART DS", "AMCP", "Mario Kart DS\nNintendo")
        self.game("Oben.nds", "UP", "CUYX", "Disney•Pixar Oben\nTHQ")
        self.game("New Super Mario Bros.nds", "NEW MARIO", "A2DP", "New Super Mario Bros.\nNintendo")
        self.game("Mario Party DS.nds", "MARIO PARTY", "AMPP", "Mario Party DS\nNintendo")
        self.game("Homebrew.nds", "HOMEBREW", "####", "My Homebrew\nby me")
        self.game("Unknown Game.nds", "UNKNOWN", "ZZZZ")
        write(self.sd / "Games" / "GBA" / "Golden Sun.gba", gba_rom("GOLDEN_SUN_A", "AGSD"))
        self.files.update({
            thumb(DS, "Named_Titles", MARIO_KART): png_bytes((256, 384), (255, 0, 0)),
            thumb(DS, "Named_Boxarts", MARIO_KART): png_bytes((512, 459), (0, 0, 255)),
            # "Up" has no title in libretro, only a snap (a level map) and the box
            thumb(DS, "Named_Snaps", "Up (Europe) (Fr,De,Nl)"): png_bytes((256, 384), (0, 255, 0)),
            thumb(DS, "Named_Boxarts", "Up (Europe) (Fr,De,Nl)"): png_bytes((512, 452), (255, 255, 0)),
            thumb(DS, "Named_Titles", "New Super Mario Bros. (Europe) (En,Fr,De,Es,It)"): png_bytes((256, 384)),
            thumb(DS, "Named_Snaps", "Mario Party DS (Europe) (En,Fr,De,Es,It)"): png_bytes((256, 384)),
            thumb(GBA, "Named_Boxarts", "Golden Sun (Germany)"): png_bytes((512, 512), (0, 200, 200)),
        })

    def test_tiles_and_fallback_order(self):
        stats, _ = self.run_updater()
        self.assertEqual(stats.total_errors, 0, self.logs)
        tiles = stats.tiles
        self.assertEqual(tiles.downloaded, 5)
        self.assertEqual(tiles.icons, 1)
        self.assertEqual(tiles.missing, 1)
        # boxart first, then title, then snap, then the banner icon
        self.assertTrue(self.tile_source("Games/NDS/Mario Kart DS.bmp").startswith("libretro:boxart:"))
        self.assertTrue(self.tile_source("Games/GBA/Golden Sun.bmp").startswith("libretro:boxart:"))
        self.assertTrue(self.tile_source("Games/NDS/New Super Mario Bros.bmp").startswith("libretro:title:"))
        self.assertTrue(self.tile_source("Games/NDS/Mario Party DS.bmp").startswith("libretro:snap:"))
        self.assertEqual(self.tile_source("Games/NDS/Homebrew.bmp"), "icon:1")
        self.assertFalse(self.tile("Games/NDS/Unknown Game.bmp").exists())
        self.assertIn("Unknown Game.nds: kein Kachel-Bild (Spiel nicht erkannt; das ROM hat kein Icon)",
                      self.messages("warn"))
        # every tile is a valid 64x64 BMP with palette index 0 unused, folders get none
        for path in self.tile("").rglob("*.bmp"):
            with self.subTest(path=path.name):
                validation = validate_bmp(path.read_bytes(), (64, 64))
                self.assertEqual((validation.errors, validation.warnings), ([], []))
        self.assertFalse(self.tile("Games/NDS.bmp").exists())
        self.assertFalse(self.tile("Games.bmp").exists())
        # the cover tile of Mario Kart is blue (the box), not red (the title screen)
        red, green, blue = Image.open(self.tile("Games/NDS/Mario Kart DS.bmp")).convert("RGB").resize((1, 1)).getpixel((0, 0))
        self.assertGreater(blue, red)

        # second run: nothing new
        mtime = self.tile("Games/NDS/Mario Kart DS.bmp").stat().st_mtime_ns
        stats, client = self.run_updater()
        self.assertEqual(stats.tiles.downloaded, 0)
        self.assertEqual(stats.tiles.unchanged, 6)
        self.assertEqual(self.tile("Games/NDS/Mario Kart DS.bmp").stat().st_mtime_ns, mtime)

    def test_missing_title_uses_box_before_snap(self):
        self.run_updater()
        self.assertTrue(self.art_source("Games/NDS/Oben.bmp").startswith("libretro:boxart:"))
        # with "snap" chosen on purpose the snap is used
        stats, _ = self.run_updater(art_type="snap", force=True)
        self.assertTrue(self.art_source("Games/NDS/Oben.bmp").startswith("libretro:snap:"))

    def test_no_tiles(self):
        stats, _ = self.run_updater(tiles=False)
        self.assertIsNone(stats.tiles)
        self.assertFalse(self.tile("").exists())
        self.assertNotIn("Kachel", stats.summary())

    def test_tile_override_and_hand_placed_tile(self):
        override = self.sd / "_dsrunner" / "tile-override"
        write(override / "Games" / "NDS" / "Unknown Game.png", png_bytes((100, 140), (255, 0, 255)))
        write(override / "homebrew.png", png_bytes((64, 64), (0, 255, 255)))
        own = write(self.tile("Games/NDS/Mario Kart DS.bmp"), b"my own tile")
        stats, _ = self.run_updater()
        self.assertEqual(stats.tiles.overrides, 2)
        self.assertEqual(stats.tiles.foreign, 1)
        self.assertEqual(own.read_bytes(), b"my own tile")
        self.assertTrue(self.tile_source("Games/NDS/Unknown Game.bmp").startswith("override:"))
        self.assertTrue(validate_bmp(self.tile("Games/NDS/Homebrew.bmp").read_bytes(), (64, 64)).ok)

    def test_clean_and_check(self):
        self.run_updater()
        (self.sd / "Games" / "NDS" / "Mario Kart DS.nds").unlink()
        stats, _ = self.run_updater(clean=True)
        self.assertEqual(stats.tiles.removed, 1)
        self.assertFalse(self.tile("Games/NDS/Mario Kart DS.bmp").exists())
        messages = []
        self.assertEqual(check_card(self.sd, lambda message, level="info": messages.append(message)), 0)
        self.assertIn("5 Kachel-Bilder geprüft, 0 unbrauchbar.", messages)
        # an artwork in the tile folder has the wrong size
        write(self.tile("Games/NDS/Wrong.bmp"), self.art("Games/NDS.bmp").read_bytes())
        self.assertEqual(check_card(self.sd, lambda message, level="info": messages.append(message)), 1)
        self.assertTrue(any("tiles/Games/NDS/Wrong.bmp: Größe 256x192 statt 64x64" in m for m in messages))


class HandPlacedTests(CardTestCase):
    def setUp(self):
        super().setUp()
        self.game("Mario Kart DS.nds", "MARIOKART DS", "AMCP", "Mario Kart DS\nNintendo")
        self.files[thumb(DS, "Named_Titles", MARIO_KART)] = png_bytes((256, 384), (255, 0, 0))
        buffer = io.BytesIO()
        Image.new("RGB", (256, 192), (1, 2, 3)).quantize(8).save(buffer, "BMP")
        self.hand_made = buffer.getvalue()
        write(self.art("Games/NDS.bmp"), self.hand_made)
        write(self.art("Games/NDS/Mario Kart DS.bmp"), self.hand_made)

    def test_kept_by_default_and_reported(self):
        stats, _ = self.run_updater()
        self.assertEqual(stats.foreign, 2)
        self.assertEqual(self.art("Games/NDS.bmp").read_bytes(), self.hand_made)
        self.assertIn("2 von Hand abgelegt (behalten)", stats.summary())
        hint = self.messages("info")
        self.assertIn("Hinweis: 2 von Hand abgelegte Bilder wurden behalten", hint)
        self.assertIn("--replace-manual", hint)
        self.assertIn("Games/NDS/Mario Kart DS.bmp", hint)

    def test_replace_manual(self):
        stats, _ = self.run_updater(replace_manual=True)
        self.assertEqual(stats.foreign, 0)
        self.assertEqual(stats.replaced_manual, 2)
        self.assertEqual(stats.downloaded, 1)
        self.assertEqual(stats.generated, 1)
        for relative in ("Games/NDS.bmp", "Games/NDS/Mario Kart DS.bmp"):
            data = self.art(relative).read_bytes()
            self.assertNotEqual(data, self.hand_made)
            self.assertTrue(is_tool_bmp(data))
        # from now on they belong to the tool
        stats, _ = self.run_updater()
        self.assertEqual((stats.foreign, stats.unchanged), (0, 2))

    def test_overrides_still_win_and_nothing_found_keeps_the_file(self):
        del self.files[thumb(DS, "Named_Titles", MARIO_KART)]
        stats, _ = self.run_updater(replace_manual=True, icon_fallback=False)
        # nothing found for Mario Kart: the hand-placed file stays
        self.assertEqual(self.art("Games/NDS/Mario Kart DS.bmp").read_bytes(), self.hand_made)

    def test_unusable_hand_placed_file_is_reported(self):
        write(self.art("Games/NDS/Mario Kart DS.bmp"), b"BM" + b"\0" * 60)
        stats, _ = self.run_updater()
        self.assertEqual(stats.foreign_bad, 1)
        self.assertIn("kann DsRunner nicht anzeigen", self.messages("warn"))

    def test_lost_manifest(self):
        # files the tool wrote stay its own even when the manifest is gone (e.g. an interrupted run)
        self.art("Games/NDS.bmp").unlink()
        self.art("Games/NDS/Mario Kart DS.bmp").unlink()
        self.run_updater()
        (self.sd / "_dsrunner" / "art" / ".dsrunner-art.json").unlink()
        (self.sd / "_dsrunner" / "tiles" / ".dsrunner-art.json").unlink()
        stats, _ = self.run_updater()
        self.assertEqual(stats.foreign + stats.tiles.foreign, 0)
        # the game keeps its picture, the generated folder picture is simply made again
        self.assertEqual((stats.unchanged, stats.generated, stats.tiles.unchanged), (1, 1, 1))
        self.assertIn("/_dsrunner/art/games/nds/mario kart ds.bmp", self.manifest())


class BrokenSource(ArtSource):
    """A source whose downloads fail in unexpected ways."""
    id = "broken"
    label = "Kaputt"
    art_types = ("title", "snap", "boxart")

    def __init__(self, error: Exception):
        super().__init__(None)
        self.error = error

    def candidates(self, game, art_type):
        return [Candidate(f"https://example.invalid/{art_type}.png", art_type, self.id, "kaputte Quelle")]

    def load(self, candidate):
        if isinstance(self.error, Exception):
            raise self.error
        # an image that can't be converted
        return mock.Mock(spec=Image.Image, mode="RGB", size=(0, 0))


class ReportedGamesTests(CardTestCase):
    """The NDS games that got no picture at all on the owner's card (DSi enhanced, "[b]", long names)."""

    def setUp(self):
        super().setUp()
        for name, title, code, banner in REPORTED_GAMES:
            self.game(name, title, code, banner)
        self.names = [name.rsplit(".", 1)[0] for name, *_ in REPORTED_GAMES]

    def assert_every_game_has_pictures(self, stats, art_status: str):
        self.assertEqual((stats.missing, stats.errors, stats.tiles.missing, stats.tiles.errors), (0, 0, 0, 0),
                         self.logs)
        for name in self.names:
            with self.subTest(name=name):
                self.assertTrue(validate_bmp(self.art(f"Games/NDS/{name}.bmp").read_bytes()).ok)
                self.assertTrue(validate_bmp(self.tile(f"Games/NDS/{name}.bmp").read_bytes(), (64, 64)).ok)
                source = self.art_source(f"Games/NDS/{name}.bmp")
                self.assertTrue(source.startswith(art_status), source)

    def test_found_by_game_code(self):
        for name in ("Pokemon - Black Version (USA, Europe) (NDSi Enhanced)",
                     "Pokemon - Black Version 2 (USA, Europe) (NDSi Enhanced)",
                     "Plants vs. Zombies (Europe) (En,Fr,De,Es,It,Nl) (MSL)",
                     "Adventure Time - Hey Ice King! Why'd You Steal Our Garbage!! (USA)"):
            self.files[thumb(DS, "Named_Titles", name)] = png_bytes((256, 384), (200, 40, 40))
            self.files[thumb(DS, "Named_Boxarts", name)] = png_bytes((512, 460), (40, 40, 200))
        stats, _ = self.run_updater()
        self.assert_every_game_has_pictures(stats, "libretro:title:")
        self.assertIn("Game-Code IRBO", self.messages())

    def test_nothing_online_gives_the_icon(self):
        stats, _ = self.run_updater()
        self.assertEqual((stats.icons, stats.tiles.icons), (4, 4))
        self.assert_every_game_has_pictures(stats, "icon:1")

    def test_broken_downloads_still_give_the_icon(self):
        for error in (RuntimeError("kaputt"), http.client.IncompleteRead(b"x"), None):
            with self.subTest(error=error):
                shutil.rmtree(self.sd / "_dsrunner", ignore_errors=True)
                stats, _ = self.run_updater(sources=[BrokenSource(error)])
                self.assert_every_game_has_pictures(stats, "icon:1")
                self.assertIn("unbrauchbar", self.messages())

    def test_icon_switched_off_says_why(self):
        stats, _ = self.run_updater(offline=True, icon_fallback=False)
        self.assertEqual(stats.missing, 4)
        self.assertIn("(ohne Download; Icon-Ersatzbild ist ausgeschaltet)", self.messages("warn"))


class NetworkErrorTests(unittest.TestCase):
    def test_broken_transfer_is_a_network_error(self):
        with tempfile.TemporaryDirectory() as folder:
            client = HttpClient(Path(folder), retries=0)
            with mock.patch("dsrunnerart.net.urllib.request.urlopen",
                            side_effect=http.client.IncompleteRead(b"partial")):
                with self.assertRaises(NetworkError):
                    client.fetch("https://raw.githubusercontent.com/x.png")


if __name__ == "__main__":
    unittest.main()
