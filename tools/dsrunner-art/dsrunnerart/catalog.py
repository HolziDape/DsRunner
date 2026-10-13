"""Game name catalogs: No-Intro dats from libretro-database and the file lists of libretro-thumbnails.

Both are downloaded once and cached on the PC.
"""
from __future__ import annotations

import json
import threading
import urllib.parse
from typing import Callable, Optional

from .datfile import Dat
from .names import NameIndex
from .net import HttpClient, NetworkError

DAT_URL = "https://raw.githubusercontent.com/libretro/libretro-database/master/metadat/no-intro/{system}.dat"
TREE_URL = "https://api.github.com/repos/libretro-thumbnails/{repo}/git/trees/master?recursive=1"
DAT_MAX_AGE = 30 * 24 * 3600
TREE_MAX_AGE = 7 * 24 * 3600

THUMBNAIL_FOLDERS = {"boxart": "Named_Boxarts", "snap": "Named_Snaps", "title": "Named_Titles"}

Log = Callable[[str], None]


def repo_name(libretro_system: str) -> str:
    """libretro-thumbnails repository of a system ("Nintendo - Nintendo DS" -> "Nintendo_-_Nintendo_DS")."""
    return libretro_system.replace(" ", "_")


class ThumbnailIndex:
    """Which thumbnails exist for one system (file names without .png per art type)."""

    def __init__(self, files: dict[str, set[str]]):
        self.files = files
        self._name_index: Optional[NameIndex] = None

    @classmethod
    def from_tree(cls, tree: dict) -> Optional["ThumbnailIndex"]:
        if tree.get("truncated"):
            return None
        folders = {folder: art_type for art_type, folder in THUMBNAIL_FOLDERS.items()}
        files: dict[str, set[str]] = {art_type: set() for art_type in THUMBNAIL_FOLDERS}
        for item in tree.get("tree", []):
            path = item.get("path", "")
            folder, _, file_name = path.partition("/")
            if folder in folders and file_name.lower().endswith(".png") and "/" not in file_name:
                files[folders[folder]].add(file_name[:-4])
        return cls(files)

    def has(self, art_type: str, thumbnail_name: str) -> bool:
        return thumbnail_name in self.files.get(art_type, ())

    @property
    def name_index(self) -> NameIndex:
        if self._name_index is None:
            names = set().union(*self.files.values()) if self.files else set()
            self._name_index = NameIndex.build(names)
        return self._name_index


class Catalog:
    """Lazily loads and caches the dat and the thumbnail index per libretro system."""

    def __init__(self, client: HttpClient, log: Log = lambda message: None, use_tree_index: bool = True):
        self.client = client
        self.log = log
        self.use_tree_index = use_tree_index
        self._lock = threading.Lock()
        self._dats: dict[str, Optional[Dat]] = {}
        self._trees: dict[str, Optional[ThumbnailIndex]] = {}
        self._tree_warned = False

    def dat(self, libretro_system: str) -> Optional[Dat]:
        with self._lock:
            if libretro_system in self._dats:
                return self._dats[libretro_system]
            dat = None
            url = DAT_URL.format(system=urllib.parse.quote(libretro_system))
            try:
                data = self.client.fetch(url, folder="dat", max_age=DAT_MAX_AGE)
                if data:
                    dat = Dat.parse(data.decode("utf-8", "replace"))
                else:
                    self.log(f"Keine Namensliste für {libretro_system} gefunden.")
            except NetworkError as error:
                self.log(f"Namensliste für {libretro_system} nicht geladen ({error}).")
            self._dats[libretro_system] = dat
            return dat

    def thumbnails(self, libretro_system: str) -> Optional[ThumbnailIndex]:
        """File list of the thumbnail repository (GitHub API). None when not available: then
        thumbnails are simply tried by name."""
        if not self.use_tree_index:
            return None
        with self._lock:
            if libretro_system in self._trees:
                return self._trees[libretro_system]
            index = None
            url = TREE_URL.format(repo=repo_name(libretro_system))
            try:
                data = self.client.fetch(url, folder="index", max_age=TREE_MAX_AGE,
                                         headers={"Accept": "application/vnd.github+json"})
                if data:
                    index = ThumbnailIndex.from_tree(json.loads(data.decode("utf-8")))
            except (NetworkError, ValueError) as error:
                if not self._tree_warned:
                    self.log(f"Dateiliste von libretro-thumbnails nicht verfügbar ({error}), Bilder werden direkt gesucht.")
                    self._tree_warned = True
            self._trees[libretro_system] = index
            return index
