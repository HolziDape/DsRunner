"""End-to-end runs on a generated SD card with a fake network."""
import contextlib
import io
import json
import tempfile
import unittest
import urllib.parse
from pathlib import Path

from tests.helpers import DAT_GB, DAT_NDS, cleanup_env, gba_rom, nds_rom, png_bytes, write

from dsrunnerart import cli
from dsrunnerart.imaging import validate_bmp
from dsrunnerart.net import HttpClient, NetworkError
from dsrunnerart.updater import Options, Updater, check_card

RAW = "https://raw.githubusercontent.com/libretro-thumbnails/"


def thumb(repo: str, folder: str, name: str) -> str:
    return f"{RAW}{repo}/master/{folder}/{urllib.parse.quote(name, safe='')}.png"


class FakeClient(HttpClient):
    """Serves dats and thumbnails from memory; GitHub API and GameTDB are 'blocked'."""

    def __init__(self, cache_dir: Path, files: dict, offline: bool = False):
        super().__init__(cache_dir, offline=offline, retries=0)
        self.files = files
        self.requests: list[str] = []

    def _open(self, url, headers=None):
        if self.offline:
            raise NetworkError("offline")
        self.requests.append(url)
        if "api.github.com" in url:
            raise NetworkError("api.github.com: HTTP 403")
        if "gametdb.com" in url:
            raise NetworkError("art.gametdb.com: blocked")
        for key, value in self.files.items():
            if urllib.parse.unquote(key) == urllib.parse.unquote(url):
                return value
        return None


def dat_url(system: str) -> str:
    return ("https://raw.githubusercontent.com/libretro/libretro-database/master/metadat/no-intro/"
            + urllib.parse.quote(system) + ".dat")


