"""python -m dsstream --url URL [--mapping default.json] [--mode 256x192] ..."""
from __future__ import annotations

import argparse
import asyncio
import logging
import sys

from . import protocol as P
from .browser import Browser
from .mapping import Mapping
from .server import StreamServer, serve

DEFAULT_VIEWPORT = {
    P.MODE_256x192: (512, 384),
    P.MODE_128x96: (512, 384),
    P.MODE_256x384: (512, 768),
}


def parse_size(s: str) -> tuple[int, int]:
    w, h = s.lower().split("x")
    return int(w), int(h)


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="dsstream", description="DsRunner Stream host: streamt eine "
                                 "Webseite (Browser-Spiel) auf den Nintendo DS.")
    ap.add_argument("--url", required=True, help="Adresse der Seite, oder 'test' fuer die eingebaute Testseite")
    ap.add_argument("--mapping", default="default.json",
                    help="Tastenbelegung (Datei oder Name in mappings/), Standard default.json")
    ap.add_argument("--mode", default="256x192", choices=list(P.MODES),
                    help="256x192 (ein Bildschirm), 128x96 (schneller, gröber) oder 256x384 (beide Bildschirme)")
    ap.add_argument("--viewport", type=parse_size, default=None,
                    help="Groesse der Seite im Browser, z. B. 640x480 (Standard 512x384, bei 256x384: 512x768)")
    ap.add_argument("--port", type=int, default=P.DEFAULT_PORT, help="UDP-Port (Standard 7770)")
    ap.add_argument("--bind", default="0.0.0.0", help="nur an dieser IP lauschen (dann klappt die automatische Suche nicht, "
                    "host= in stream.ini eintragen)")
    ap.add_argument("--name", default=None, help="Name, den der DS bei der Suche anzeigt")
    ap.add_argument("--fps", type=float, default=20.0, help="hoechstens so viele Bilder pro Sekunde (Standard 20)")
    ap.add_argument("--rate", type=float, default=120.0, help="Start-Senderate in KB/s (Standard 120)")
    ap.add_argument("--min-rate", type=float, default=40.0, help="kleinste Senderate in KB/s")
    ap.add_argument("--max-rate", type=float, default=200.0, help="groesste Senderate in KB/s (Standard 200)")
    ap.add_argument("--palette", default="adaptive", choices=["adaptive", "fixed"],
                    help="adaptive: 256 Farben passend zum Bild (Standard), fixed: feste Farben (weniger CPU)")
    ap.add_argument("--no-lz4", action="store_true", help="ohne Kompression (nur zum Testen)")
    ap.add_argument("--headful", action="store_true",
                    help="Browser mit Fenster starten (auf dem Raspberry Pi empfohlen: GPU fuer WebGL)")
    ap.add_argument("--gpu", action="store_true", help="GPU-Flags fuer Chromium setzen")
    ap.add_argument("--browser", default="auto",
                    help="auto (Pi: System-Chromium, sonst Playwright), bundled, oder Pfad zu chrome/chromium")
    ap.add_argument("--capture", default="screencast", choices=["screencast", "screenshot"],
                    help="Aufnahme per CDP-Screencast (Standard) oder einzelnen Screenshots")
    ap.add_argument("--mute", action="store_true", help="Ton der Seite stummschalten")
    ap.add_argument("--broadcast", action="store_true",
                    help="Bilder als Broadcast senden (hilft, wenn der DS im WLAN ist, aber kaum Bilder ankommen)")
    ap.add_argument("--packet-size", type=int, default=P.MAX_CHUNK,
                    help="groesste UDP-Paketgroesse in Bytes (Standard 1200, hoechstens 1272: der DS nimmt nur "
                         "IP-Pakete bis 1300 Byte an; bei Empfangsproblemen 1000 oder 600 probieren)")
    ap.add_argument("--jpeg", action="store_true",
                    help="Chromium liefert die Bilder als JPEG statt PNG (auf dem Pi oft mehr Bilder pro Sekunde)")
    ap.add_argument("--allow-popups", action="store_true",
                    help="neue Fenster/Pop-ups streamen statt sie zu schliessen (Standard: schliessen)")
    ap.add_argument("--chromium-arg", action="append", default=[], help="zusaetzliches Chromium-Argument")
    ap.add_argument("--stats", type=float, default=5.0, help="Statistik alle N Sekunden ausgeben")
    ap.add_argument("-v", "--verbose", action="store_true")
    return ap


async def amain(args) -> None:
    mode = P.MODES[args.mode]
    mapping = Mapping.load(args.mapping)
    viewport = args.viewport or DEFAULT_VIEWPORT[mode]
    out_size = P.MODE_SIZE[mode]
    browser = Browser(args.url, viewport, out_size, headless=not args.headful, gpu=args.gpu,
                      executable=args.browser, capture=args.capture, max_fps=args.fps,
                      mute=args.mute, extra_args=args.chromium_arg,
                      allow_popups=args.allow_popups,
                      image_format="jpeg" if args.jpeg else "png")
    await browser.start()
    logging.getLogger("dsstream").info("Modus %s, Seite %dx%d, Belegung \"%s\"", args.mode,
                                       viewport[0], viewport[1], mapping.name)
    server = StreamServer(browser, mapping, mode, port=args.port, name=args.name,
                          palette=args.palette, use_lz4=not args.no_lz4, max_fps=args.fps,
                          rate_kbs=args.rate, min_rate_kbs=args.min_rate, max_rate_kbs=args.max_rate,
                          stats_every=args.stats, broadcast=args.broadcast,
                          packet_size=args.packet_size,
                          noise=3 if args.jpeg else 0)
    try:
        await serve(server, args.bind)
    finally:
        await browser.close()


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format="%(asctime)s %(name)s: %(message)s", datefmt="%H:%M:%S")
    if not args.verbose:
        logging.getLogger("asyncio").setLevel(logging.WARNING)
    try:
        asyncio.run(amain(args))
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
