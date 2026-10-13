"""Runs an update: scan the card, pick a source per entry, convert and write the BMPs.

Two kinds of files are written per game: the artwork for the top screen (`/_dsrunner/art`, 256x192)
and the tile image for the launcher grid (`/_dsrunner/tiles`, 64x64). Console folders only get an
artwork.

Bookkeeping: `.dsrunner-art.json` in both folders remembers which files this tool wrote (with their
SHA-1), so files the user put there by hand are not overwritten (unless `replace_manual` is set) and
later runs only do what is missing. The written BMPs also carry a signature in their header, so they
are recognized as the tool's own even when the manifest got lost.
"""
from __future__ import annotations

import concurrent.futures
import hashlib
import json
import os
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Optional

from PIL import Image

from . import __version__
from .catalog import Catalog
from .consoles import find_by_folder_name, hero_color_for, tile_color_for
from .identify import CrcCache, GameInfo, identify
from .imaging import (TILE_SIZE, console_art, icon_art, icon_tile, is_tool_bmp, open_image, to_ds_bmp, to_hero,
                      to_tile, validate_bmp)
from .names import REGION_PREFERENCES
from .romheader import read_header
from .net import HttpClient, NetworkError, default_cache_dir
from .sdcard import (ART_FOLDER, MANIFEST_NAME, OVERRIDE_FOLDER, TILE_FOLDER, TILE_OVERRIDE_FOLDER, Entry,
                     ScanResult, scan, to_fs_path)
from .sources import ART_TYPE_LABELS, ART_TYPES, SOURCE_IDS, TILE_ORDER, ArtSource, build_sources, type_order

IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp")
# bump to regenerate the generated console artworks after a design change
GENERATED_VERSION = 1
# files with these sources are replaced as soon as a real image is found
_RETRY_SOURCES = ("icon:",)
# the manifests are saved after this many written files (and at the end), so an interrupted run
# does not leave files behind that look hand-placed
_SAVE_EVERY = 20

Log = Callable[[str, str], None]
Progress = Callable[[int, int, str], None]


@dataclass(frozen=True)
class Kind:
    """One kind of file the tool writes per entry."""
    key: str
    folder: str
    override_folder: str
    size: tuple[int, int]
    # for the log and the summary
    label: str
    plural: str
    # key of the record list in the manifest
    section: str


ART = Kind("art", ART_FOLDER, OVERRIDE_FOLDER, (256, 192), "Artwork", "Artworks", "artworks")
TILE = Kind("tile", TILE_FOLDER, TILE_OVERRIDE_FOLDER, (TILE_SIZE, TILE_SIZE), "Kachel-Bild", "Kachel-Bilder",
            "tiles")
KINDS = (ART, TILE)


@dataclass
class Options:
    art_type: str = "title"
    force: bool = False
    download: bool = True
    console_art: bool = True
    icon_fallback: bool = True
    region: str = "de"
    # folder with the user's own images (default: <SD>/_dsrunner/art-override)
    override_dir: Optional[Path] = None
    sources: tuple[str, ...] = SOURCE_IDS
    clean: bool = False
    dry_run: bool = False
    cache_dir: Optional[Path] = None
    workers: int = 4
    use_crc: bool = True
    max_depth: int = 4
    library_path: Optional[str] = None
    refresh_misses: bool = False
    # 64x64 tile images for the launcher grid
    tiles: bool = True
    # folder with the user's own tile images (default: <SD>/_dsrunner/tile-override)
    tile_override_dir: Optional[Path] = None
    # also replace BMPs this tool did not write (hand-placed images)
    replace_manual: bool = False

    def __post_init__(self):
        if self.art_type not in ART_TYPES:
            raise ValueError(f"unknown art type {self.art_type}")
        if self.region not in REGION_PREFERENCES:
            raise ValueError(f"unknown region {self.region}")


