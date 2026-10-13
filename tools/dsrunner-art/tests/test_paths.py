import json
import tempfile
import unittest
from pathlib import Path

from tests.helpers import gba_rom, nds_rom, write

from dsrunnerart.sdcard import (ART_FOLDER, TILE_FOLDER, Entry, art_path, art_path_for_entry, read_library_path, scan,
                                skip_drive, tile_path_for_entry)


def request_art_reference(folder_path: str, file_name: str, is_folder: bool):
    """Line by line transliteration of DsRunnerFileInfoCardView::RequestArt on UTF-8 bytes."""
    art_folder = b"/_dsrunner/art"
    folder = folder_path.encode()
    colon = folder.find(b":")
    if colon >= 0:
        folder = folder[colon + 1:]
    folder_length = len(folder)
    while folder_length > 0 and folder[folder_length - 1:folder_length] == b"/":
        folder_length -= 1
    name = file_name.encode()
    name_length = len(name)
    extension = -1 if is_folder else name.rfind(b".")
    if extension > 0:  # strrchr found it and it's not the first character
        name_length = extension
    if len(art_folder) + folder_length + 1 + name_length + 4 >= 256:
        return None
    return (art_folder + folder[:folder_length] + b"/" + name[:name_length] + b".bmp").decode()


CASES = [
    ("fat:/Games/NDS", "Mario Kart DS.nds", False, "/_dsrunner/art/Games/NDS/Mario Kart DS.bmp"),
    ("/Games/NDS/", "Mario Kart DS.nds", False, "/_dsrunner/art/Games/NDS/Mario Kart DS.bmp"),
    ("fat:/Games", "NDS", True, "/_dsrunner/art/Games/NDS.bmp"),
    ("fat:/Games/", "NDS", True, "/_dsrunner/art/Games/NDS.bmp"),
    ("/Games/GBA/Rennen", "Golden Sun.gba", False, "/_dsrunner/art/Games/GBA/Rennen/Golden Sun.bmp"),
    ("/Games/GBA", "Rennen", True, "/_dsrunner/art/Games/GBA/Rennen.bmp"),
    # dots: only the last extension goes, folders keep theirs
    ("/Games/NDS", "Pokemon Platin.v1.2.nds", False, "/_dsrunner/art/Games/NDS/Pokemon Platin.v1.2.bmp"),
    ("/Games", "Spiele v2.0", True, "/_dsrunner/art/Games/Spiele v2.0.bmp"),
    ("/Games/NDS", "Mr. Driller.nds", False, "/_dsrunner/art/Games/NDS/Mr. Driller.bmp"),
    # a leading dot is no extension, a missing extension keeps the name
    ("/Games/NDS", ".hidden", False, "/_dsrunner/art/Games/NDS/.hidden.bmp"),
    ("/Games/NDS", "README", False, "/_dsrunner/art/Games/NDS/README.bmp"),
    # entries in the SD root
    ("fat:/", "Games", True, "/_dsrunner/art/Games.bmp"),
    ("/", "game.nds", False, "/_dsrunner/art/game.bmp"),
    # non-ASCII names (UTF-8 on the card)
    ("/Games/NDS", "Pokémon Weiße Edition.nds", False, "/_dsrunner/art/Games/NDS/Pokémon Weiße Edition.bmp"),
]


