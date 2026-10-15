"""Known consoles: folder names and colors (mirror of arm9/source/themes/dsrunner/DsRunnerConsoles.h)
plus the mapping from file extensions to the libretro / No-Intro system names.

/_dsrunner/systems.json (see docs/CONFIG.md) can add consoles and override the built-in ones. The launcher
reads it in DsRunnerSystems.cpp, this module mirrors that: parse_systems_json() + set_custom_systems()."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Iterable, Optional

Rgb = tuple[int, int, int]


@dataclass(frozen=True)
class Console:
    folder_name: str
    label: str
    full_name: str
    tile: Rgb
    hero: Rgb


# Keep in sync with DsRunnerConsoles::kConsoles (checked by tests/test_consoles.py).
CONSOLES: tuple[Console, ...] = (
    Console("NDS", "NDS", "Nintendo DS", (58, 92, 190), (44, 74, 168)),
    Console("DS", "NDS", "Nintendo DS", (58, 92, 190), (44, 74, 168)),
    Console("GBA", "GBA", "Game Boy Advance", (112, 64, 184), (92, 52, 156)),
    Console("GBC", "GBC", "Game Boy Color", (198, 92, 72), (168, 70, 54)),
    Console("GB", "GB", "Game Boy", (96, 144, 64), (74, 116, 48)),
    Console("NES", "NES", "Nintendo Entertainment System", (190, 48, 48), (160, 36, 36)),
    Console("FC", "NES", "Nintendo Entertainment System", (190, 48, 48), (160, 36, 36)),
    Console("SNES", "SNES", "Super Nintendo", (112, 92, 172), (92, 74, 148)),
    Console("SFC", "SNES", "Super Nintendo", (112, 92, 172), (92, 74, 148)),
    Console("SMS", "SMS", "Sega Master System", (44, 112, 192), (32, 92, 166)),
    Console("GG", "GG", "Sega Game Gear", (92, 92, 108), (72, 72, 88)),
    Console("GEN", "MD", "Sega Mega Drive", (76, 76, 90), (58, 58, 72)),
    Console("MD", "MD", "Sega Mega Drive", (76, 76, 90), (58, 58, 72)),
    Console("MEGADRIVE", "MD", "Sega Mega Drive", (76, 76, 90), (58, 58, 72)),
    Console("PCE", "PCE", "PC Engine", (150, 150, 166), (116, 116, 134)),
    Console("APPS", "APP", "Apps und Homebrew", (32, 160, 150), (24, 128, 120)),
    Console("HOMEBREW", "APP", "Apps und Homebrew", (32, 160, 150), (24, 128, 120)),
)

UNKNOWN_TILE: Rgb = (64, 74, 100)
UNKNOWN_HERO: Rgb = (40, 48, 70)

# Built-in console images of the DsRunner theme (_pico/themes/dsrunner/consoles/<key>.bmp, drawn by
# tools/console-art): folder name -> image key. Keep in sync with DsRunnerConsoleArt::kArtKeys
# (checked by tests/test_consoles.py).
ART_KEYS: dict[str, str] = {
    "NDS": "NDS", "DS": "NDS", "GBA": "GBA", "GBC": "GBC", "GB": "GB", "NES": "NES", "FC": "NES",
    "SNES": "SNES", "SFC": "SNES", "SMS": "SMS", "GG": "GG", "GEN": "MD", "MD": "MD", "MEGADRIVE": "MD",
    "PCE": "PCE", "APPS": "APPS", "HOMEBREW": "APPS", "★ Favoriten": "FAV", "• Suche": "SEARCH",
}
# image of consoles from systems.json without a built-in image, and of all other folders
ART_KEY_CUSTOM_CONSOLE = "CART"
ART_KEY_FOLDER = "FOLDER"


@dataclass(frozen=True)
class System:
    """A game system as libretro names it (No-Intro dat name = thumbnail repository name)."""
    key: str
    # libretro system names, the first one is the main one; the others are tried as well
    libretro_names: tuple[str, ...]
    # folder name of the console in DsRunnerConsoles (for colors)
    console_folder: str
    extensions: tuple[str, ...] = field(default=())


# Built-in systems of the launcher. The extensions are exactly DsRunnerConsoles::kExtensions
# (checked by tests/test_consoles.py), console_folder is the folder name given there.
SYSTEMS: tuple[System, ...] = (
    System("nds", ("Nintendo - Nintendo DS", "Nintendo - Nintendo DSi"), "NDS", ("nds", "dsi", "srl")),
    System("gba", ("Nintendo - Game Boy Advance",), "GBA", ("gba", "agb")),
    System("gbc", ("Nintendo - Game Boy Color", "Nintendo - Game Boy"), "GBC", ("gbc",)),
    System("gb", ("Nintendo - Game Boy", "Nintendo - Game Boy Color"), "GB", ("gb", "sgb")),
    System("nes", ("Nintendo - Nintendo Entertainment System",), "NES", ("nes",)),
    System("fds", ("Nintendo - Family Computer Disk System",), "NES", ("fds",)),
    System("snes", ("Nintendo - Super Nintendo Entertainment System",), "SNES", ("sfc", "smc")),
    System("sms", ("Sega - Master System - Mark III",), "SMS", ("sms",)),
    System("gg", ("Sega - Game Gear",), "GG", ("gg",)),
    System("md", ("Sega - Mega Drive - Genesis",), "MD", ("md", "gen", "smd")),
    System("pce", ("NEC - PC Engine - TurboGrafx 16",), "PCE", ("pce",)),
    System("sgx", ("NEC - PC Engine SuperGrafx", "NEC - PC Engine - TurboGrafx 16"), "PCE", ("sgx",)),
)

# Systems the launcher doesn't know by itself. A console of systems.json with one of these extensions
# gets the right libretro system without a "libretro" entry (DS emulators exist for all of them).
EXTRA_SYSTEMS: tuple[System, ...] = (
    System("ws", ("Bandai - WonderSwan", "Bandai - WonderSwan Color"), "WS", ("ws",)),
    System("wsc", ("Bandai - WonderSwan Color", "Bandai - WonderSwan"), "WSC", ("wsc",)),
    System("ngp", ("SNK - Neo Geo Pocket", "SNK - Neo Geo Pocket Color"), "NGP", ("ngp",)),
    System("ngc", ("SNK - Neo Geo Pocket Color", "SNK - Neo Geo Pocket"), "NGPC", ("ngc",)),
    System("a26", ("Atari - 2600",), "A26", ("a26",)),
    System("a78", ("Atari - 7800",), "A78", ("a78",)),
    System("col", ("Coleco - ColecoVision",), "COL", ("col",)),
    System("sg", ("Sega - SG-1000",), "SG", ("sg",)),
)

_SYSTEM_BY_EXTENSION = {ext: system for system in SYSTEMS for ext in system.extensions}
_EXTRA_BY_EXTENSION = {ext: system for system in EXTRA_SYSTEMS for ext in system.extensions}
_SYSTEM_BY_KEY = {system.key: system for system in SYSTEMS + EXTRA_SYSTEMS}

# Extensions of the built-in consoles (DsRunnerConsoles::kExtensions).
ROM_EXTENSIONS = frozenset(_SYSTEM_BY_EXTENSION)

# --- systems.json -------------------------------------------------------------------------------

# limits of DsRunnerSystems (DsRunnerSystems.h)
MAX_CUSTOM_CONSOLES = 16
MAX_EXTENSIONS = 4
MAX_SYSTEMS_FILE_SIZE = 8 * 1024
# buffer sizes in DsRunnerSystems::CustomConsole (bytes including the terminator)
_FOLDER_NAME_BUFFER = 64
_LABEL_BUFFER = 20
_FULL_NAME_BUFFER = 64
_EXTENSION_BUFFER = 8


@dataclass(frozen=True)
class CustomConsole:
    """One entry of systems.json, with the defaults of the launcher filled in."""
    console: Console
    extensions: tuple[str, ...]
    # libretro system names from "libretro" (empty: derived from the extensions)
    libretro_names: tuple[str, ...] = ()
    emulator: Optional[str] = None


_custom_consoles: tuple[CustomConsole, ...] = ()
_custom_system_by_extension: dict[str, System] = {}


def find_builtin_by_folder_name(name: str) -> Optional[Console]:
    """Case insensitive lookup in the built-in list only (DsRunnerConsoles::FindBuiltInByFolderName)."""
    lowered = name.lower()
    for console in CONSOLES:
        if console.folder_name.lower() == lowered:
            return console
    return None


def find_by_folder_name(name: str) -> Optional[Console]:
    """Case insensitive lookup like DsRunnerConsoles::FindByFolderName: systems.json first."""
    lowered = name.lower()
    for custom in _custom_consoles:
        if custom.console.folder_name.lower() == lowered:
            return custom.console
    return find_builtin_by_folder_name(name)


def art_key(folder_name: str) -> str:
    """Image key of a folder like DsRunnerConsoleArt::GetKey: its built-in console, CART for the other
    consoles of systems.json, FOLDER for everything else."""
    lowered = folder_name.lower()
    for name, key in ART_KEYS.items():
        if name.lower() == lowered:
            return key
    return ART_KEY_CUSTOM_CONSOLE if find_by_folder_name(folder_name) else ART_KEY_FOLDER


def _truncate_utf8(text: str, buffer_size: int) -> str:
    """Like strlcpy into a buffer of buffer_size bytes (without cutting a character in half)."""
    data = text.encode("utf-8")
    if len(data) < buffer_size:
        return text
    return data[:buffer_size - 1].decode("utf-8", "ignore")


def _parse_color(value: Any) -> Optional[Rgb]:
    """"#RRGGBB", "RRGGBB" or [r, g, b] like tryParseColor in DsRunnerSystems.cpp."""
    if isinstance(value, str):
        text = value[1:] if value.startswith("#") else value
        if len(text) != 6:
            return None
        try:
            return int(text[0:2], 16), int(text[2:4], 16), int(text[4:6], 16)
        except ValueError:
            return None
    if isinstance(value, list) and len(value) == 3:
        if all(isinstance(part, int) and not isinstance(part, bool) and 0 <= part <= 255 for part in value):
            return value[0], value[1], value[2]
    return None


def hero_from_tile(tile: Rgb) -> Rgb:
    """The top screen gradient is a bit darker than the tile (heroFromTile in DsRunnerSystems.cpp)."""
    return tile[0] * 13 // 16, tile[1] * 13 // 16, tile[2] * 13 // 16


def parse_systems_json(data: Any) -> tuple[list[CustomConsole], list[str]]:
    """Reads the parsed content of systems.json like DsRunnerSystems::LoadSystemsJson.

    Returns the consoles and warnings (German, for the log). Invalid entries are skipped, a file
    without a "systems" object gives no consoles at all (the launcher keeps its defaults then)."""
    warnings: list[str] = []
    systems = data.get("systems") if isinstance(data, dict) else None
    if not isinstance(systems, dict):
        return [], ["systems.json: \"systems\" fehlt oder ist kein Objekt, die Datei wird ignoriert."]
    items = list(systems.items())
    if len(items) > MAX_CUSTOM_CONSOLES:
        warnings.append(f"systems.json: mehr als {MAX_CUSTOM_CONSOLES} Konsolen, der Rest wird ignoriert.")
        items = items[:MAX_CUSTOM_CONSOLES]

    result: list[CustomConsole] = []
    for folder_name, entry in items:
        if not folder_name or len(folder_name.encode("utf-8")) >= _FOLDER_NAME_BUFFER or not isinstance(entry, dict):
            warnings.append(f"systems.json: Eintrag \"{folder_name}\" wird ignoriert.")
            continue
        builtin = find_builtin_by_folder_name(folder_name)

        def text(key: str, default: str) -> str:
            value = entry.get(key)
            return value if isinstance(value, str) else default

        label = text("label", builtin.label if builtin else folder_name)
        full_name = text("name", builtin.full_name if builtin else folder_name)
        tile = builtin.tile if builtin else UNKNOWN_TILE
        hero = builtin.hero if builtin else UNKNOWN_HERO
        if entry.get("color") is not None:
            color = _parse_color(entry["color"])
            if color:
                tile, hero = color, hero_from_tile(color)
            else:
                warnings.append(f"systems.json: ungültige Farbe bei \"{folder_name}\".")
        if entry.get("heroColor") is not None:
            color = _parse_color(entry["heroColor"])
            if color:
                hero = color
            else:
                warnings.append(f"systems.json: ungültige heroColor bei \"{folder_name}\".")

        extensions: list[str] = []
        raw_extensions = entry.get("extensions")
        if isinstance(raw_extensions, list):
            for value in raw_extensions:
                if not isinstance(value, str):
                    continue
                extension = value[1:] if value.startswith(".") else value
                if not extension or len(extension.encode("utf-8")) >= _EXTENSION_BUFFER:
                    warnings.append(f"systems.json: Endung \"{value}\" wird ignoriert.")
                    continue
                if len(extensions) >= MAX_EXTENSIONS:
                    warnings.append(f"systems.json: mehr als {MAX_EXTENSIONS} Endungen bei \"{folder_name}\".")
                    break
                extensions.append(extension.lower())
        elif builtin:
            # an override of a built-in console keeps its extensions
            extensions = [ext for system in SYSTEMS if system.console_folder.lower() == folder_name.lower()
                          for ext in system.extensions][:MAX_EXTENSIONS]

        libretro = entry.get("libretro")
        if isinstance(libretro, str):
            libretro_names: tuple[str, ...] = (libretro,) if libretro else ()
        elif isinstance(libretro, list):
            libretro_names = tuple(name for name in libretro if isinstance(name, str) and name)
        else:
            libretro_names = ()

        emulator = entry.get("emulator")
        console = Console(folder_name, _truncate_utf8(label, _LABEL_BUFFER),
                          _truncate_utf8(full_name, _FULL_NAME_BUFFER), tile, hero)
        result.append(CustomConsole(console, tuple(extensions), libretro_names,
                                    emulator if isinstance(emulator, str) and emulator else None))
    return result, warnings


def _system_for_custom(custom: CustomConsole, extension: str) -> Optional[System]:
    """The system of a custom console's extension, or None when the built-in mapping stays."""
    if custom.libretro_names:
        # a known libretro system keeps its key (the imaging code treats some systems specially)
        for system in SYSTEMS + EXTRA_SYSTEMS:
            if system.libretro_names[0].lower() == custom.libretro_names[0].lower():
                return system
        return System(f"custom:{custom.console.folder_name.lower()}", custom.libretro_names,
                      custom.console.folder_name, custom.extensions)
    if extension in _SYSTEM_BY_EXTENSION:
        return None
    if extension in _EXTRA_BY_EXTENSION:
        return _EXTRA_BY_EXTENSION[extension]
    # unknown system: the files are listed (generated artworks), but there is nothing to download
    return System(f"custom:{custom.console.folder_name.lower()}", (), custom.console.folder_name,
                  custom.extensions)


