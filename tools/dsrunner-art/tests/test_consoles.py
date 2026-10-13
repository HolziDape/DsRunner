import json
import re
import tempfile
import unittest
from pathlib import Path

from tests.helpers import write

from dsrunnerart import consoles
from dsrunnerart.consoles import (CONSOLES, EXTRA_SYSTEMS, MAX_CUSTOM_CONSOLES, ROM_EXTENSIONS, SYSTEMS,
                                  find_by_folder_name, hero_color_for, hero_from_tile, parse_systems_json,
                                  set_custom_systems, system_for_file, tile_color_for)
from dsrunnerart.identify import CrcCache, identify
from dsrunnerart.romheader import copier_header_size
from dsrunnerart.sdcard import read_systems_json, scan

REPO = Path(__file__).resolve().parents[3]
CONSOLES_H = REPO / "arm9" / "source" / "themes" / "dsrunner" / "DsRunnerConsoles.h"
EMULATORS_H = REPO / "arm9" / "source" / "romBrowser" / "Theme" / "dsrunner" / "DsRunnerEmulators.h"


def _header(path: Path) -> str:
    if not path.is_file():
        raise unittest.SkipTest(f"{path} not found (tool outside the repository)")
    return path.read_text(encoding="utf-8")


def _unescape(text: str) -> str:
    """C string literal with \\xNN escapes (UTF-8 bytes) -> str."""
    data = re.sub(r"\\x([0-9A-Fa-f]{2})", lambda m: chr(int(m.group(1), 16)), text)
    return data.encode("latin-1").decode("utf-8")


class LauncherParityTests(unittest.TestCase):
    """consoles.py mirrors DsRunnerConsoles.h; these tests read the header so both stay in sync."""

    def test_consoles_match_header(self):
        header = _header(CONSOLES_H)
        pattern = re.compile(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*Color\(\s*(\d+),\s*(\d+),\s*(\d+)\),'
                             r'\s*Color\(\s*(\d+),\s*(\d+),\s*(\d+)\)\s*\}')
        launcher = []
        for match in pattern.finditer(header):
            folder = _unescape(match.group(1))
            if "Favoriten" in folder:
                continue  # virtual folder of the launcher, no console
            numbers = [int(value) for value in match.groups()[3:]]
            launcher.append(consoles.Console(folder, match.group(2), match.group(3),
                                             tuple(numbers[:3]), tuple(numbers[3:])))
        self.assertTrue(launcher)
        self.assertEqual(launcher, list(CONSOLES))

    def test_extensions_match_header(self):
        header = _header(CONSOLES_H)
        table = header[header.index("kExtensions[]"):]
        table = table[:table.index("};")]
        launcher = {ext: folder for ext, folder in re.findall(r'\{\s*"(\w+)",\s*"(\w+)"\s*\}', table)}
        self.assertEqual(set(launcher), set(ROM_EXTENSIONS))
        for system in SYSTEMS:
            for extension in system.extensions:
                self.assertEqual(launcher[extension], system.console_folder, extension)
                self.assertIsNotNone(find_by_folder_name(system.console_folder))

    def test_every_console_with_roms_has_an_emulator(self):
        header = _header(EMULATORS_H)
        table = header[header.index("kEmulators[]"):]
        table = table[:table.index("};")]
        consoles_with_emulator = set()
        for console_list in re.findall(r'\{\s*"[^"]+",\s*\{([^}]*)\}', table):
            consoles_with_emulator.update(re.findall(r'"(\w+)"', console_list))
        folders = {system.console_folder for system in SYSTEMS} - {"NDS"}
        self.assertEqual(folders - consoles_with_emulator, set())

    def test_system_names(self):
        for system in SYSTEMS + EXTRA_SYSTEMS:
            self.assertTrue(system.libretro_names)
            for name in system.libretro_names:
                # libretro-thumbnails repositories are "<Maker> - <System>" with spaces as underscores
                self.assertRegex(name, r"^[A-Za-z0-9]+ - [^/]+$")
        extensions = [ext for system in SYSTEMS + EXTRA_SYSTEMS for ext in system.extensions]
        self.assertEqual(len(extensions), len(set(extensions)))

    def test_system_for_file(self):
        cases = {"Tetris.gb": "gb", "Pokemon Gelb.SGB": "gb", "Zelda.gbc": "gbc", "Metroid.nes": "nes",
                 "Zelda no Densetsu.fds": "fds", "Mario.smc": "snes", "Sonic.gen": "md", "Sonic.smd": "md",
                 "Sonic.md": "md", "Alex Kidd.sms": "sms", "Shinobi.gg": "gg", "Bonk.pce": "pce", "x.sgx": "sgx"}
        for name, key in cases.items():
            with self.subTest(name=name):
                self.assertEqual(system_for_file(name).key, key)
        self.assertIsNone(system_for_file("Gunpey.ws"))  # only with a systems.json console
        self.assertIsNone(system_for_file("readme.txt"))


