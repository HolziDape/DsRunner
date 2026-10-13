"""Identifies ROMs: game code from the header, checksum or file name -> No-Intro names."""
from __future__ import annotations

import json
import os
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from .catalog import Catalog
from .consoles import System
from .names import GAME_CODE_REGIONS, NameIndex, base_title, regions_of, sort_names
from .romheader import RomHeader, copier_header_size, crc32_of_file, read_header
from .sdcard import Entry

# files up to this size get a CRC32 to find them in the dat (NDS ROMs are found by game code)
CRC_MAX_SIZE = 64 * 1024 * 1024
MAX_NAMES = 3


@dataclass
class GameInfo:
    entry: Entry
    system: System
    header: Optional[RomHeader] = None
    # candidate No-Intro names as (libretro system, name), best first
    names: list[tuple[str, str]] = field(default_factory=list)
    # how the names were found (for the log), empty when the game is unknown
    match: str = ""

    @property
    def display_name(self) -> str:
        return self.names[0][1] if self.names else self.entry.name

    @property
    def game_code(self) -> Optional[str]:
        return self.header.game_code if self.header else None


class CrcCache:
    """CRC32 per file (keyed by path, size and modification time), stored on the PC."""

    def __init__(self, path: Optional[Path]):
        self.path = path
        self._lock = threading.Lock()
        self._values: dict[str, int] = {}
        self._dirty = False
        if path:
            try:
                with open(path, encoding="utf-8") as file:
                    self._values = {key: int(value) for key, value in json.load(file).items()}
            except (OSError, ValueError, AttributeError):
                self._values = {}

    @staticmethod
    def _key(path: Path, skip: int) -> Optional[str]:
        try:
            info = os.stat(path)
        except OSError:
            return None
        return f"{path}|{info.st_size}|{info.st_mtime_ns}|{skip}"

    def crc(self, path: Path, skip: int = 0) -> Optional[int]:
        key = self._key(path, skip)
        if key is None:
            return None
        with self._lock:
            if key in self._values:
                return self._values[key]
        try:
            value = crc32_of_file(path, skip)
        except OSError:
            return None
        with self._lock:
            self._values[key] = value
            self._dirty = True
        return value

    def save(self) -> None:
        if not self.path or not self._dirty:
            return
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            temp = self.path.with_suffix(".tmp")
            with self._lock:
                temp.write_text(json.dumps(self._values), encoding="utf-8")
                self._dirty = False
            os.replace(temp, self.path)
        except OSError:
            pass


def _queries(entry: Entry, header: Optional[RomHeader]) -> list[str]:
    """Names to search for: file name, NDS banner title, header title."""
    stem = entry.name.rsplit(".", 1)[0] if "." in entry.name[1:] else entry.name
    queries = [stem]
    if header and header.banner_title:
        lines = [line.strip() for line in header.banner_title.splitlines() if line.strip()]
        if lines:
            queries.append(lines[0])
            if len(lines) >= 3:
                # three line banners: "Title", "Subtitle", "Publisher"
                queries.append(f"{lines[0]} - {lines[1]}")
    if header and header.title and len(header.title) >= 4:
        queries.append(header.title.replace("_", " "))
    return list(dict.fromkeys(query for query in queries if base_title(query)))


def identify(entry: Entry, catalog: Catalog, preferred_regions: list[str], crc_cache: CrcCache,
             use_crc: bool = True) -> GameInfo:
    system = entry.system
    assert system is not None
    header = read_header(entry.fs_path, system.key) if system.key in ("nds", "gba") else None
    info = GameInfo(entry, system, header)

    # the file's own region tags ("(Germany)", "GER") first, then the game code's, then the user's
    preferred = list(regions_of(entry.name))
    if header and header.region_char in GAME_CODE_REGIONS:
        preferred.append(GAME_CODE_REGIONS[header.region_char])
    preferred += preferred_regions
    preferred = list(dict.fromkeys(preferred))

    stem = entry.name.rsplit(".", 1)[0] if "." in entry.name[1:] else entry.name

    def found(system_name: str, names: list[str], how: str) -> GameInfo:
        ordered = sort_names(names, preferred)
        # a file that carries the exact No-Intro name gets exactly that one
        exact = [name for name in ordered if name.lower() == stem.lower()]
        ordered = exact + [name for name in ordered if name not in exact]
        info.names = [(system_name, name) for name in ordered[:MAX_NAMES]]
        info.match = how
        return info

    dats = [(name, catalog.dat(name)) for name in system.libretro_names]
    dats = [(name, dat) for name, dat in dats if dat]

    # 1. game code (NDS / GBA)
    if header and header.game_code:
        for system_name, dat in dats:
            names = dat.names_for_serial(header.game_code)
            if names:
                return found(system_name, names, f"Game-Code {header.game_code}")

    # 2. checksum (small ROMs)
    if use_crc and system.key != "nds" and dats:
        try:
            size = entry.fs_path.stat().st_size
        except OSError:
            size = CRC_MAX_SIZE + 1
        if size <= CRC_MAX_SIZE:
            skips = [0]
            skip = copier_header_size(entry.fs_path, system.key, size)
            if skip:
                skips.append(skip)
            for skip in skips:
                crc = crc_cache.crc(entry.fs_path, skip)
                if crc is None:
                    continue
                for system_name, dat in dats:
                    names = dat.by_crc.get(crc)
                    if names:
                        return found(system_name, names, f"Prüfsumme {crc:08X}")

    # 3. name: exact, then similar. Without a dat the thumbnail file list is searched instead.
    indexes: list[tuple[str, NameIndex]] = [(name, dat.index) for name, dat in dats]
    if not indexes:
        for system_name in system.libretro_names:
            thumbnails = catalog.thumbnails(system_name)
            if thumbnails:
                indexes.append((system_name, thumbnails.name_index))
    queries = _queries(entry, header)
    for query in queries:
        for system_name, index in indexes:
            names = index.exact(query)
            if names:
                return found(system_name, names, "Name")
    best: tuple[float, str, list[str]] = (0.0, "", [])
    for query in queries:
        for system_name, index in indexes:
            names, ratio = index.fuzzy(query)
            if names and ratio > best[0]:
                best = (ratio, system_name, names)
    if best[2]:
        return found(best[1], best[2], f"ähnlicher Name ({best[0]:.0%})")

    # 4. the file may carry a No-Intro name the lists don't know (yet)
    if regions_of(stem):
        info.names = [(system.libretro_names[0], stem)]
        info.match = "Dateiname"
    return info
