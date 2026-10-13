"""SD card layout: settings, library scanning and the artwork path contract of the launcher."""
from __future__ import annotations

import json
import os
import stat
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Optional

from .consoles import System, system_for_file

# see DsRunnerFileInfoCardView.cpp
ART_FOLDER = "/_dsrunner/art"
# size of the path buffer in DsRunnerFileInfoCardView::RequestArt (including the terminator)
ART_PATH_BUFFER = 256
SETTINGS_PATH = "/_pico/settings.json"
DEFAULT_LIBRARY_PATH = "/Games"
OVERRIDE_FOLDER = "/_dsrunner/art-override"
MANIFEST_NAME = ".dsrunner-art.json"

# folders in the SD root that are never consoles (only relevant without a library folder)
_ROOT_SYSTEM_FOLDERS = {"system", "system volume information", "$recycle.bin", "recycler", "found.000"}


def skip_drive(path: str) -> str:
    """Mirror of DsRunnerPaths::SkipDrive: drops a drive prefix like "fat:"."""
    colon = path.find(":")
    return path[colon + 1:] if colon >= 0 else path


def art_path(folder_path: str, file_name: str, is_folder: bool) -> Optional[str]:
    """Mirror of DsRunnerFileInfoCardView::RequestArt.

    folder_path is the folder that contains the entry (as the launcher has it, e.g. "fat:/Games/NDS/"),
    file_name the name of the entry. Returns the SD path of the artwork, or None when the launcher
    can't build it (path too long for its buffer).
    """
    folder = skip_drive(folder_path).rstrip("/")
    name = file_name
    if not is_folder:
        # files lose their extension, folder names are kept as they are
        dot = name.rfind(".")
        if dot > 0:
            name = name[:dot]
    # lengths in bytes: the launcher works on UTF-8 strings (FF_LFN_UNICODE 2)
    length = len(ART_FOLDER.encode()) + len(folder.encode()) + 1 + len(name.encode()) + len(".bmp")
    if length >= ART_PATH_BUFFER:
        return None
    return f"{ART_FOLDER}{folder}/{name}.bmp"


def art_path_for_entry(entry_path: str, is_folder: bool) -> Optional[str]:
    """Artwork SD path for the entry at entry_path (a path from the SD root, e.g. "/Games/NDS")."""
    path = skip_drive(entry_path)
    if is_folder:
        path = path.rstrip("/")
    slash = path.rfind("/")
    return art_path(path[:slash + 1] if slash >= 0 else "/", path[slash + 1:], is_folder)


def to_fs_path(sd_root: Path, sd_path: str) -> Path:
    """Local path of an SD path ("/Games/NDS" -> E:\\Games\\NDS)."""
    parts = [part for part in skip_drive(sd_path).split("/") if part]
    return Path(sd_root, *parts)


def normalize_sd_root(text: str) -> Path:
    """Accepts "E:", "E:\\", "/media/sd" ... and returns a usable root path."""
    text = text.strip().strip('"')
    if len(text) == 2 and text[1] == ":":
        text += "\\"
    return Path(text)


def read_library_path(sd_root: Path) -> str:
    """The libraryPath setting of the launcher ("/Games" when missing, "" when switched off)."""
    settings = to_fs_path(sd_root, SETTINGS_PATH)
    try:
        with open(settings, encoding="utf-8-sig") as file:
            data = json.load(file)
    except (OSError, ValueError):
        return DEFAULT_LIBRARY_PATH
    value = data.get("libraryPath", DEFAULT_LIBRARY_PATH) if isinstance(data, dict) else None
    if not isinstance(value, str):
        return DEFAULT_LIBRARY_PATH
    return skip_drive(value)


@dataclass
class Entry:
    """A folder or ROM the launcher shows and that gets an artwork."""
    sd_path: str
    fs_path: Path
    is_folder: bool
    # 0 = directly in the library folder (console folders)
    depth: int
    # names of the folders between the library folder and the entry (for colors)
    parent_folders: list[str] = field(default_factory=list)
    system: Optional[System] = None

    @property
    def name(self) -> str:
        return self.sd_path.rstrip("/").rsplit("/", 1)[-1]

    @property
    def art_sd_path(self) -> Optional[str]:
        return art_path_for_entry(self.sd_path, self.is_folder)


@dataclass
class ScanResult:
    sd_root: Path
    library_path: str
    # True when there is no library folder and the launcher shows the SD root
    root_mode: bool
    entries: list[Entry]
    warnings: list[str]

    @property
    def folders(self) -> list[Entry]:
        return [entry for entry in self.entries if entry.is_folder]

    @property
    def games(self) -> list[Entry]:
        return [entry for entry in self.entries if not entry.is_folder]


def _is_hidden(path: Path, name: str) -> bool:
    if name.startswith("."):
        return True
    try:
        attributes = getattr(os.stat(path), "st_file_attributes", 0)
    except OSError:
        return True
    return bool(attributes & getattr(stat, "FILE_ATTRIBUTE_HIDDEN", 0))


def scan(sd_root: Path, library_path: Optional[str] = None, max_depth: int = 4,
         cancelled: Callable[[], bool] = lambda: False) -> ScanResult:
    """Collects the console folders, subfolders and ROMs of the library."""
    warnings: list[str] = []
    if library_path is None:
        library_path = read_library_path(sd_root)
    library_path = "/" + library_path.strip("/") if library_path.strip("/") else ""
    root_mode = not library_path or not to_fs_path(sd_root, library_path).is_dir()
    if root_mode:
        if library_path:
            warnings.append(f"Bibliotheksordner {library_path} fehlt, es wird das Hauptverzeichnis verwendet.")
        library_path = ""

    entries: list[Entry] = []

    def walk(sd_folder: str, depth: int, parents: list[str]) -> None:
        fs_folder = to_fs_path(sd_root, sd_folder)
        try:
            children = sorted(os.listdir(fs_folder), key=str.lower)
        except OSError as error:
            warnings.append(f"Ordner {sd_folder or '/'} nicht lesbar: {error}")
            return
        for name in children:
            if cancelled():
                return
            fs_path = fs_folder / name
            if _is_hidden(fs_path, name):
                continue
            sd_path = f"{sd_folder}/{name}"
            if fs_path.is_dir():
                if root_mode and depth == 0 and (name.startswith("_") or name.lower() in _ROOT_SYSTEM_FOLDERS):
                    continue
                entries.append(Entry(sd_path, fs_path, True, depth, list(parents)))
                if depth + 1 < max_depth:
                    walk(sd_path, depth + 1, parents + [name])
            else:
                system = system_for_file(name)
                if system:
                    entries.append(Entry(sd_path, fs_path, False, depth, list(parents), system))

    walk(library_path, 0, [])
    return ScanResult(sd_root, library_path or "/", root_mode, entries, warnings)
