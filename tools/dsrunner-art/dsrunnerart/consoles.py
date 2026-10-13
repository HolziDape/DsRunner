"""Known consoles: folder names and colors (mirror of arm9/source/themes/dsrunner/DsRunnerConsoles.h)
plus the mapping from file extensions to the libretro / No-Intro system names."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional

Rgb = tuple[int, int, int]


@dataclass(frozen=True)
class Console:
    folder_name: str
    label: str
    full_name: str
    tile: Rgb
    hero: Rgb


# Keep in sync with DsRunnerConsoles::kConsoles.
CONSOLES: tuple[Console, ...] = (
    Console("NDS", "NDS", "Nintendo DS", (58, 92, 190), (44, 74, 168)),
    Console("DS", "NDS", "Nintendo DS", (58, 92, 190), (44, 74, 168)),
    Console("GBA", "GBA", "Game Boy Advance", (112, 64, 184), (92, 52, 156)),
    Console("GBC", "GBC", "Game Boy Color", (198, 92, 72), (168, 70, 54)),
    Console("GB", "GB", "Game Boy", (96, 144, 64), (74, 116, 48)),
    Console("NES", "NES", "Nintendo Entertainment System", (190, 48, 48), (160, 36, 36)),
    Console("SNES", "SNES", "Super Nintendo", (112, 92, 172), (92, 74, 148)),
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


def find_by_folder_name(name: str) -> Optional[Console]:
    """Case insensitive lookup like DsRunnerConsoles::FindByFolderName."""
    lowered = name.lower()
    for console in CONSOLES:
        if console.folder_name.lower() == lowered:
            return console
    return None


@dataclass(frozen=True)
class System:
    """A game system as libretro names it (No-Intro dat name = thumbnail repository name)."""
    key: str
    # libretro system names, the first one is the main one; the others are tried as well
    libretro_names: tuple[str, ...]
    # folder name of the console in DsRunnerConsoles (for colors)
    console_folder: str
    extensions: tuple[str, ...] = field(default=())


SYSTEMS: tuple[System, ...] = (
    System("nds", ("Nintendo - Nintendo DS", "Nintendo - Nintendo DSi"), "NDS", ("nds", "dsi", "srl")),
    System("gba", ("Nintendo - Game Boy Advance",), "GBA", ("gba", "agb")),
    System("gbc", ("Nintendo - Game Boy Color", "Nintendo - Game Boy"), "GBC", ("gbc",)),
    System("gb", ("Nintendo - Game Boy", "Nintendo - Game Boy Color"), "GB", ("gb",)),
    System("nes", ("Nintendo - Nintendo Entertainment System",), "NES", ("nes",)),
    System("snes", ("Nintendo - Super Nintendo Entertainment System",), "SNES", ("sfc", "smc")),
    System("sms", ("Sega - Master System - Mark III",), "SMS", ("sms",)),
    System("gg", ("Sega - Game Gear",), "GG", ("gg",)),
    System("md", ("Sega - Mega Drive - Genesis",), "MD", ("md", "gen", "smd")),
    System("pce", ("NEC - PC Engine - TurboGrafx 16",), "PCE", ("pce",)),
)

_SYSTEM_BY_EXTENSION = {ext: system for system in SYSTEMS for ext in system.extensions}
_SYSTEM_BY_KEY = {system.key: system for system in SYSTEMS}

# Console folders whose files are identified by extension only (e.g. a "DS" folder holds nds files).
ROM_EXTENSIONS = frozenset(_SYSTEM_BY_EXTENSION)


def system_for_file(file_name: str) -> Optional[System]:
    """The system of a ROM file by its extension, or None for files that are no known ROM."""
    dot = file_name.rfind(".")
    if dot <= 0:
        return None
    return _SYSTEM_BY_EXTENSION.get(file_name[dot + 1:].lower())


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