class ArtPathTests(unittest.TestCase):
    def test_examples(self):
        for folder, name, is_folder, expected in CASES:
            with self.subTest(folder=folder, name=name):
                self.assertEqual(art_path(folder, name, is_folder), expected)
                self.assertEqual(request_art_reference(folder, name, is_folder), expected)

    def test_entry_paths(self):
        self.assertEqual(art_path_for_entry("/Games/NDS/Mario Kart DS.nds", False),
                         "/_dsrunner/art/Games/NDS/Mario Kart DS.bmp")
        self.assertEqual(art_path_for_entry("/Games/NDS", True), "/_dsrunner/art/Games/NDS.bmp")
        self.assertEqual(art_path_for_entry("/Games/NDS/", True), "/_dsrunner/art/Games/NDS.bmp")
        self.assertEqual(art_path_for_entry("fat:/Games/GBA/Rennen/Golden Sun.gba", False),
                         "/_dsrunner/art/Games/GBA/Rennen/Golden Sun.bmp")
        self.assertEqual(art_path_for_entry("/Games", True), "/_dsrunner/art/Games.bmp")

    def test_length_limit_matches_launcher(self):
        folder = "/Games/NDS"
        # 14 + 10 + 1 + n + 4 >= 256 fails -> n = 226 is the longest name that fits
        fits = "a" * 226 + ".nds"
        too_long = "a" * 227 + ".nds"
        self.assertIsNotNone(art_path(folder, fits, False))
        self.assertIsNone(art_path(folder, too_long, False))
        self.assertIsNone(request_art_reference(folder, too_long, False))
        # bytes, not characters: "ä" is two bytes in UTF-8
        self.assertIsNone(art_path(folder, "ä" * 114 + ".nds", False))
        self.assertIsNotNone(art_path(folder, "ä" * 113 + ".nds", False))

    def test_matches_reference_for_generated_names(self):
        names = ["a.b.c.nds", "x", ".", "..", "a.", "Folder.With.Dots", "Name (Europe) (En,Fr).nds", "trail.gba"]
        folders = ["/", "fat:/", "/Games", "/Games/", "sd:/Games/GBA//", "/A/B/C/D"]
        for folder in folders:
            for name in names:
                for is_folder in (False, True):
                    with self.subTest(folder=folder, name=name, is_folder=is_folder):
                        self.assertEqual(art_path(folder, name, is_folder),
                                         request_art_reference(folder, name, is_folder))

    def test_skip_drive(self):
        self.assertEqual(skip_drive("fat:/Games"), "/Games")
        self.assertEqual(skip_drive("/Games"), "/Games")


class TilePathTests(unittest.TestCase):
    """Tile images: the artwork rule with /_dsrunner/tiles as root, games only."""

    def test_examples(self):
        cases = [
            ("/Games/NDS/Mario Kart DS.nds", "/_dsrunner/tiles/Games/NDS/Mario Kart DS.bmp"),
            ("fat:/Games/GBA/Rennen/Golden Sun.gba", "/_dsrunner/tiles/Games/GBA/Rennen/Golden Sun.bmp"),
            ("/Games/NDS/Pokemon Platin.v1.2.nds", "/_dsrunner/tiles/Games/NDS/Pokemon Platin.v1.2.bmp"),
            ("/Games/NDS/Pokemon - Black Version 2 (USA, Europe) (NDSi Enhanced) [b].nds",
             "/_dsrunner/tiles/Games/NDS/Pokemon - Black Version 2 (USA, Europe) (NDSi Enhanced) [b].bmp"),
            ("/game.nds", "/_dsrunner/tiles/game.bmp"),
        ]
        for path, expected in cases:
            with self.subTest(path=path):
                self.assertEqual(tile_path_for_entry(path, False), expected)
                # same rule as the artworks, only the root differs
                self.assertEqual(expected.replace(TILE_FOLDER, ART_FOLDER, 1), art_path_for_entry(path, False))

    def test_folders_have_no_tile(self):
        self.assertIsNone(tile_path_for_entry("/Games/NDS", True))
        entry = Entry("/Games/NDS", Path("x"), True, 0)
        self.assertIsNone(entry.tile_sd_path)
        self.assertEqual(entry.art_sd_path, "/_dsrunner/art/Games/NDS.bmp")

    def test_length_limit(self):
        # 16 + 10 + 1 + n + 4 >= 256 fails -> n = 224 is the longest name that fits
        self.assertIsNotNone(tile_path_for_entry("/Games/NDS/" + "a" * 224 + ".nds", False))
        self.assertIsNone(tile_path_for_entry("/Games/NDS/" + "a" * 225 + ".nds", False))

    def test_entry_keys(self):
        game = Entry("/Games/NDS/Mario Kart DS.nds", Path("x"), False, 1, ["NDS"])
        self.assertEqual(game.tile_sd_path, "/_dsrunner/tiles/Games/NDS/Mario Kart DS.bmp")
        self.assertEqual(game.stem, "Mario Kart DS")
        self.assertEqual(game.relative_key, "games/nds/mario kart ds")
        folder = Entry("/Games/Spiele v2.0", Path("x"), True, 0)
        self.assertEqual(folder.stem, "Spiele v2.0")
        self.assertEqual(folder.relative_key, "games/spiele v2.0")


class LibraryTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_library_path_from_settings(self):
        self.assertEqual(read_library_path(self.root), "/Games")
        write(self.root / "_pico" / "settings.json", json.dumps({"libraryPath": "fat:/Spiele"}).encode())
        self.assertEqual(read_library_path(self.root), "/Spiele")
        write(self.root / "_pico" / "settings.json", b"\xef\xbb\xbf{\"libraryPath\": \"\"}")
        self.assertEqual(read_library_path(self.root), "")
        write(self.root / "_pico" / "settings.json", b"{broken")
        self.assertEqual(read_library_path(self.root), "/Games")

    def test_scan_library(self):
        write(self.root / "Games" / "NDS" / "Mario Kart DS.nds", nds_rom("MARIOKART DS", "AMCP"))
        write(self.root / "Games" / "NDS" / "Mario Kart DS.sav", b"\0")
        write(self.root / "Games" / "GBA" / "Rennen" / "Golden Sun.gba", gba_rom("GOLDEN_SUN_A", "AGSD"))
        write(self.root / "Games" / "GBA" / ".hidden.gba", gba_rom("X", "AAAA"))
        write(self.root / "Games" / "loose.gb", b"\0" * 16)
        write(self.root / "_dsrunner" / "art" / "x.bmp", b"")
        result = scan(self.root)
        self.assertFalse(result.root_mode)
        self.assertEqual(result.library_path, "/Games")
        paths = {(entry.sd_path, entry.is_folder) for entry in result.entries}
        self.assertEqual(paths, {
            ("/Games/GBA", True), ("/Games/GBA/Rennen", True), ("/Games/GBA/Rennen/Golden Sun.gba", False),
            ("/Games/NDS", True), ("/Games/NDS/Mario Kart DS.nds", False), ("/Games/loose.gb", False),
        })
        game = next(entry for entry in result.entries if entry.name == "Golden Sun.gba")
        self.assertEqual(game.parent_folders, ["GBA", "Rennen"])
        self.assertEqual(game.system.key, "gba")
        self.assertEqual(game.art_sd_path, "/_dsrunner/art/Games/GBA/Rennen/Golden Sun.bmp")

    def test_scan_root_mode(self):
        write(self.root / "_pico" / "settings.json", json.dumps({"libraryPath": ""}).encode())
        write(self.root / "NDS" / "game.nds", nds_rom("GAME", "AMCP"))
        write(self.root / "_nds" / "other.nds", nds_rom("GAME", "AMCP"))
        (self.root / "System Volume Information").mkdir()
        result = scan(self.root)
        self.assertTrue(result.root_mode)
        self.assertEqual({entry.sd_path for entry in result.entries}, {"/NDS", "/NDS/game.nds"})
        self.assertEqual(result.entries[1].art_sd_path, "/_dsrunner/art/NDS/game.bmp")

    def test_missing_library_folder_falls_back_to_root(self):
        write(self.root / "NDS" / "game.nds", nds_rom("GAME", "AMCP"))
        result = scan(self.root)
        self.assertTrue(result.root_mode)
        self.assertTrue(result.warnings)


if __name__ == "__main__":
    unittest.main()