@dataclass
class Stats:
    downloaded: int = 0
    generated: int = 0
    icons: int = 0
    overrides: int = 0
    unchanged: int = 0
    foreign: int = 0
    # hand-placed files the launcher can't show
    foreign_bad: int = 0
    missing: int = 0
    errors: int = 0
    removed: int = 0
    replaced_manual: int = 0
    missing_names: list[str] = field(default_factory=list)
    foreign_names: list[str] = field(default_factory=list)
    # the same counters for the tile images (None when they are switched off)
    tiles: Optional["Stats"] = None

    def _parts(self, tiles: bool) -> list[str]:
        parts = [f"{self.downloaded} geladen"]
        if self.overrides or not tiles:
            parts.append(f"{self.overrides} eigene Bilder")
        if not tiles:
            parts.append(f"{self.generated} Konsolen/Ordner erzeugt")
        parts += [f"{self.icons} aus dem Spiel-Icon", f"{self.unchanged} unverändert"]
        if self.replaced_manual:
            parts.append(f"{self.replaced_manual} von Hand abgelegte ersetzt")
        if self.foreign:
            parts.append(f"{self.foreign} von Hand abgelegt (behalten)")
        if self.removed:
            parts.append(f"{self.removed} verwaiste gelöscht")
        parts.append(f"{self.missing} ohne Bild")
        if self.errors:
            parts.append(f"{self.errors} Fehler")
        return parts

    def summary(self) -> str:
        text = ", ".join(self._parts(False))
        if self.tiles is not None:
            text = f"Artworks: {text}. Kachel-Bilder: {', '.join(self.tiles._parts(True))}"
        return text

    def short_summary(self) -> str:
        """One line for the status bar of the window (artworks and tile images together)."""
        items = [self] + ([self.tiles] if self.tiles else [])
        new = sum(item.downloaded + item.generated + item.icons + item.overrides for item in items)
        text = f"{new} Bilder neu, {sum(item.missing for item in items)} ohne Bild"
        if self.total_errors:
            text += f", {self.total_errors} Fehler"
        return text + " (Details im Protokoll)"

    @property
    def total_errors(self) -> int:
        return self.errors + (self.tiles.errors if self.tiles else 0)


@dataclass
class Result:
    entry: Entry
    status: str  # downloaded, generated, icon, override, unchanged, foreign, missing, error, skipped
    message: str = ""
    art_key: Optional[str] = None
    record: Optional[dict] = None
    kind: Kind = ART
    # foreign files: False when the launcher can't show the file
    usable: bool = True
    detail: str = ""
    # a hand-placed file was overwritten
    replaced_manual: bool = False


def _sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


class Manifest:
    """Which files this tool wrote: {lower case SD path: {path, source, sha1}}."""

    def __init__(self, path: Path, section: str = "artworks"):
        self.path = path
        self.section = section
        self.records: dict[str, dict] = {}
        try:
            with open(path, encoding="utf-8") as file:
                data = json.load(file)
            if isinstance(data, dict) and isinstance(data.get(section), dict):
                self.records = data[section]
        except (OSError, ValueError):
            self.records = {}

    def get(self, key: str) -> Optional[dict]:
        return self.records.get(key)

    def save(self) -> None:
        if not self.records and not self.path.exists():
            return
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temp = self.path.with_name(self.path.name + ".tmp")
        payload = {"tool": f"DsRunner Art {__version__}", self.section: dict(sorted(self.records.items()))}
        temp.write_text(json.dumps(payload, ensure_ascii=False, indent=1), encoding="utf-8")
        os.replace(temp, self.path)


class OverrideIndex:
    """User images: mirrored paths (<override>/Games/NDS/Mario Kart DS.png) or just the name
    (<override>/Mario Kart DS.png)."""

    def __init__(self, root: Optional[Path]):
        self.root = root
        self.by_path: dict[str, Path] = {}
        self.by_name: dict[str, Path] = {}
        if not root or not root.is_dir():
            return
        for folder, _, files in os.walk(root):
            for file_name in sorted(files):
                stem, extension = os.path.splitext(file_name)
                if extension.lower() not in IMAGE_EXTENSIONS or file_name.startswith("."):
                    continue
                path = Path(folder) / file_name
                relative = path.relative_to(root).with_suffix("").as_posix().lower()
                self.by_path.setdefault(relative, path)
                self.by_name.setdefault(stem.lower(), path)

    def __len__(self) -> int:
        return len(self.by_path)

    def find(self, entry: Entry) -> Optional[Path]:
        return self.by_path.get(entry.relative_key) or self.by_name.get(entry.stem.lower())