class SystemsJsonTests(unittest.TestCase):
    def tearDown(self):
        set_custom_systems([])

    def test_override_and_new_console(self):
        customs, warnings = parse_systems_json({"systems": {
            "SMS": {"label": "MS", "color": [224, 64, 128]},
            "WS": {"name": "WonderSwan", "color": "#C08020", "extensions": [".WS", "wsc"],
                   "emulator": "/_nds/NitroSwan.nds"},
        }})
        self.assertEqual(warnings, [])
        sms, ws = customs
        self.assertEqual(sms.console.label, "MS")
        self.assertEqual(sms.console.full_name, "Sega Master System")  # from the built-in list
        self.assertEqual(sms.console.tile, (224, 64, 128))
        self.assertEqual(sms.console.hero, hero_from_tile((224, 64, 128)))
        self.assertEqual(sms.extensions, ("sms",))  # kept from the built-in console
        self.assertEqual(ws.console.label, "WS")  # folder name, like an unknown folder
        self.assertEqual(ws.console.tile, (0xC0, 0x80, 0x20))
        self.assertEqual(ws.extensions, ("ws", "wsc"))
        self.assertEqual(ws.emulator, "/_nds/NitroSwan.nds")

        set_custom_systems(customs)
        self.assertEqual(find_by_folder_name("ws").full_name, "WonderSwan")
        self.assertEqual(find_by_folder_name("sms").label, "MS")
        self.assertEqual(hero_color_for(["WS"]), ws.console.hero)
        self.assertEqual(tile_color_for(["SMS", "Sub"]), (224, 64, 128))
        # known extension -> libretro system without a "libretro" entry
        self.assertEqual(system_for_file("Gunpey.ws").libretro_names[0], "Bandai - WonderSwan")
        self.assertEqual(system_for_file("Gunpey.wsc").key, "wsc")
        self.assertEqual(system_for_file("Alex.sms").key, "sms")

        set_custom_systems([])
        self.assertIsNone(system_for_file("Gunpey.ws"))
        self.assertEqual(find_by_folder_name("SMS").label, "SMS")

    def test_libretro_entry(self):
        customs, _ = parse_systems_json({"systems": {
            "NGP": {"extensions": ["ngp", "ngc"], "libretro": "SNK - Neo Geo Pocket Color"},
            "MSX": {"extensions": ["rom"], "libretro": ["Microsoft - MSX", "Microsoft - MSX2"]},
            "Sega8": {"extensions": ["sms", "gg"]},
            "Mystery": {"extensions": ["xyz"]},
        }})
        set_custom_systems(customs)
        self.assertEqual(system_for_file("a.ngp").key, "ngc")  # known libretro system keeps its key
        msx = system_for_file("Penguin Adventure.rom")
        self.assertEqual(msx.libretro_names, ("Microsoft - MSX", "Microsoft - MSX2"))
        self.assertEqual(msx.console_folder, "MSX")
        self.assertEqual(system_for_file("a.gg").key, "gg")  # built-in mapping stays
        mystery = system_for_file("a.xyz")
        self.assertEqual(mystery.libretro_names, ())
        self.assertEqual(mystery.console_folder, "Mystery")

    def test_invalid_entries(self):
        customs, warnings = parse_systems_json({"systems": {
            "": {},
            "A" * 64: {},
            "NoObject": "x",
            "BadColor": {"color": "#12345", "heroColor": [1, 2, 300], "extensions": ["toolongext", "a", "b", "c", "d", "e"]},
        }})
        self.assertEqual([custom.console.folder_name for custom in customs], ["BadColor"])
        bad = customs[0]
        self.assertEqual(bad.console.tile, consoles.UNKNOWN_TILE)
        self.assertEqual(bad.console.hero, consoles.UNKNOWN_HERO)
        self.assertEqual(bad.extensions, ("a", "b", "c", "d"))
        self.assertEqual(len(warnings), 3 + 2 + 2)

        for data in (None, [], {"systems": []}, {"other": {}}):
            customs, warnings = parse_systems_json(data)
            self.assertEqual(customs, [])
            self.assertEqual(len(warnings), 1)

    def test_entry_limit(self):
        data = {"systems": {f"C{i}": {"extensions": [f"e{i}"]} for i in range(MAX_CUSTOM_CONSOLES + 4)}}
        customs, warnings = parse_systems_json(data)
        self.assertEqual(len(customs), MAX_CUSTOM_CONSOLES)
        self.assertEqual(len(warnings), 1)


class SystemsJsonScanTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)

    def tearDown(self):
        set_custom_systems([])
        self._tmp.cleanup()

    def write_systems(self, data) -> None:
        write(self.root / "_dsrunner" / "systems.json", json.dumps(data).encode())

    def test_scan_with_custom_console(self):
        self.write_systems({"systems": {"WS": {"color": "#C08020", "extensions": ["ws"],
                                               "emulator": "/_nds/NitroSwan.nds"}}})
        write(self.root / "Games" / "WS" / "Gunpey (Japan).ws", b"\0" * 64)
        write(self.root / "Games" / "NES" / "Zelda.nes", b"\0" * 64)
        write(self.root / "Games" / "NES" / "readme.md.txt", b"")
        result = scan(self.root)
        self.assertEqual(len(result.warnings), 1)  # the emulator is missing
        self.assertIn("NitroSwan", result.warnings[0])
        games = {entry.name: entry for entry in result.games}
        self.assertEqual(set(games), {"Gunpey (Japan).ws", "Zelda.nes"})
        self.assertEqual(games["Gunpey (Japan).ws"].system.key, "ws")
        self.assertEqual(hero_color_for(games["Gunpey (Japan).ws"].parent_folders), hero_from_tile((0xC0, 0x80, 0x20)))

        # the next scan without the file forgets the console
        (self.root / "_dsrunner" / "systems.json").unlink()
        result = scan(self.root)
        self.assertEqual({entry.name for entry in result.games}, {"Zelda.nes"})
        self.assertEqual(result.warnings, [])

    def test_invalid_file_is_ignored(self):
        write(self.root / "_dsrunner" / "systems.json", b"{ not json")
        customs, warnings = read_systems_json(self.root)
        self.assertEqual(customs, [])
        self.assertEqual(len(warnings), 1)
        write(self.root / "_dsrunner" / "systems.json", b" " * (consoles.MAX_SYSTEMS_FILE_SIZE + 1))
        customs, warnings = read_systems_json(self.root)
        self.assertEqual(customs, [])
        self.assertEqual(len(warnings), 1)
        self.assertEqual(read_systems_json(self.root / "missing"), ([], []))

    def test_identify_without_libretro_system(self):
        self.write_systems({"systems": {"Mystery": {"extensions": ["xyz"]}}})
        write(self.root / "Games" / "Mystery" / "Game (Europe).xyz", b"\0" * 64)
        entry = scan(self.root).games[0]

        class OfflineCatalog:
            def dat(self, name):
                return None

            def thumbnails(self, name):
                return None

        info = identify(entry, OfflineCatalog(), ["Europe"], CrcCache(None))
        self.assertEqual(info.names, [])

    def test_fds_header_is_skipped_for_the_checksum(self):
        path = write(self.root / "Games" / "NES" / "Zelda.fds", b"FDS\x1a" + b"\0" * 12 + b"\x01*NINTENDO-HVC*")
        self.assertEqual(copier_header_size(path, "fds", path.stat().st_size), 16)
        path = write(self.root / "Games" / "NES" / "Raw.fds", b"\x01*NINTENDO-HVC*")
        self.assertEqual(copier_header_size(path, "fds", path.stat().st_size), 0)


if __name__ == "__main__":
    unittest.main()