def set_custom_systems(customs: Iterable[CustomConsole]) -> None:
    """Makes the consoles of systems.json known (replaces the previous ones, an empty list resets)."""
    global _custom_consoles, _custom_system_by_extension
    _custom_consoles = tuple(customs)
    mapping: dict[str, System] = {}
    for custom in _custom_consoles:
        for extension in custom.extensions:
            if extension in mapping:
                continue  # the first console wins, like FindCustomByExtension
            system = _system_for_custom(custom, extension)
            if system:
                mapping[extension] = system
    _custom_system_by_extension = mapping


def custom_consoles() -> tuple[CustomConsole, ...]:
    return _custom_consoles


def system_for_file(file_name: str) -> Optional[System]:
    """The system of a ROM file by its extension (systems.json first), or None for files that are no known ROM."""
    dot = file_name.rfind(".")
    if dot <= 0:
        return None
    extension = file_name[dot + 1:].lower()
    return _custom_system_by_extension.get(extension) or _SYSTEM_BY_EXTENSION.get(extension)


def system_by_key(key: str) -> Optional[System]:
    return _SYSTEM_BY_KEY.get(key)


def hero_color_for(folder_names: list[str], system: Optional[System] = None) -> Rgb:
    """Hero color for an entry: the innermost known console folder wins, then the ROM system."""
    for name in reversed(folder_names):
        console = find_by_folder_name(name)
        if console:
            return console.hero
    if system:
        console = find_by_folder_name(system.console_folder)
        if console:
            return console.hero
    return UNKNOWN_HERO


def tile_color_for(folder_names: list[str], system: Optional[System] = None) -> Rgb:
    for name in reversed(folder_names):
        console = find_by_folder_name(name)
        if console:
            return console.tile
    if system:
        console = find_by_folder_name(system.console_folder)
        if console:
            return console.tile
    return UNKNOWN_TILE
