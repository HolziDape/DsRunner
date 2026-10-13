"""Artwork sources.

A source turns an identified game into candidate image URLs for an art type and downloads them.
New sources (e.g. SteamGridDB or ScreenScraper, which need an account / API key) subclass
ArtSource, implement `candidates` (and `load` if the download needs more than a GET), and are added
to `build_sources`.
"""
from __future__ import annotations

import posixpath
import urllib.parse
from dataclasses import dataclass
from typing import Optional

from PIL import Image

from .catalog import THUMBNAIL_FOLDERS, Catalog, repo_name
from .identify import GameInfo
from .imaging import open_image
from .names import thumbnail_file_name
from .net import HttpClient

ART_TYPES = ("title", "snap", "boxart")
ART_TYPE_LABELS = {"title": "Titelbild", "snap": "Screenshot", "boxart": "Cover (Boxart)"}
# fallbacks after the preferred type: a snap is a random game scene (a level map, a menu ...), the
# box is the better replacement for a missing title screen (e.g. "Up" has no title in libretro)
FALLBACK_ORDER = ("title", "boxart", "snap")
# tile images: the box first, it is close to square and made to be recognized at a glance
TILE_ORDER = ("boxart", "title", "snap")
# covers for the cover flow: the real box, a screen when there is none
COVER_ORDER = ("boxart", "title", "snap")
# small icons: the box has the largest shapes (logo, main character) to cut out
ICON_ORDER = ("boxart", "title", "snap")


def type_order(preferred: str) -> list[str]:
    """Preferred type first, the others as fallback (title, then boxart, snap last)."""
    return [preferred] + [art_type for art_type in FALLBACK_ORDER if art_type != preferred]


@dataclass
class Candidate:
    url: str
    art_type: str
    source: str
    # what is shown in the log, e.g. "libretro Titelbild: Mario Kart DS (Europe)"
    description: str


class ArtSource:
    id = "base"
    label = "Basis"
    art_types: tuple[str, ...] = ()

    def __init__(self, client: HttpClient):
        self.client = client

    def candidates(self, game: GameInfo, art_type: str) -> list[Candidate]:
        raise NotImplementedError

    def load(self, candidate: Candidate) -> Optional[Image.Image]:
        """Downloads a candidate. None when it does not exist. May raise net.NetworkError."""
        data = self.client.fetch(candidate.url)
        return open_image(data) if data else None


class LibretroThumbnails(ArtSource):
    """https://github.com/libretro-thumbnails: boxarts, title screens and screenshots by No-Intro name."""
    id = "libretro"
    label = "libretro-thumbnails"
    art_types = ("title", "snap", "boxart")
    BASE_URL = "https://raw.githubusercontent.com/libretro-thumbnails/{repo}/master/{folder}/{name}.png"

    def __init__(self, client: HttpClient, catalog: Catalog):
        super().__init__(client)
        self.catalog = catalog

    def url(self, libretro_system: str, art_type: str, name: str) -> str:
        return self.BASE_URL.format(repo=repo_name(libretro_system), folder=THUMBNAIL_FOLDERS[art_type],
                                    name=urllib.parse.quote(thumbnail_file_name(name), safe=""))

    def candidates(self, game: GameInfo, art_type: str) -> list[Candidate]:
        result = []
        for libretro_system, name in game.names:
            file_name = thumbnail_file_name(name)
            index = self.catalog.thumbnails(libretro_system)
            if index is not None and not index.has(art_type, file_name):
                continue
            result.append(Candidate(self.url(libretro_system, art_type, name), art_type, self.id,
                                    f"libretro, {name}"))
        return result

    def load(self, candidate: Candidate) -> Optional[Image.Image]:
        url = candidate.url
        # the repositories use symlinks for alternative names; raw.githubusercontent.com returns
        # the link target ("Other Name (USA).png") as text instead of the image
        for _ in range(3):
            data = self.client.fetch(url)
            if not data:
                return None
            image = open_image(data)
            if image is not None:
                return image
            target = _symlink_target(data)
            if not target:
                return None
            folder = posixpath.dirname(urllib.parse.urlsplit(url).path)
            path = posixpath.normpath(posixpath.join(urllib.parse.unquote(folder), target))
            url = urllib.parse.urlunsplit(("https", urllib.parse.urlsplit(url).netloc,
                                           urllib.parse.quote(path), "", ""))
        return None


def _symlink_target(data: bytes) -> Optional[str]:
    if len(data) > 1024:
        return None
    try:
        text = data.decode("utf-8").strip()
    except UnicodeDecodeError:
        return None
    if text.lower().endswith(".png") and "\n" not in text and "<" not in text:
        return text
    return None


# GameTDB regions per game code region letter, in the order they are tried
_GAMETDB_REGIONS = {
    "E": ["US"], "O": ["US"], "P": ["EN"], "D": ["DE", "EN"], "F": ["FR", "EN"], "S": ["ES", "EN"],
    "I": ["IT", "EN"], "H": ["NL", "EN"], "J": ["JA"], "K": ["KO"], "U": ["AU", "EN"],
}


class GameTdb(ArtSource):
    """GameTDB (https://www.gametdb.com) covers of NDS games by game code."""
    id = "gametdb"
    label = "GameTDB"
    art_types = ("boxart",)
    # largest first; the jpg/png variants differ between the cover sizes
    PATTERNS = (
        "https://art.gametdb.com/ds/coverHQ/{region}/{code}.jpg",
        "https://art.gametdb.com/ds/coverM/{region}/{code}.jpg",
        "https://art.gametdb.com/ds/cover/{region}/{code}.jpg",
        "https://art.gametdb.com/ds/coverS/{region}/{code}.png",
    )

    def __init__(self, client: HttpClient, german: bool = True):
        super().__init__(client)
        self.german = german

    def regions(self, game_code: str) -> list[str]:
        regions = list(_GAMETDB_REGIONS.get(game_code[3], ["EN", "US"]))
        if game_code[3] == "P" and self.german:
            regions.insert(0, "DE")
        return regions

    def candidates(self, game: GameInfo, art_type: str) -> list[Candidate]:
        code = game.game_code
        if art_type != "boxart" or game.system.key != "nds" or not code:
            return []
        return [Candidate(pattern.format(region=region, code=code), art_type, self.id, f"GameTDB, {code} ({region})")
                for region in self.regions(code) for pattern in self.PATTERNS]


SOURCE_IDS = ("libretro", "gametdb")


def build_sources(client: HttpClient, catalog: Catalog, enabled: tuple[str, ...] = SOURCE_IDS,
                  german: bool = True) -> list[ArtSource]:
    """The sources in the order they are asked. Add new sources here."""
    sources: list[ArtSource] = []
    for source_id in enabled:
        if source_id == "libretro":
            sources.append(LibretroThumbnails(client, catalog))
        elif source_id == "gametdb":
            sources.append(GameTdb(client, german))
        # elif source_id == "steamgriddb": sources.append(SteamGridDb(client, api_key)) ...
    return sources