@dataclass
class Slot:
    """The target file of one entry and one kind, and what is known about the file there now."""
    kind: Kind
    sd_path: str
    target: Path
    key: str
    record: Optional[dict]
    existing: bool
    ours: bool
    # contents of a file that is not ours (to check whether the launcher can show it)
    foreign_data: Optional[bytes] = None
    # True when the file is recognized by its signature only (the manifest lost it): the record is
    # written back to the manifest
    adopted: bool = False

    def unchanged(self, source: str, force: bool) -> bool:
        return self.existing and self.ours and (self.record or {}).get("source") == source and not force


class Updater:
    def __init__(self, sd_root: Path, options: Options, log: Log = lambda message, level="info": None,
                 progress: Progress = lambda done, total, message: None,
                 cancel: Optional[threading.Event] = None,
                 client: Optional[HttpClient] = None, sources: Optional[list[ArtSource]] = None):
        self.sd_root = Path(sd_root)
        self.options = options
        self.log = log
        self.progress = progress
        self.cancel = cancel or threading.Event()
        cache_dir = Path(options.cache_dir) if options.cache_dir else default_cache_dir()
        self.client = client or HttpClient(cache_dir, offline=not options.download,
                                           refresh_misses=options.refresh_misses or options.force)
        self.catalog = Catalog(self.client, lambda message: self.log(message, "warn"))
        self.sources = sources if sources is not None else build_sources(
            self.client, self.catalog, options.sources, german=options.region == "de")
        self.crc_cache = CrcCache(cache_dir / "crc.json")
        self.kinds = (ART, TILE) if options.tiles else (ART,)
        self.manifests = {kind.key: Manifest(to_fs_path(self.sd_root, kind.folder) / MANIFEST_NAME, kind.section)
                          for kind in KINDS}
        override_dirs = {
            ART.key: options.override_dir or to_fs_path(self.sd_root, OVERRIDE_FOLDER),
            TILE.key: options.tile_override_dir or to_fs_path(self.sd_root, TILE_OVERRIDE_FOLDER),
        }
        self.overrides = {key: OverrideIndex(Path(folder)) for key, folder in override_dirs.items()}
        self.stats = Stats()
        if options.tiles:
            self.stats.tiles = Stats()
        self._warned_sources: set[str] = set()
        self._lock = threading.Lock()
        self._unsaved = 0
        self._save_warned = False

    @property
    def manifest(self) -> Manifest:
        """The artwork manifest (kept for callers of the first version)."""
        return self.manifests[ART.key]

    def _stats(self, kind: Kind) -> Stats:
        return self.stats.tiles if kind is TILE and self.stats.tiles is not None else self.stats

    # --- main loop ---------------------------------------------------------------------------

    def run(self) -> Stats:
        options = self.options
        if not self.sd_root.is_dir():
            raise FileNotFoundError(f"{self.sd_root} nicht gefunden")
        self.log(f"Karte: {self.sd_root}", "info")
        result = scan(self.sd_root, options.library_path, options.max_depth, self.cancel.is_set)
        for warning in result.warnings:
            self.log(warning, "warn")
        self.log(f"Bibliothek: {result.library_path} · {len(result.folders)} Ordner, {len(result.games)} Spiele", "info")
        for kind in self.kinds:
            overrides = self.overrides[kind.key]
            if len(overrides):
                label = "Eigene Bilder" if kind is ART else "Eigene Kachel-Bilder"
                self.log(f"{label}: {len(overrides)} in {overrides.root}", "info")
        if options.replace_manual:
            self.log("Von Hand abgelegte Bilder werden ersetzt, wenn ein Bild gefunden wird.", "info")
        if options.dry_run:
            self.log("Probelauf: es wird nichts auf die Karte geschrieben.", "info")

        entries = result.entries
        total = len(entries)
        done = 0
        self.progress(0, total, "")
        try:
            with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, options.workers)) as pool:
                futures = {pool.submit(self._process_safe, entry): entry for entry in entries}
                for future in concurrent.futures.as_completed(futures):
                    if self.cancel.is_set():
                        for pending in futures:
                            pending.cancel()
                    if future.cancelled():
                        continue
                    outcomes = future.result()
                    done += 1
                    for outcome in outcomes:
                        self._apply(outcome)
                    self.progress(done, total, futures[future].name)
            if options.clean and not self.cancel.is_set():
                for kind in self.kinds:
                    self._clean(result, kind)
        finally:
            self._forget_deleted()
            self._save_manifests(final=True)
            self.client.save()
            self.crc_cache.save()
        if self.cancel.is_set():
            self.log("Abgebrochen.", "warn")
        self.log(f"Fertig. {self.stats.summary()}.", "info")
        self._log_hints()
        return self.stats

    def _log_hints(self) -> None:
        stats = [self.stats] + ([self.stats.tiles] if self.stats.tiles else [])
        foreign = sum(item.foreign for item in stats)
        if foreign:
            names = [name for item in stats for name in item.foreign_names]
            examples = ", ".join(names[:3]) + (" …" if len(names) > 3 else "")
            self.log(f"Hinweis: {foreign} von Hand abgelegte Bilder wurden behalten ({examples}). Sollen gefundene "
                     "Bilder sie ersetzen, „Auch von Hand abgelegte Bilder ersetzen“ wählen (--replace-manual).", "info")
            bad = sum(item.foreign_bad for item in stats)
            if bad:
                self.log(f"{bad} davon kann DsRunner nicht anzeigen (falsches Format, siehe oben).", "warn")

    def _save_manifests(self, final: bool = False) -> None:
        if self.options.dry_run:
            return
        self._unsaved = 0
        for kind in KINDS:
            try:
                self.manifests[kind.key].save()
            except OSError as error:
                if final or not self._save_warned:
                    self._save_warned = True
                    self.log(f"Liste der {kind.plural} nicht gespeichert: {error}", "error")

    def _apply(self, outcome: Result) -> None:
        kind = outcome.kind
        stats = self._stats(kind)
        name = outcome.entry.sd_path
        status = outcome.status
        if outcome.record is not None and outcome.art_key and not self.options.dry_run:
            self.manifests[kind.key].records[outcome.art_key] = outcome.record
            self._unsaved += 1
            if self._unsaved >= _SAVE_EVERY:
                self._save_manifests()
        if outcome.replaced_manual:
            stats.replaced_manual += 1
        tile = kind is TILE
        # tile lines that report success only show with "Details anzeigen", problems always
        if status == "downloaded":
            stats.downloaded += 1
            if tile:
                self.log(f"✓ {name}: Kachel-Bild aus {outcome.message}", "detail")
            else:
                self.log(f"✓ {name}: {outcome.message}", "info")
        elif status == "override":
            stats.overrides += 1
            self.log(f"✓ {name}: eigenes {'Kachel-Bild' if tile else 'Bild'} {outcome.message}", "info")
        elif status == "generated":
            stats.generated += 1
            self.log(f"✓ {name}: erzeugt", "detail")
        elif status == "icon":
            stats.icons += 1
            if tile:
                self.log(f"~ {name}: Kachel-Bild aus dem Icon des Spiels{outcome.message}", "detail")
            else:
                self.log(f"~ {name}: kein Bild gefunden, Icon des Spiels verwendet{outcome.message}", "info")
        elif status == "unchanged":
            stats.unchanged += 1
            self.log(f"· {name}: {'Kachel-Bild ' if tile else ''}vorhanden", "detail")
        elif status == "foreign":
            stats.foreign += 1
            stats.foreign_names.append(outcome.message or name)
            if outcome.usable:
                self.log(f"· {name}: von Hand abgelegtes {kind.label} bleibt ({outcome.message})", "detail")
            else:
                stats.foreign_bad += 1
                self.log(f"✗ {name}: von Hand abgelegtes {kind.label} {outcome.message} kann DsRunner nicht "
                         f"anzeigen ({outcome.detail}). Löschen oder „Auch von Hand abgelegte Bilder ersetzen“ "
                         "wählen.", "warn")
        elif status == "missing":
            stats.missing += 1
            stats.missing_names.append(name)
            what = "kein Kachel-Bild" if tile else "kein Bild gefunden"
            self.log(f"✗ {name}: {what}{outcome.message}", "warn")
        elif status == "error":
            stats.errors += 1
            self.log(f"✗ {name}: {kind.label + ': ' if tile else ''}{outcome.message}", "error")

    # --- one entry ---------------------------------------------------------------------------

    def _process_safe(self, entry: Entry) -> list[Result]:
        if self.cancel.is_set():
            return []
        try:
            return self._process(entry)
        except Exception as error:  # noqa: BLE001 - one broken file must not stop the run
            return [Result(entry, "error", f"{type(error).__name__}: {error}")]

    def _process(self, entry: Entry) -> list[Result]:
        kinds = [kind for kind in self.kinds if kind is ART or not entry.is_folder]
        results: list[Result] = []
        pending: list[Slot] = []
        for kind in kinds:
            try:
                slot = self._slot(entry, kind)
                if isinstance(slot, Result):
                    results.append(slot)
                    continue
                result = self._settle(entry, slot)
            except Exception as error:  # noqa: BLE001 - the other kind may still work
                results.append(Result(entry, "error", f"{type(error).__name__}: {error}", kind=kind))
                continue
            if result is None:
                pending.append(slot)
            else:
                results.append(result)
        if not pending:
            return results

        # games that need a picture: identify once, then look for each kind
        note = ""
        try:
            game = self._identify(entry)
        except Exception as error:  # noqa: BLE001 - the icon fallback still works without it
            note = f"Erkennung fehlgeschlagen: {type(error).__name__}: {error}"
            game = self._header_only(entry)
        for slot in pending:
            try:
                results.append(self._fill(entry, slot, game, note))
            except Exception as error:  # noqa: BLE001
                results.append(Result(entry, "error", f"{type(error).__name__}: {error}", kind=slot.kind))
        return results

    def _slot(self, entry: Entry, kind: Kind):
        sd_path = entry.art_sd_path if kind is ART else entry.tile_sd_path
        if not sd_path:
            return Result(entry, "error", "Pfad zu lang für den Launcher (max. 255 Zeichen)", kind=kind)
        target = to_fs_path(self.sd_root, sd_path)
        key = sd_path.lower()
        record = self.manifests[kind.key].get(key)
        existing = target.is_file()
        slot = Slot(kind, sd_path, target, key, record, existing, False)
        if existing:
            try:
                data = target.read_bytes()
            except OSError:
                data = b""
            if record:
                slot.ours = _sha1(data) == record.get("sha1")
            elif is_tool_bmp(data):
                # written by this tool, but the manifest does not know it (e.g. an interrupted run)
                slot.ours = True
                slot.adopted = True
                slot.record = {"path": sd_path, "source": "recovered", "sha1": _sha1(data), "time": int(time.time())}
            if not slot.ours:
                slot.foreign_data = data
        return slot

    def _settle(self, entry: Entry, slot: Slot) -> Optional[Result]:
        """Handles everything that needs no download. None when the entry needs a downloaded image."""
        options = self.options
        kind = slot.kind

        def unchanged() -> Result:
            # an adopted file goes back into the manifest
            return Result(entry, "unchanged", art_key=slot.key if slot.adopted else None,
                          record=slot.record if slot.adopted else None, kind=kind)

        # 1. the user's own image always wins
        override = self.overrides[kind.key].find(entry)
        if override:
            try:
                stat = override.stat()
                source = f"override:{override.name}:{stat.st_size}:{stat.st_mtime_ns}"
            except OSError as error:
                return Result(entry, "error", f"{override}: {error}", kind=kind)
            if slot.unchanged(source, options.force):
                return unchanged()
            image = open_image(override.read_bytes())
            if image is None:
                return Result(entry, "error", f"{override} ist kein lesbares Bild", kind=kind)
            converted = to_tile(image, "custom") if kind is TILE else to_hero(image, "custom")
            return self._write(entry, slot, converted, source, "override", str(override.name))

        # 2. files someone put there by hand stay, unless the user wants them replaced
        if slot.existing and not slot.ours and not options.replace_manual:
            relative = slot.sd_path[len(kind.folder) + 1:]
            validation = validate_bmp(slot.foreign_data or b"", kind.size)
            if validation.ok:
                return Result(entry, "foreign", relative, kind=kind)
            return Result(entry, "foreign", relative, kind=kind, usable=False, detail="; ".join(validation.errors))

        if entry.is_folder:
            if not options.console_art:
                return Result(entry, "skipped", kind=kind)
            names = entry.parent_folders + [entry.name]
            console = find_by_folder_name(entry.name) if entry.depth == 0 else None
            label = console.label if console else entry.name
            hero = hero_color_for(names)
            source = f"generated:{GENERATED_VERSION}:{label}:{hero}"
            if slot.unchanged(source, options.force):
                return unchanged()
            image = console_art(label, hero, tile_color_for(names))
            return self._write(entry, slot, image, source, "generated")

        # 3. games: keep what is there unless forced (icon fallbacks are retried)
        if slot.existing and slot.ours and not options.force and \
                not str((slot.record or {}).get("source", "")).startswith(_RETRY_SOURCES):
            return unchanged()
        return None

    def _header_only(self, entry: Entry) -> GameInfo:
        system = entry.system
        return GameInfo(entry, system, read_header(entry.fs_path, system.key) if system.key == "nds" else None)

    def _identify(self, entry: Entry) -> GameInfo:
        if not self.options.download:
            # without downloads only the header matters (for the icon)
            return self._header_only(entry)
        return identify(entry, self.catalog, REGION_PREFERENCES[self.options.region], self.crc_cache,
                        self.options.use_crc)

    def _fill(self, entry: Entry, slot: Slot, game: GameInfo, note: str = "") -> Result:
        """Downloads a picture for a game, or builds one from its icon."""
        options = self.options
        kind = slot.kind
        system_key = entry.system.key if entry.system else None
        failures: list[str] = []
        if options.download:
            if kind is TILE:
                order = list(TILE_ORDER)
                convert = lambda image, art_type: to_tile(image, art_type, system_key)  # noqa: E731
            else:
                order = type_order(options.art_type)
                convert = lambda image, art_type: to_hero(image, art_type, system_key)  # noqa: E731
            found = self._download(game, order, convert, failures)
            if found:
                image, candidate = found
                source = f"{candidate.source}:{candidate.art_type}:{candidate.url}"
                if slot.unchanged(source, options.force):
                    return Result(entry, "unchanged", kind=kind)
                message = f"{ART_TYPE_LABELS.get(candidate.art_type, candidate.art_type)} von {candidate.description}"
                if game.match and kind is ART:
                    message += f" [{game.match}]"
                return self._write(entry, slot, image, source, "downloaded", message)

        reasons = [self._identity_hint(game)]
        if note:
            reasons.append(note)
        if failures:
            reasons.append(f"{len(failures)} Bild(er) unbrauchbar: {failures[0]}")
        icon = game.header.icon if game.header else None
        if options.icon_fallback and icon is not None:
            source = "icon:1"
            if slot.existing and slot.ours and (slot.record or {}).get("source") == source:
                return Result(entry, "unchanged", kind=kind)
            if kind is TILE:
                image = icon_tile(icon)
            else:
                image = icon_art(icon, hero_color_for(entry.parent_folders, entry.system))
            return self._write(entry, slot, image, source, "icon", f" ({'; '.join(reasons)})")

        # say why there is no picture at all
        if not options.icon_fallback:
            reasons.append("Icon-Ersatzbild ist ausgeschaltet")
        elif system_key != "nds":
            reasons.append("nur DS-Spiele haben ein Icon")
        elif game.header is None:
            reasons.append("ROM-Kopf nicht lesbar")
        else:
            reasons.append("das ROM hat kein Icon")
        return Result(entry, "missing", f" ({'; '.join(reasons)})", kind=kind)

    def _identity_hint(self, game: GameInfo) -> str:
        if not self.options.download:
            return "ohne Download"
        if game.match:
            return f"{game.match}: {game.display_name}"
        return "Spiel nicht erkannt"

    def _download(self, game: GameInfo, order: list[str], convert: Callable[[Image.Image, str], Image.Image],
                  failures: list[str]):
        """First candidate that downloads and converts, as (converted image, candidate).

        A broken download or an image that fails to convert only skips that candidate; the caller
        then still has the icon fallback."""
        for art_type in order:
            for source in self.sources:
                if art_type not in source.art_types:
                    continue
                for candidate in source.candidates(game, art_type):
                    if self.cancel.is_set():
                        return None
                    try:
                        image = source.load(candidate)
                    except NetworkError as error:
                        self._warn_source(source, error)
                        break
                    except Exception as error:  # noqa: BLE001 - try the next candidate
                        failures.append(f"{candidate.description}: {type(error).__name__}: {error}")
                        continue
                    if image is None:
                        continue
                    try:
                        return convert(image, art_type), candidate
                    except Exception as error:  # noqa: BLE001 - e.g. an unusual image mode
                        failures.append(f"{candidate.description}: {type(error).__name__}: {error}")
        return None

    def _warn_source(self, source: ArtSource, error: Exception) -> None:
        with self._lock:
            if source.id in self._warned_sources:
                return
            self._warned_sources.add(source.id)
        self.log(f"{source.label} nicht erreichbar ({error}), diese Quelle wird übersprungen.", "warn")

    def _write(self, entry: Entry, slot: Slot, image: Image.Image, source: str, status: str,
               message: str = "") -> Result:
        kind = slot.kind
        data = to_ds_bmp(image)
        validation = validate_bmp(data, kind.size)
        if not validation.ok:
            return Result(entry, "error", "Umwandlung fehlgeschlagen: " + "; ".join(validation.errors), kind=kind)
        record = {"path": slot.sd_path, "source": source, "sha1": _sha1(data), "time": int(time.time())}
        if not self.options.dry_run:
            _write_file(slot.target, data)
        replaced = slot.existing and not slot.ours
        return Result(entry, status, message, slot.key, record, kind=kind, replaced_manual=replaced)

    # --- housekeeping ------------------------------------------------------------------------

    def _forget_deleted(self) -> None:
        for manifest in self.manifests.values():
            for key, record in list(manifest.records.items()):
                if not to_fs_path(self.sd_root, record.get("path") or key).is_file():
                    del manifest.records[key]

    def _clean(self, result: ScanResult, kind: Kind) -> None:
        """Deletes files this tool wrote for entries that are gone from the card."""
        if kind is ART:
            current = {entry.art_sd_path.lower() for entry in result.entries if entry.art_sd_path}
        else:
            current = {entry.tile_sd_path.lower() for entry in result.entries if entry.tile_sd_path}
        manifest = self.manifests[kind.key]
        for key, record in list(manifest.records.items()):
            if key in current:
                continue
            path = to_fs_path(self.sd_root, record.get("path") or key)
            try:
                if _sha1(path.read_bytes()) != record.get("sha1"):
                    continue
                if not self.options.dry_run:
                    path.unlink()
                    del manifest.records[key]
                self._stats(kind).removed += 1
                self.log(f"Gelöscht (Spiel fehlt): {record.get('path') or key}", "info")
            except OSError:
                continue


