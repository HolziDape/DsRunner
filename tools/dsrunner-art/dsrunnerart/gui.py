"""Small tkinter window: choose the card, press "Bilder aktualisieren"."""
from __future__ import annotations

import json
import os
import queue
import string
import subprocess
import sys
import threading
from pathlib import Path
from typing import Optional

from . import __version__
from .net import default_cache_dir
from .sdcard import ART_FOLDER, OVERRIDE_FOLDER, normalize_sd_root, read_library_path, to_fs_path

REGIONS = [("de", "Deutschland"), ("eu", "Europa"), ("us", "USA"), ("jp", "Japan")]
ART_TYPE_CHOICES = [("title", "Titelbild"), ("snap", "Screenshot"), ("boxart", "Cover")]


def _settings_path() -> Path:
    # next to the cache: %LOCALAPPDATA%\DsRunnerArt\cache -> %LOCALAPPDATA%\DsRunnerArt\settings.json
    cache = default_cache_dir()
    return cache.parent / "settings.json" if cache.name == "cache" else cache / "settings.json"


def _load_settings() -> dict:
    try:
        with open(_settings_path(), encoding="utf-8") as file:
            data = json.load(file)
        return data if isinstance(data, dict) else {}
    except (OSError, ValueError):
        return {}


def _save_settings(settings: dict) -> None:
    try:
        path = _settings_path()
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(settings, indent=1), encoding="utf-8")
    except OSError:
        pass


def is_dsrunner_card(root: Path) -> bool:
    return (root / "_pico").is_dir() or (root / "_dsrunner").is_dir()


def list_drives() -> list[tuple[str, str]]:
    """(path, label) of the drives that may be the SD card, DsRunner cards first."""
    drives: list[tuple[str, str, bool]] = []
    if os.name == "nt":
        import ctypes
        kernel32 = ctypes.windll.kernel32
        mask = kernel32.GetLogicalDrives()
        for index, letter in enumerate(string.ascii_uppercase):
            if not mask & (1 << index):
                continue
            root = f"{letter}:\\"
            drive_type = kernel32.GetDriveTypeW(root)
            # 2 = removable, 3 = fixed (card readers sometimes report fixed)
            if drive_type not in (2, 3):
                continue
            try:
                if not os.path.isdir(root):
                    continue
            except OSError:
                continue
            name_buffer = ctypes.create_unicode_buffer(261)
            kernel32.GetVolumeInformationW(root, name_buffer, 261, None, None, None, None, 0)
            card = is_dsrunner_card(Path(root))
            if drive_type == 3 and not card:
                continue
            label = f"{letter}:  {name_buffer.value}".rstrip()
            drives.append((root, label + ("   (DsRunner)" if card else ""), card))
    else:
        candidates: list[Path] = []
        for base in ("/media", f"/media/{os.environ.get('USER', '')}", f"/run/media/{os.environ.get('USER', '')}",
                     "/mnt", "/Volumes"):
            try:
                candidates += [path for path in Path(base).iterdir() if path.is_dir()]
            except OSError:
                continue
        for path in candidates:
            card = is_dsrunner_card(path)
            drives.append((str(path), str(path) + ("   (DsRunner)" if card else ""), card))
    drives.sort(key=lambda drive: (not drive[2], drive[0]))
    return [(path, label) for path, label, _ in drives]


def open_folder(path: Path) -> None:
    try:
        if os.name == "nt":
            os.startfile(path)  # type: ignore[attr-defined]
        elif sys.platform == "darwin":
            subprocess.Popen(["open", str(path)])
        else:
            subprocess.Popen(["xdg-open", str(path)])
    except OSError:
        pass