class UpdaterTests(unittest.TestCase):
    def setUp(self):
        cleanup_env()
        self.temp = tempfile.TemporaryDirectory()
        base = Path(self.temp.name)
        self.sd = base / "sd"
        self.cache = base / "cache"
        write(self.sd / "_pico" / "settings.json", json.dumps({"libraryPath": "/Games"}).encode())
        write(self.sd / "Games" / "NDS" / "Mario Kart DS.nds", nds_rom("MARIOKART DS", "AMCP", "Mario Kart DS\nNintendo"))
        write(self.sd / "Games" / "NDS" / "Homebrew.nds", nds_rom("HOMEBREW", "####", "My Homebrew\nby me"))
        write(self.sd / "Games" / "NDS" / "Unknown Game.nds", nds_rom("UNKNOWN", "ZZZZ"))
        write(self.sd / "Games" / "GB" / "Tetris (World) (Rev 1).gb", b"\0" * 1024)
        write(self.sd / "Games" / "GBA" / "Golden Sun.gba", gba_rom("GOLDEN_SUN_A", "AGSD"))
        self.files = {
            dat_url("Nintendo - Nintendo DS"): DAT_NDS.encode(),
            dat_url("Nintendo - Game Boy"): DAT_GB.encode(),
            thumb("Nintendo_-_Nintendo_DS", "Named_Titles", "Mario Kart DS (Europe) (En,Fr,De,Es,It)"):
                png_bytes((256, 384), (255, 0, 0)),
            thumb("Nintendo_-_Nintendo_DS", "Named_Boxarts", "Mario Kart DS (Europe) (En,Fr,De,Es,It)"):
                png_bytes((512, 459), (0, 0, 255)),
            # symlink as raw.githubusercontent.com serves it
            thumb("Nintendo_-_Game_Boy", "Named_Titles", "Tetris (World) (Rev 1)"): b"Tetris (Japan) (En).png",
            thumb("Nintendo_-_Game_Boy", "Named_Titles", "Tetris (Japan) (En)"): png_bytes((160, 144), (0, 255, 0)),
        }
        self.logs: list[tuple[str, str]] = []

    def tearDown(self):
        self.temp.cleanup()

    def run_updater(self, offline=False, **options):
        client = FakeClient(self.cache, self.files, offline=offline)
        self.logs = []
        updater = Updater(self.sd, Options(cache_dir=self.cache, workers=2, download=not offline, **options),
                          log=lambda message, level="info": self.logs.append((level, message)), client=client)
        return updater.run(), client

    def art(self, relative: str) -> Path:
        return self.sd / "_dsrunner" / "art" / relative

    def manifest(self) -> dict:
        return json.loads(self.art(".dsrunner-art.json").read_text(encoding="utf-8"))["artworks"]

    def test_full_run_and_incremental(self):
        stats, client = self.run_updater()
        self.assertEqual(stats.errors, 0, self.logs)
        self.assertEqual(stats.downloaded, 2)   # Mario Kart DS (game code), Tetris (name + symlink)
        self.assertEqual(stats.icons, 1)        # Homebrew
        self.assertEqual(stats.generated, 3)    # NDS, GB, GBA folders
        self.assertEqual(stats.missing, 2)      # Unknown Game, Golden Sun (no GBA dat/thumbnail)
        for relative in ("Games/NDS.bmp", "Games/GB.bmp", "Games/GBA.bmp", "Games/NDS/Mario Kart DS.bmp",
                         "Games/NDS/Homebrew.bmp", "Games/GB/Tetris (World) (Rev 1).bmp"):
            with self.subTest(relative=relative):
                self.assertTrue(validate_bmp(self.art(relative).read_bytes()).ok)
        self.assertFalse(self.art("Games/NDS/Unknown Game.bmp").exists())

        # the title screen of Mario Kart DS is the red top half of the dual-screen image
        from PIL import Image, ImageStat
        image = Image.open(self.art("Games/NDS/Mario Kart DS.bmp")).convert("RGB")
        red, green, blue = ImageStat.Stat(image).mean
        self.assertGreater(red, blue + 50)
        manifest = self.manifest()
        self.assertIn("/_dsrunner/art/games/nds/mario kart ds.bmp", manifest)
        self.assertTrue(manifest["/_dsrunner/art/games/nds/mario kart ds.bmp"]["source"].startswith("libretro:title:"))

        # unreachable sources are reported once and skipped
        warnings = [message for level, message in self.logs if "GameTDB" in message]
        self.assertEqual(len(warnings), 1)

        # second run: nothing to download again, nothing rewritten
        mtime = self.art("Games/NDS/Mario Kart DS.bmp").stat().st_mtime_ns
        stats, client = self.run_updater()
        self.assertEqual(stats.downloaded + stats.generated + stats.icons, 0)
        self.assertEqual(stats.unchanged, 6)
        self.assertEqual(self.art("Games/NDS/Mario Kart DS.bmp").stat().st_mtime_ns, mtime)
        self.assertFalse([url for url in client.requests if "Named_" in url and "Mario" in url])

    def test_foreign_files_stay(self):
        own = write(self.art("Games/NDS/Mario Kart DS.bmp"), b"my own picture")
        stats, _ = self.run_updater(force=True)
        self.assertEqual(own.read_bytes(), b"my own picture")
        self.assertEqual(stats.foreign, 1)

    def test_overrides_win(self):
        override = self.sd / "_dsrunner" / "art-override"
        write(override / "Games" / "NDS" / "Unknown Game.png", png_bytes((640, 480), (0, 128, 255)))
        write(override / "homebrew.JPG", b"not really a jpeg")
        own = write(self.art("Games/NDS/Mario Kart DS.bmp"), b"my own picture")
        write(override / "Mario Kart DS.png", png_bytes((100, 100), (255, 255, 0)))
        stats, _ = self.run_updater()
        self.assertEqual(stats.overrides, 2)
        self.assertEqual(stats.errors, 1)  # the broken "jpeg"
        self.assertTrue(validate_bmp(self.art("Games/NDS/Unknown Game.bmp").read_bytes()).ok)
        # an explicit override replaces even a hand-placed file
        self.assertTrue(validate_bmp(own.read_bytes()).ok)
        # unchanged override -> unchanged artwork
        stats, _ = self.run_updater()
        self.assertEqual(stats.overrides, 0)

    def test_force_with_other_type(self):
        self.run_updater()
        before = self.art("Games/NDS/Mario Kart DS.bmp").read_bytes()
        stats, _ = self.run_updater(art_type="boxart")
        self.assertEqual(self.art("Games/NDS/Mario Kart DS.bmp").read_bytes(), before)
        stats, _ = self.run_updater(art_type="boxart", force=True)
        self.assertNotEqual(self.art("Games/NDS/Mario Kart DS.bmp").read_bytes(), before)
        source = self.manifest()["/_dsrunner/art/games/nds/mario kart ds.bmp"]["source"]
        self.assertTrue(source.startswith("libretro:boxart:"))

    def test_clean_removes_only_own_orphans(self):
        self.run_updater()
        foreign = write(self.art("Games/GB/Old Game.bmp"), b"hand made")
        (self.sd / "Games" / "GB" / "Tetris (World) (Rev 1).gb").unlink()
        stats, _ = self.run_updater(clean=True)
        self.assertEqual(stats.removed, 1)
        self.assertFalse(self.art("Games/GB/Tetris (World) (Rev 1).bmp").exists())
        self.assertTrue(foreign.exists())
        self.assertNotIn("/_dsrunner/art/games/gb/tetris (world) (rev 1).bmp", self.manifest())

    def test_dry_run_writes_nothing(self):
        stats, _ = self.run_updater(dry_run=True)
        self.assertGreater(stats.downloaded, 0)
        self.assertFalse((self.sd / "_dsrunner").exists())

    def test_offline(self):
        stats, client = self.run_updater(offline=True)
        self.assertEqual(client.requests, [])
        self.assertEqual(stats.downloaded, 0)
        self.assertEqual(stats.generated, 3)
        # with the banner icon even an offline run gives every NDS game with an icon a picture
        self.assertEqual(stats.icons, 2)

    def test_path_too_long(self):
        write(self.sd / "Games" / "NDS" / ("x" * 230 + ".nds"), nds_rom("X", "####"))
        stats, _ = self.run_updater(offline=True)
        self.assertEqual(stats.errors, 1)

    def test_check_card_and_cli(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            code = cli.main(["--sd", str(self.sd), "--offline", "--cache", str(self.cache), "--no-icon"])
        self.assertIn("ohne Download", output.getvalue())
        self.assertEqual(code, 0)
        self.assertTrue(self.art("Games/NDS.bmp").exists())
        self.assertFalse(self.art("Games/NDS/Homebrew.bmp").exists())
        write(self.art("Games/broken.bmp"), b"BM" + b"\0" * 100)
        messages = []
        self.assertEqual(check_card(self.sd, lambda message, level="info": messages.append(message)), 1)
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
            self.assertEqual(cli.main(["--sd", str(self.sd), "--check"]), 1)


if __name__ == "__main__":
    unittest.main()
