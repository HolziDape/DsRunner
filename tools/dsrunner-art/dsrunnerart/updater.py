"""Runs an artwork update: scan the card, pick a source per entry, convert and write the BMPs.

Bookkeeping: `/_dsrunner/art/.dsrunner-art.json` on the card remembers which files this tool wrote
(with their SHA-1), so files the user put there by hand are never overwritten and later runs only
do what is missing.
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
from .imaging import console_art, icon_art, open_image, to_ds_bmp, to_hero, validate_bmp
from .names import REGION_PREFERENCES
from .romheader import read_header
from .net import HttpClient, NetworkError, default_cache_dir
from .sdcard import ART_FOLDER, MANIFEST_NAME, OVERRIDE_FOLDER, Entry, ScanResult, scan, to_fs_path
from .sources import ART_TYPE_LABELS, ART_TYPES, SOURCE_IDS, ArtSource, build_sources, type_order

IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp")
# bump to regenerate the generated console artworks after a design change
GENERATED_VERSION = 1
# artworks with these sources are replaced as soon as a real image is found
_RETRY_SOURCES = ("icon:",)

Log = Callable[[str, str], None]
Progress = Callable[[int, int, str], None]


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
    missing: int = 0
    errors: int = 0
    removed: int = 0
    missing_names: list[str] = field(default_factory=list)

    def summary(self) -> str:
        parts = [
            f"{self.downloaded} geladen",
            f"{self.overrides} eigene Bilder",
            f"{self.generated} Konsolen/Ordner erzeugt",
            f"{self.icons} aus dem Spiel-Icon",
            f"{self.unchanged} unverändert",
        ]
        if self.foreign:
            parts.append(f"{self.foreign} von Hand abgelegt (nicht angefasst)")
        if self.removed:
            parts.append(f"{self.removed} verwaiste gelöscht")
        parts.append(f"{self.missing} ohne Bild")
        if self.errors:
            parts.append(f"{self.errors} Fehler")
        return ", ".join(parts)


@dataclass
class Result:
    entry: Entry
    status: str  # downloaded, generated, icon, override, unchanged, foreign, missing, error, skipped
    message: str = ""
    art_key: Optional[str] = None
    record: Optional[dict] = None


def _sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


class Manifest:
    """Which artworks this tool wrote: {lower case art path: {path, source, sha1}}."""

    def __init__(self, path: Path):
        self.path = path
        self.records: dict[str, dict] = {}
        try:
            with open(path, encoding="utf-8") as file:
                data = json.load(file)
            if isinstance(data, dict) and isinstance(data.get("artworks"), dict):
                self.records = data["artworks"]
        except (OSError, ValueError):
            self.records = {}

    def get(self, key: str) -> Optional[dict]:
        return self.records.get(key)

    def save(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temp = self.path.with_name(self.path.name + ".tmp")
        payload = {"tool": f"DsRunner Art {__version__}", "artworks": dict(sorted(self.records.items()))}
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
        art = entry.art_sd_path
        if art:
            relative = art[len(ART_FOLDER) + 1:-len(".bmp")].lower()
            if relative in self.by_path:
                return self.by_path[relative]
        name = entry.name
        if not entry.is_folder and "." in name[1:]:
            name = name.rsplit(".", 1)[0]
        return self.by_name.get(name.lower())


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
        self.manifest = Manifest(to_fs_path(self.sd_root, ART_FOLDER) / MANIFEST_NAME)
        override_dir = options.override_dir or to_fs_path(self.sd_root, OVERRIDE_FOLDER)
        self.overrides = OverrideIndex(Path(override_dir))
        self.stats = Stats()
        self._warned_sources: set[str] = set()
        self._lock = threading.Lock()

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
        if len(self.overrides):
            self.log(f"Eigene Bilder: {len(self.overrides)} in {self.overrides.root}", "info")
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
                    outcome = future.result()
                    done += 1
                    self._apply(outcome)
                    self.progress(done, total, outcome.entry.name)
            if options.clean and not self.cancel.is_set():
                self._clean(result)
        finally:
            self._forget_deleted()
            if not options.dry_run:
                try:
                    self.manifest.save()
                except OSError as error:
                    self.log(f"Liste der Artworks nicht gespeichert: {error}", "error")
            self.client.save()
            self.crc_cache.save()
        if self.cancel.is_set():
            self.log("Abgebrochen.", "warn")
        self.log(f"Fertig: {self.stats.summary()}.", "info")
        return self.stats

    def _apply(self, outcome: Result) -> None:
        stats = self.stats
        name = outcome.entry.sd_path
        status = outcome.status
        if outcome.record is not None and outcome.art_key and not self.options.dry_run:
            self.manifest.records[outcome.art_key] = outcome.record
        if status == "downloaded":
            stats.downloaded += 1
            self.log(f"✓ {name}: {outcome.message}", "info")
        elif status == "override":
            stats.overrides += 1
            self.log(f"✓ {name}: eigenes Bild {outcome.message}", "info")
        elif status == "generated":
            stats.generated += 1
            self.log(f"✓ {name}: erzeugt", "detail")
        elif status == "icon":
            stats.icons += 1
            self.log(f"~ {name}: kein Bild gefunden, Icon des Spiels verwendet{outcome.message}", "info")
        elif status == "unchanged":
            stats.unchanged += 1
            self.log(f"· {name}: vorhanden", "detail")
        elif status == "foreign":
            stats.foreign += 1
            self.log(f"· {name}: von Hand abgelegtes Bild bleibt", "detail")
        elif status == "missing":
            stats.missing += 1
            stats.missing_names.append(name)
            self.log(f"✗ {name}: kein Bild gefunden{outcome.message}", "warn")
        elif status == "error":
            stats.errors += 1
            self.log(f"✗ {name}: {outcome.message}", "error")

    # --- one entry ---------------------------------------------------------------------------

    def _process_safe(self, entry: Entry) -> Result:
        if self.cancel.is_set():
            return Result(entry, "skipped")
        try:
            return self._process(entry)
        except Exception as error:  # noqa: BLE001 - one broken file must not stop the run
            return Result(entry, "error", f"{type(error).__name__}: {error}")

    def _process(self, entry: Entry) -> Result:
        options = self.options
        art_sd = entry.art_sd_path
        if not art_sd:
            return Result(entry, "error", "Pfad zu lang für den Launcher (max. 255 Zeichen)")
        target = to_fs_path(self.sd_root, art_sd)
        key = art_sd.lower()
        record = self.manifest.get(key)
        existing = target.is_file()
        ours = False
        if existing and record:
            try:
                ours = _sha1(target.read_bytes()) == record.get("sha1")
            except OSError:
                ours = False

        def unchanged_or(source: str) -> bool:
            return existing and ours and record.get("source") == source and not options.force

        # 1. the user's own image always wins
        override = self.overrides.find(entry)
        if override:
            try:
                stat = override.stat()
                source = f"override:{override.name}:{stat.st_size}:{stat.st_mtime_ns}"
            except OSError as error:
                return Result(entry, "error", f"{override}: {error}")
            if unchanged_or(source):
                return Result(entry, "unchanged")
            image = open_image(override.read_bytes())
            if image is None:
                return Result(entry, "error", f"{override} ist kein lesbares Bild")
            return self._write(entry, target, key, to_hero(image, "custom"), source, "override", str(override.name))

        # 2. never touch files someone put there by hand
        if existing and not ours:
            return Result(entry, "foreign")

        if entry.is_folder:
            if not options.console_art:
                return Result(entry, "skipped")
            names = entry.parent_folders + [entry.name]
            console = find_by_folder_name(entry.name) if entry.depth == 0 else None
            label = console.label if console else entry.name
            hero = hero_color_for(names)
            source = f"generated:{GENERATED_VERSION}:{label}:{hero}"
            if unchanged_or(source):
                return Result(entry, "unchanged")
            image = console_art(label, hero, tile_color_for(names))
            return self._write(entry, target, key, image, source, "generated")

        # 3. games: keep what is there unless forced (icon fallbacks are retried)
        if existing and ours and not options.force and not str(record.get("source", "")).startswith(_RETRY_SOURCES):
            return Result(entry, "unchanged")

        if options.download:
            game = identify(entry, self.catalog, REGION_PREFERENCES[options.region], self.crc_cache, options.use_crc)
        else:
            # without downloads only the header matters (for the icon)
            system = entry.system
            game = GameInfo(entry, system, read_header(entry.fs_path, system.key) if system.key == "nds" else None)
        if not options.download:
            hint = " (ohne Download)"
        elif game.match:
            hint = f" ({game.match}: {game.display_name})"
        else:
            hint = " (Spiel nicht erkannt)"
        if options.download:
            found = self._download(game)
            if found:
                image, candidate = found
                source = f"{candidate.source}:{candidate.art_type}:{candidate.url}"
                if unchanged_or(source):
                    return Result(entry, "unchanged")
                message = f"{ART_TYPE_LABELS.get(candidate.art_type, candidate.art_type)} von {candidate.description}"
                if game.match:
                    message += f" [{game.match}]"
                return self._write(entry, target, key, to_hero(image, candidate.art_type), source, "downloaded", message)

        if options.icon_fallback and game.header and game.header.icon is not None:
            source = "icon:1"
            if unchanged_or(source) or (existing and ours and record.get("source") == source):
                return Result(entry, "unchanged")
            hero = hero_color_for(entry.parent_folders, entry.system)
            return self._write(entry, target, key, icon_art(game.header.icon, hero), source, "icon", hint)
        return Result(entry, "missing", hint)

    def _download(self, game: GameInfo):
        for art_type in type_order(self.options.art_type):
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
                    if image is not None:
                        return image, candidate
        return None

    def _warn_source(self, source: ArtSource, error: Exception) -> None:
        with self._lock:
            if source.id in self._warned_sources:
                return
            self._warned_sources.add(source.id)
        self.log(f"{source.label} nicht erreichbar ({error}), diese Quelle wird übersprungen.", "warn")

    def _write(self, entry: Entry, target: Path, key: str, image: Image.Image, source: str, status: str,
               message: str = "") -> Result:
        data = to_ds_bmp(image)
        validation = validate_bmp(data)
        if not validation.ok:
            return Result(entry, "error", "Umwandlung fehlgeschlagen: " + "; ".join(validation.errors))
        record = {"path": entry.art_sd_path, "source": source, "sha1": _sha1(data), "time": int(time.time())}
        if not self.options.dry_run:
            target.parent.mkdir(parents=True, exist_ok=True)
            temp = target.with_name(target.name + ".tmp")
            temp.write_bytes(data)
            os.replace(temp, target)
        return Result(entry, status, message, key, record)

    # --- housekeeping ------------------------------------------------------------------------

    def _forget_deleted(self) -> None:
        for key, record in list(self.manifest.records.items()):
            if not to_fs_path(self.sd_root, record.get("path") or key).is_file():
                del self.manifest.records[key]

    def _clean(self, result: ScanResult) -> None:
        """Deletes artworks this tool wrote for entries that are gone from the card."""
        current = {entry.art_sd_path.lower() for entry in result.entries if entry.art_sd_path}
        for key, record in list(self.manifest.records.items()):
            if key in current:
                continue
            path = to_fs_path(self.sd_root, record.get("path") or key)
            try:
                if _sha1(path.read_bytes()) != record.get("sha1"):
                    continue
                if not self.options.dry_run:
                    path.unlink()
                    del self.manifest.records[key]
                self.stats.removed += 1
                self.log(f"Gelöscht (Spiel fehlt): {record.get('path') or key}", "info")
            except OSError:
                continue


def check_card(sd_root: Path, log: Log) -> int:
    """Validates every artwork on the card like the launcher would. Returns the number of bad files."""
    art_root = to_fs_path(Path(sd_root), ART_FOLDER)
    bad = 0
    count = 0
    for folder, _, files in os.walk(art_root):
        for file_name in sorted(files):
            if not file_name.lower().endswith(".bmp"):
                continue
            count += 1
            path = Path(folder) / file_name
            validation = validate_bmp(path.read_bytes())
            relative = path.relative_to(art_root).as_posix()
            if not validation.ok:
                bad += 1
                log(f"✗ {relative}: {'; '.join(validation.errors)}", "error")
            for warning in validation.warnings:
                log(f"~ {relative}: {warning}", "warn")
    log(f"{count} Artworks geprüft, {bad} unbrauchbar.", "info")
    return bad