def run_gui(initial_sd: Optional[str] = None) -> int:
    try:
        import tkinter as tk
        from tkinter import filedialog, messagebox, ttk
        from tkinter.scrolledtext import ScrolledText
    except ImportError:
        print("tkinter fehlt. Unter Windows bei der Python-Installation \"tcl/tk and IDLE\" auswählen, "
              "oder das Tool mit --sd LAUFWERK auf der Kommandozeile starten.", file=sys.stderr)
        return 2

    if os.name == "nt":
        try:
            import ctypes
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except (AttributeError, OSError):
            pass

    from .updater import Options, Updater, check_card

    settings = _load_settings()
    root = tk.Tk()
    root.title(f"DsRunner Art {__version__}")
    root.minsize(560, 520)
    try:
        style = ttk.Style(root)
        if "vista" in style.theme_names():
            style.theme_use("vista")
        elif "clam" in style.theme_names():
            style.theme_use("clam")
    except tk.TclError:
        pass

    messages: "queue.Queue[tuple]" = queue.Queue()
    state = {"thread": None, "cancel": None}

    # --- variables ---------------------------------------------------------------------------
    drive_var = tk.StringVar()
    type_var = tk.StringVar(value=settings.get("type", "title"))
    region_var = tk.StringVar(value=dict(REGIONS).get(settings.get("region", "de"), "Deutschland"))
    force_var = tk.BooleanVar(value=False)
    console_var = tk.BooleanVar(value=settings.get("console_art", True))
    icon_var = tk.BooleanVar(value=settings.get("icon_fallback", True))
    clean_var = tk.BooleanVar(value=False)
    tiles_var = tk.BooleanVar(value=settings.get("tiles", True))
    covers_var = tk.BooleanVar(value=settings.get("covers", True))
    icons_var = tk.BooleanVar(value=settings.get("icons", True))
    replace_var = tk.BooleanVar(value=False)
    override_var = tk.StringVar(value=settings.get("override", ""))
    details_var = tk.BooleanVar(value=False)
    status_var = tk.StringVar(value="Bereit.")
    card_info_var = tk.StringVar(value="")

    drives: list[tuple[str, str]] = []

    main = ttk.Frame(root, padding=12)
    main.pack(fill="both", expand=True)
    main.columnconfigure(1, weight=1)

    ttk.Label(main, text="DsRunner Art", font=("Segoe UI", 14, "bold")).grid(row=0, column=0, columnspan=4, sticky="w")
    ttk.Label(main, text="Lädt die Bilder für den oberen Bildschirm, die Kacheln (3×2), die Cover (CoverFlow) und "
                         "die kleinen Icons (4×3) auf die SD-Karte.", wraplength=520).grid(
        row=1, column=0, columnspan=4, sticky="w", pady=(0, 10))

    # --- drive -------------------------------------------------------------------------------
    ttk.Label(main, text="SD-Karte:").grid(row=2, column=0, sticky="w")
    drive_box = ttk.Combobox(main, textvariable=drive_var, state="readonly")
    drive_box.grid(row=2, column=1, sticky="ew", padx=(6, 6))

    def selected_root() -> Optional[Path]:
        value = drive_var.get()
        for path, label in drives:
            if label == value:
                return Path(path)
        return normalize_sd_root(value) if value else None

    def update_card_info(*_):
        sd_root = selected_root()
        if not sd_root or not sd_root.is_dir():
            card_info_var.set("Bitte die SD-Karte einstecken und auswählen.")
            return
        library = read_library_path(sd_root) or "/"
        exists = to_fs_path(sd_root, library).is_dir()
        if not is_dsrunner_card(sd_root):
            card_info_var.set("Hinweis: Auf diesem Laufwerk ist (noch) kein DsRunner (_pico fehlt).")
        elif exists:
            card_info_var.set(f"Bibliothek: {library}")
        else:
            card_info_var.set(f"Bibliothek {library} fehlt, es wird das Hauptverzeichnis verwendet.")

    def refresh_drives(select: Optional[str] = None):
        nonlocal drives
        drives = list_drives()
        labels = [label for _, label in drives]
        if select and select not in [path for path, _ in drives]:
            drives.append((select, select))
            labels.append(select)
        drive_box["values"] = labels
        wanted = select or settings.get("sd")
        chosen = next((label for path, label in drives if wanted and os.path.normcase(path) == os.path.normcase(wanted)), None)
        if chosen is None and labels:
            chosen = labels[0]
        drive_var.set(chosen or "")
        update_card_info()

    def choose_folder():
        folder = filedialog.askdirectory(title="SD-Karte oder Ordner wählen")
        if folder:
            refresh_drives(os.path.normpath(folder))

    ttk.Button(main, text="↻", width=3, command=lambda: refresh_drives()).grid(row=2, column=2)
    ttk.Button(main, text="Ordner wählen …", command=choose_folder).grid(row=2, column=3, padx=(6, 0))
    ttk.Label(main, textvariable=card_info_var, foreground="#555").grid(row=3, column=1, columnspan=3, sticky="w", pady=(2, 8))
    drive_box.bind("<<ComboboxSelected>>", update_card_info)

    # --- options -----------------------------------------------------------------------------
    options_frame = ttk.LabelFrame(main, text="Optionen", padding=8)
    options_frame.grid(row=4, column=0, columnspan=4, sticky="ew")
    options_frame.columnconfigure(1, weight=1)

    ttk.Label(options_frame, text="Bildart:").grid(row=0, column=0, sticky="w")
    type_frame = ttk.Frame(options_frame)
    type_frame.grid(row=0, column=1, columnspan=2, sticky="w")
    for value, label in ART_TYPE_CHOICES:
        ttk.Radiobutton(type_frame, text=label, value=value, variable=type_var).pack(side="left", padx=(6, 6))

    ttk.Label(options_frame, text="Region:").grid(row=1, column=0, sticky="w", pady=(4, 0))
    ttk.Combobox(options_frame, textvariable=region_var, values=[label for _, label in REGIONS], state="readonly",
                 width=14).grid(row=1, column=1, sticky="w", padx=(6, 0), pady=(4, 0))

    ttk.Checkbutton(options_frame, text="Vorhandene überschreiben (vom Tool erzeugte Bilder neu laden)",
                    variable=force_var).grid(row=2, column=0, columnspan=3, sticky="w", pady=(6, 0))
    ttk.Checkbutton(options_frame, text="Bilder für Ordner erzeugen (ohne eingebaute Konsolen-Bilder)", variable=console_var).grid(
        row=3, column=0, columnspan=3, sticky="w")
    ttk.Checkbutton(options_frame, text="Spiel-Icon als Ersatzbild, wenn nichts gefunden wird", variable=icon_var).grid(
        row=4, column=0, columnspan=3, sticky="w")
    ttk.Checkbutton(options_frame, text="Kachel-Bilder erzeugen (64 × 64 für das Raster 3 × 2)",
                    variable=tiles_var).grid(row=5, column=0, columnspan=3, sticky="w")
    ttk.Checkbutton(options_frame, text="Cover erzeugen (für die CoverFlow-Ansicht)",
                    variable=covers_var).grid(row=6, column=0, columnspan=3, sticky="w")
    ttk.Checkbutton(options_frame, text="Kleine Icons erzeugen (32 × 32 für das Raster 4 × 3, GBA & Co.)",
                    variable=icons_var).grid(row=7, column=0, columnspan=3, sticky="w")
    ttk.Checkbutton(options_frame, text="Auch von Hand abgelegte Bilder ersetzen", variable=replace_var).grid(
        row=8, column=0, columnspan=3, sticky="w")
    ttk.Checkbutton(options_frame, text="Bilder gelöschter Spiele entfernen", variable=clean_var).grid(
        row=9, column=0, columnspan=3, sticky="w")

    ttk.Label(options_frame, text="Eigene Bilder:").grid(row=10, column=0, sticky="w", pady=(6, 0))
    ttk.Entry(options_frame, textvariable=override_var).grid(row=10, column=1, sticky="ew", padx=(6, 6), pady=(6, 0))

    def choose_override():
        folder = filedialog.askdirectory(title="Ordner mit eigenen Bildern")
        if folder:
            override_var.set(os.path.normpath(folder))

    ttk.Button(options_frame, text="Wählen …", command=choose_override).grid(row=10, column=2, pady=(6, 0))
    ttk.Label(options_frame, text="leer = " + OVERRIDE_FOLDER.replace("/", os.sep).lstrip(os.sep) + " auf der Karte",
              foreground="#555").grid(row=11, column=1, columnspan=2, sticky="w", padx=(6, 0))

    # --- actions -----------------------------------------------------------------------------
    actions = ttk.Frame(main)
    actions.grid(row=5, column=0, columnspan=4, sticky="ew", pady=(10, 4))
    start_button = ttk.Button(actions, text="Bilder aktualisieren")
    start_button.pack(side="left")
    cancel_button = ttk.Button(actions, text="Abbrechen", state="disabled")
    cancel_button.pack(side="left", padx=(6, 0))
    check_button = ttk.Button(actions, text="Prüfen")
    check_button.pack(side="left", padx=(6, 0))
    open_button = ttk.Button(actions, text="Artwork-Ordner öffnen")
    open_button.pack(side="right")

    progress = ttk.Progressbar(main, mode="determinate")
    progress.grid(row=6, column=0, columnspan=4, sticky="ew")
    ttk.Label(main, textvariable=status_var).grid(row=7, column=0, columnspan=4, sticky="w", pady=(2, 4))

    log_box = ScrolledText(main, height=12, wrap="word", state="disabled", font=("Consolas", 9))
    log_box.grid(row=8, column=0, columnspan=4, sticky="nsew")
    log_box.tag_configure("warn", foreground="#a15c00")
    log_box.tag_configure("error", foreground="#c00000")
    log_box.tag_configure("detail", foreground="#777777")
    main.rowconfigure(8, weight=1)
    ttk.Checkbutton(main, text="Details anzeigen", variable=details_var).grid(row=9, column=0, columnspan=2, sticky="w")

    def append_log(message: str, level: str) -> None:
        if level == "detail" and not details_var.get():
            return
        log_box.configure(state="normal")
        log_box.insert("end", message + "\n", level)
        log_box.see("end")
        log_box.configure(state="disabled")

    def set_running(running: bool) -> None:
        start_button.configure(state="disabled" if running else "normal")
        check_button.configure(state="disabled" if running else "normal")
        cancel_button.configure(state="normal" if running else "disabled")

    def poll() -> None:
        try:
            while True:
                kind, *payload = messages.get_nowait()
                if kind == "log":
                    append_log(*payload)
                elif kind == "progress":
                    done, total, name = payload
                    progress["maximum"] = max(total, 1)
                    progress["value"] = done
                    status_var.set(f"{done} von {total}" + (f": {name}" if name else ""))
                elif kind == "done":
                    set_running(False)
                    status_var.set(payload[0])
                    state["thread"] = None
        except queue.Empty:
            pass
        root.after(100, poll)

    def current_options() -> Options:
        region = next((key for key, label in REGIONS if label == region_var.get()), "de")
        override = override_var.get().strip()
        return Options(art_type=type_var.get(), force=force_var.get(), console_art=console_var.get(),
                       icon_fallback=icon_var.get(), region=region, clean=clean_var.get(),
                       override_dir=Path(override) if override else None, tiles=tiles_var.get(),
                       replace_manual=replace_var.get(), covers=covers_var.get(), icons=icons_var.get())

    def remember(sd_root: Path, options: Options) -> None:
        settings.update({"sd": str(sd_root), "type": options.art_type, "region": options.region,
                         "console_art": options.console_art, "icon_fallback": options.icon_fallback,
                         "tiles": options.tiles, "covers": options.covers, "icons": options.icons,
                         "override": override_var.get().strip()})
        _save_settings(settings)

    def start() -> None:
        sd_root = selected_root()
        if not sd_root or not sd_root.is_dir():
            messagebox.showwarning("DsRunner Art", "Bitte zuerst die SD-Karte auswählen.")
            return
        if not is_dsrunner_card(sd_root) and not messagebox.askyesno(
                "DsRunner Art", f"Auf {sd_root} ist kein DsRunner (_pico fehlt). Trotzdem fortfahren?"):
            return
        options = current_options()
        remember(sd_root, options)
        cancel = threading.Event()
        state["cancel"] = cancel
        log_box.configure(state="normal")
        log_box.delete("1.0", "end")
        log_box.configure(state="disabled")
        set_running(True)
        status_var.set("Karte wird gelesen …")

        def work():
            try:
                updater = Updater(sd_root, options,
                                  log=lambda message, level="info": messages.put(("log", message, level)),
                                  progress=lambda done, total, name: messages.put(("progress", done, total, name)),
                                  cancel=cancel)
                stats = updater.run()
                summary = "Abgebrochen." if cancel.is_set() else f"Fertig: {stats.short_summary()}."
            except Exception as error:  # noqa: BLE001 - shown to the user instead of a silent crash
                messages.put(("log", f"Fehler: {type(error).__name__}: {error}", "error"))
                summary = "Fehler, siehe Protokoll."
            messages.put(("done", summary))

        thread = threading.Thread(target=work, daemon=True)
        state["thread"] = thread
        thread.start()

    def cancel_run() -> None:
        if state["cancel"]:
            state["cancel"].set()
            status_var.set("Wird abgebrochen …")

    def check() -> None:
        sd_root = selected_root()
        if not sd_root or not sd_root.is_dir():
            messagebox.showwarning("DsRunner Art", "Bitte zuerst die SD-Karte auswählen.")
            return
        log_box.configure(state="normal")
        log_box.delete("1.0", "end")
        log_box.configure(state="disabled")
        set_running(True)

        def work():
            bad = check_card(sd_root, lambda message, level="info": messages.put(("log", message, level)))
            messages.put(("done", "Alle Bilder in Ordnung." if not bad else f"{bad} Bilder unbrauchbar."))

        threading.Thread(target=work, daemon=True).start()

    def open_art() -> None:
        sd_root = selected_root()
        if sd_root:
            folder = to_fs_path(sd_root, ART_FOLDER)
            open_folder(folder if folder.is_dir() else sd_root)

    start_button.configure(command=start)
    cancel_button.configure(command=cancel_run)
    check_button.configure(command=check)
    open_button.configure(command=open_art)

    def on_close() -> None:
        thread = state["thread"]
        if thread is not None:
            if not messagebox.askyesno("DsRunner Art", "Die Aktualisierung läuft noch. Abbrechen und beenden?"):
                return
            cancel_run()

            # wait for the run to stop: it saves the list of written files on the way out (without it
            # the files of this run would count as hand-placed next time)
            def close_when_done(waited: int = 0) -> None:
                if thread.is_alive() and waited < 300:
                    root.after(100, close_when_done, waited + 1)
                else:
                    root.destroy()

            close_when_done()
            return
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    refresh_drives(os.path.normpath(str(normalize_sd_root(initial_sd))) if initial_sd else None)
    root.after(100, poll)
    root.mainloop()
    return 0
