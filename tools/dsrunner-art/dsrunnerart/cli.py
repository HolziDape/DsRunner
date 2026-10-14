"""Command line interface. Without arguments the GUI starts."""
from __future__ import annotations

import argparse
import sys
import threading
from pathlib import Path
from typing import Optional

from . import __version__
from .names import REGION_PREFERENCES
from .sdcard import normalize_sd_root
from .sources import ART_TYPES, SOURCE_IDS

TYPE_ALIASES = {"titel": "title", "titelbild": "title", "screenshot": "snap", "cover": "boxart", "box": "boxart"}


def _art_type(value: str) -> str:
    value = TYPE_ALIASES.get(value.lower(), value.lower())
    if value not in ART_TYPES:
        raise argparse.ArgumentTypeError(f"unbekannte Art {value!r} (title, snap, boxart)")
    return value


def _sources(value: str) -> tuple[str, ...]:
    sources = tuple(part.strip().lower() for part in value.split(",") if part.strip())
    unknown = [source for source in sources if source not in SOURCE_IDS]
    if unknown:
        raise argparse.ArgumentTypeError(f"unbekannte Quelle {', '.join(unknown)} (bekannt: {', '.join(SOURCE_IDS)})")
    return sources


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="dsrunner_art",
        description="DsRunner Art: lädt Artworks (oberer Bildschirm), Kachel-Bilder (3×2), Cover (CoverFlow) und "
                    "kleine Icons (4×3) für DsRunner auf die SD-Karte. "
                    "Ohne Argumente öffnet sich das Fenster.")
    parser.add_argument("--sd", metavar="LAUFWERK", help="SD-Karte bzw. Ordner, z. B. E:\\ oder /media/sd")
    parser.add_argument("--type", type=_art_type, default="title",
                        help="bevorzugte Bildart: title (Titelbild, Standard), snap (Screenshot), boxart (Cover)")
    parser.add_argument("--region", choices=sorted(REGION_PREFERENCES), default="de",
                        help="bevorzugte Region bei mehreren Fassungen (Standard: de)")
    parser.add_argument("--force", action="store_true",
                        help="vom Tool erzeugte Artworks neu laden und überschreiben")
    parser.add_argument("--offline", action="store_true",
                        help="nichts herunterladen (nur eigene Bilder, Konsolen-Bilder und Icons)")
    parser.add_argument("--override", metavar="ORDNER", type=Path,
                        help="Ordner mit eigenen Bildern (Standard: <SD>/_dsrunner/art-override)")
    parser.add_argument("--sources", type=_sources, default=SOURCE_IDS,
                        help=f"Quellen in dieser Reihenfolge (Standard: {','.join(SOURCE_IDS)})")
    parser.add_argument("--no-console-art", action="store_true", help="keine Bilder für Ordner erzeugen (Ordner mit eingebautem Konsolen-Bild des Themes bekommen ohnehin keins)")
    parser.add_argument("--no-icon", action="store_true",
                        help="kein Ersatzbild aus dem Spiel-Icon, wenn nichts gefunden wird")
    parser.add_argument("--no-tiles", action="store_true",
                        help="keine Kachel-Bilder (64x64, /_dsrunner/tiles) für das Raster des Launchers erzeugen")
    parser.add_argument("--no-covers", action="store_true",
                        help="keine Cover (128x96, /_pico/covers/user) für die CoverFlow-Ansicht erzeugen")
    parser.add_argument("--no-icons", action="store_true",
                        help="keine kleinen Icons (32x32, /_pico/icons/user) für Spiele ohne eigenes Icon erzeugen "
                             "(nicht zu verwechseln mit --no-icon)")
    parser.add_argument("--tile-override", metavar="ORDNER", type=Path,
                        help="Ordner mit eigenen Kachel-Bildern (Standard: <SD>/_dsrunner/tile-override)")
    parser.add_argument("--replace-manual", action="store_true",
                        help="auch von Hand abgelegte Bilder ersetzen, wenn ein Bild gefunden wird")
    parser.add_argument("--no-crc", action="store_true", help="Spiele nicht über die Prüfsumme erkennen")
    parser.add_argument("--clean", action="store_true",
                        help="Bilder löschen, die das Tool für inzwischen gelöschte Spiele angelegt hat")
    parser.add_argument("--dry-run", action="store_true", help="nur anzeigen, nichts schreiben")
    parser.add_argument("--check", action="store_true", help="nur die vorhandenen Artworks, Kachel-Bilder, Cover und Icons prüfen")
    parser.add_argument("--library", metavar="PFAD", help="Bibliotheksordner statt libraryPath aus settings.json")
    parser.add_argument("--cache", metavar="ORDNER", type=Path, help="Cache-Ordner auf dem PC")
    parser.add_argument("--retry-missing", action="store_true",
                        help="auch Bilder erneut suchen, die kürzlich nicht gefunden wurden")
    parser.add_argument("--workers", type=int, default=4, help="parallele Downloads (Standard: 4)")
    parser.add_argument("-v", "--verbose", action="store_true", help="ausführliche Ausgabe")
    parser.add_argument("--gui", action="store_true", help="Fenster öffnen")
    parser.add_argument("--version", action="version", version=f"DsRunner Art {__version__}")
    return parser


def _print_log(verbose: bool):
    def log(message: str, level: str = "info") -> None:
        if level == "detail" and not verbose:
            return
        stream = sys.stderr if level == "error" else sys.stdout
        if stream:
            print(message, file=stream, flush=True)
    return log


def main(argv: Optional[list[str]] = None) -> int:
    for stream in (sys.stdout, sys.stderr):
        # Windows consoles with a legacy code page can't print every file name
        if stream and hasattr(stream, "reconfigure"):
            try:
                stream.reconfigure(errors="replace")
            except (ValueError, OSError):
                pass
    argv = sys.argv[1:] if argv is None else argv
    if not argv or argv == ["--gui"]:
        from .gui import run_gui
        return run_gui()

    args = build_parser().parse_args(argv)
    if args.gui:
        from .gui import run_gui
        return run_gui(args.sd)
    if not args.sd:
        build_parser().error("--sd fehlt (oder ohne Argumente starten, dann öffnet sich das Fenster)")

    from .updater import Options, Updater, check_card

    sd_root = normalize_sd_root(args.sd)
    log = _print_log(args.verbose)
    if args.check:
        return 1 if check_card(sd_root, log) else 0

    options = Options(
        art_type=args.type, force=args.force, download=not args.offline, console_art=not args.no_console_art,
        icon_fallback=not args.no_icon, region=args.region, override_dir=args.override, sources=args.sources,
        clean=args.clean, dry_run=args.dry_run, cache_dir=args.cache, workers=args.workers, use_crc=not args.no_crc,
        library_path=args.library, refresh_misses=args.retry_missing, tiles=not args.no_tiles,
        tile_override_dir=args.tile_override, replace_manual=args.replace_manual, covers=not args.no_covers,
        icons=not args.no_icons)

    last_percent = [-1]

    def progress(done: int, total: int, name: str) -> None:
        if args.verbose or not total or not sys.stdout:
            return
        percent = done * 100 // total
        if percent // 10 != last_percent[0] // 10:
            last_percent[0] = percent
            print(f"[{percent:3d} %] {done}/{total}", flush=True)

    cancel = threading.Event()
    updater = Updater(sd_root, options, log, progress, cancel)
    try:
        stats = updater.run()
    except KeyboardInterrupt:
        cancel.set()
        return 130
    except FileNotFoundError as error:
        log(str(error), "error")
        return 2
    return 1 if stats.total_errors else 0