def _write_file(target: Path, data: bytes) -> None:
    """Writes atomically. Retries the rename: on Windows a virus scanner or the Explorer preview may
    hold the new file for a moment (PermissionError)."""
    target.parent.mkdir(parents=True, exist_ok=True)
    temp = target.with_name(target.name + ".tmp")
    temp.write_bytes(data)
    for attempt in range(5):
        try:
            os.replace(temp, target)
            return
        except PermissionError:
            if attempt == 4:
                try:
                    temp.unlink()
                except OSError:
                    pass
                raise
            time.sleep(0.2 * (attempt + 1))


def check_card(sd_root: Path, log: Log) -> int:
    """Validates every artwork and tile image on the card like the launcher would. Returns the number of
    bad files."""
    total_bad = 0
    for kind in KINDS:
        root = to_fs_path(Path(sd_root), kind.folder)
        bad = 0
        count = 0
        for folder, _, files in os.walk(root):
            for file_name in sorted(files):
                if not file_name.lower().endswith(".bmp"):
                    continue
                count += 1
                path = Path(folder) / file_name
                validation = validate_bmp(path.read_bytes(), kind.size)
                relative = path.relative_to(root).as_posix()
                if kind is TILE:
                    relative = "tiles/" + relative
                if not validation.ok:
                    bad += 1
                    log(f"✗ {relative}: {'; '.join(validation.errors)}", "error")
                for warning in validation.warnings:
                    log(f"~ {relative}: {warning}", "warn")
        if kind is ART or count:
            log(f"{count} {kind.plural} geprüft, {bad} unbrauchbar.", "info")
        total_bad += bad
    return total_bad
