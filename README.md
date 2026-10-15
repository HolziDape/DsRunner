<a id="top"></a>

<h1 align="center">DsRunner</h1>

<p align="center">
  <b>Ein moderner Launcher für den Nintendo DS</b><br>
  Konsolen-Kacheln, große Bilder, Suche, Favoriten, Farb-Themes und „zurück aus dem Spiel“, für die M3 DS Real und viele andere Flashkarten.
</p>

<p align="center">
  <a href="https://github.com/HolziDape/DsRunner/releases/latest"><img alt="Download" src="https://img.shields.io/badge/Download-neueste%20Version-2ea44f?style=for-the-badge"></a>
  <img alt="Nintendo DS" src="https://img.shields.io/badge/Nintendo%20DS-DS%20%C2%B7%20DS%20Lite%20%C2%B7%20DSi%20%C2%B7%203DS-5b6ee1?style=for-the-badge">
  <a href="LICENSE.txt"><img alt="Lizenz Zlib" src="https://img.shields.io/badge/Lizenz-Zlib-lightgrey?style=for-the-badge"></a>
</p>

<p align="center">
  <b>Deutsch</b> · <a href="README.en.md">English</a>
  <br><br>
  <a href="#download">Download</a> ·
  <a href="#installation-m3-ds-real">Installation</a> ·
  <a href="#bedienung">Bedienung</a> ·
  <a href="#kompatible-flashkarten">Flashkarten</a> ·
  <a href="#probleme">Probleme?</a> ·
  <a href="CHANGELOG.md">Changelog</a>
</p>

<p align="center">
  <img src="docs/images/hero.png" alt="DsRunner: Konsolen-Ebene, ein GBA-Ordner mit Spielstatistik und Suchergebnisse" width="100%">
  <br><sub>Screenshots aus dem Emulator, mit Homebrew-Spielen.</sub>
</p>

## Inhalt

[Funktionen](#funktionen) · [Download](#download) · [Installation](#installation-m3-ds-real) · [Emulatoren](#emulatoren-für-andere-konsolen) · [Bedienung](#bedienung) · [Zurück aus dem Spiel](#zurück-zu-dsrunner-aus-einem-spiel) · [Suche](#suche) · [Menü und Sprache](#menü-und-sprache) · [Farb-Themes und Musik](#farb-themes-und-musik) · [Bilder](#bilder-für-die-spiele) · [Einstellungen](#einstellungen) · [DsRunner Stream](#dsrunner-stream) · [Update](#update-auf-eine-neue-version) · [Flashkarten](#kompatible-flashkarten) · [Probleme?](#probleme) · [Entwickler](#für-entwickler) · [Danke](#danke-und-lizenz)

## Funktionen

| | |
|---|---|
| **Startet automatisch** | beim Einschalten, ohne Umweg über ein anderes Menü (fertig für die M3 DS Real, für viele andere Karten [von Hand](docs/FLASHCARDS.md#autostart)) |
| **Konsolen und Spiele** | erst NDS, GBA, NES & Co. als Kacheln mit gezeichneter Konsole, dann die Spiele der Konsole |
| **Drei Ansichten** | große Kacheln (3×2), kleine Kacheln (4×3) und Coverflow |
| **Andere Konsolen** | GBA, NES, Game Boy / Color und SNES laufen sofort, die Emulatoren sind im Paket |
| **Suche** | über alle Spiele mit Bildschirmtastatur, Treffer nach Konsole geordnet |
| **Favoriten und Verlauf** | Favoriten mit X, Sortierung „zuletzt gespielt“, **Weiterspielen** mit START |
| **Spielstatistik und Namen** | „12× gespielt · zuletzt gestern“; „Golden Sun“ statt „Golden Sun (Europe) (En,Fr,De).gba“ |
| **Zurück aus dem Spiel** | Tastenkombination im Spiel startet DsRunner neu, ohne den DS auszuschalten |
| **Fünf Farb-Themes und Musik** | Dark, Hell, OLED, Retro, Blau oder eigene Farben; Hintergrundmusik optional |
| **Deutsch und Englisch** | im Menü umschaltbar, wirkt sofort |
| **DsRunner Art** | PC-Programm, lädt Bilder und Cover für alle Spiele automatisch |
| **DsRunner Stream** | Browser-Spiele vom Raspberry Pi per WLAN auf dem DS spielen (experimentell) |

Dazu Cheats über `usrcheat.dat`, Uhr, Akkuanzeige und Touchscreen-Bedienung.

<img src="docs/images/views.png" alt="Konsolen-Ebene im 3×2-Raster, im 4×3-Raster und im Coverflow" width="100%">

## Download

**[Neueste Version herunterladen](https://github.com/HolziDape/DsRunner/releases/latest)**

| Datei | Wofür |
|---|---|
| **`DsRunner_M3DSReal.zip`** | **M3 DS Real:** komplettes Paket für die SD-Karte, mit Autostart, Emulatoren und DsRunner Stream |
| `DsRunner.zip` | andere Flashkarten, siehe [Kompatible Flashkarten](#kompatible-flashkarten) |
| `PicoLoader-DsRunner_<Plattform>.zip` | andere Flashkarten: Pico Loader mit „zurück aus dem Spiel“, eine Datei pro Kartentyp |
| `DsRunnerArt.exe` | PC-Programm für Windows, lädt die Bilder für deine Spiele auf die Karte |
| `DsRunnerStream-Host.zip` | Raspberry-Pi-Teil von [DsRunner Stream](#dsrunner-stream) (ab Version 0.4.0) |

Was neu ist, steht im **[Changelog](CHANGELOG.md)**. Die installierte Version steht in DsRunner im Menü (Zahnrad oben rechts).

## Installation (M3 DS Real)

1. **SD-Karte sichern**, mindestens alle Spielstände (`.sav`), den Ordner `SYSTEM` und `SRESET.DAT`.
2. `DsRunner_M3DSReal.zip` entpacken und den **Inhalt** ins Hauptverzeichnis der SD-Karte kopieren, vorhandene Dateien ersetzen.
3. Deine Spiele in einen Ordner pro Konsole unter `/Games` legen (siehe unten).
4. Karte in den DS stecken und einschalten. DsRunner startet von selbst.

**Zurück zum alten Menü** (z. B. YSMenu): die gesicherten Dateien `SYSTEM/g6dsload.1`, `SYSTEM/g6dsload.eng` und `SRESET.DAT` zurückkopieren, dann `boot.*`, `loader.eng` und den Ordner `_system_` löschen.

### Ordner für die Spiele

```
SD-Karte
├── Games
│   ├── NDS     ← DS-Spiele (.nds)
│   ├── GBA     ← Game-Boy-Advance-Spiele (.gba)
│   ├── NES, GB, SNES, …   ← weitere Konsolen, siehe Emulatoren
│   └── Apps    ← Homebrew-Programme (hier liegt auch DsRunnerStream.nds)
├── _dsrunner   ← Emulatoren, Bilder, Favoriten, Statistik
└── _pico       ← Einstellungen, Themes, Pico Loader
```

Jeder Unterordner von `/Games` ist eine Konsole. `NDS`, `GBA`, `GB`, `GBC`, `NES`, `SNES`, `SMS`, `GG`, `MD`, `PCE` und `Apps` bekommen eine passende Konsole und Farbe, andere Ordner eine normale Kachel. Ohne `/Games` zeigt DsRunner das Hauptverzeichnis; ein anderer Ordner geht mit `libraryPath` in den [Einstellungen](#einstellungen).

## Emulatoren für andere Konsolen

DS-Spiele startet DsRunner direkt, andere Konsolen brauchen einen Emulator in **`/_dsrunner/emulators/`**. Fehlt er, zeigt der Ordner „Keine Spiele“.

| Konsole | Ordner | Dateien | Emulator |
|---|---|---|---|
| Game Boy Advance | `GBA` | `.gba` | GBARunner2: **im Paket** |
| NES | `NES` | `.nes`, `.fds` | nesDS: **im Paket** |
| Game Boy / Color | `GB`, `GBC` | `.gb`, `.gbc` | GameYob: **im Paket** |
| Super Nintendo | `SNES` | `.sfc`, `.smc` | SNEmulDS: **im Paket** |
| Master System / Game Gear | `SMS`, `GG` | `.sms`, `.gg` | [S8DS](https://github.com/FluBBaOfWard/S8DS/releases) herunterladen, `S8DS.nds` |
| Mega Drive | `MD` | `.gen`, `.md` | `jEnesisDS.nds` aus dem [Virtual-Console-Add-on von TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases) |
| PC Engine | `PCE` | `.pce` | [NitroGrafx](https://github.com/FluBBaOfWard/NitroGrafx/releases) herunterladen, `NitroGrafx.nds` |

- Die Dateinamen müssen wie in der Tabelle heißen. Emulatoren von TWiLight Menu++ (`/_nds/TWiLightMenu/emulators/`) findet DsRunner auch.
- **GBA-BIOS:** optional und nicht im Paket. Wer es hat, legt es als `/_gba/bios.bin` ab; dann laufen mehr Spiele.
- Lädt ein Mega-Drive-Spiel mit `.md` nicht, in `.gen` umbenennen. Lizenzen: [licenses/emulators](licenses/emulators/LIZENZEN.txt), Details: [docs/CONFIG.md](docs/CONFIG.md#emulatoren).

## Bedienung

| Taste | Funktion |
|---|---|
| Steuerkreuz | Auswahl bewegen |
| A | Konsole öffnen / Spiel starten |
| B | zurück zu den Konsolen |
| L / R | Seite weiterblättern; in den Suchergebnissen nur eine Konsole / alle; im Menü die Seite wechseln |
| X | Favorit hinzufügen / entfernen |
| Y | Cheats für das ausgewählte Spiel |
| START | auf der Konsolen-Ebene: das zuletzt gespielte Spiel weiterspielen |
| SELECT | Suche öffnen |
| Zahnrad (oben rechts) | Menü; der Pinsel darin öffnet die Theme-Auswahl |

Der Touchscreen funktioniert überall. Oben steht zu jedem Spiel der Name ohne Dateiendung und Zusätze wie „(Europe)“, darunter die **Spielstatistik** (Starts und letzter Start, gezählt in `/_dsrunner/stats.txt`, Datum von der Uhr des DS).

### Zurück zu DsRunner aus einem Spiel

Im laufenden DS-Spiel **L + R + Unten + B zwei Sekunden halten**. Der Bildschirm wird kurz hell, dann startet DsRunner neu.

- **Vorher speichern.** Nicht Gespeichertes geht verloren, ein laufender Speichervorgang wird aber nicht abgebrochen.
- **Nicht jedes Spiel unterstützt das.** Dann läuft es normal weiter, nur ohne diese Funktion.
- **Andere Tasten**, z. B. bei ausgeleierten L/R-Tasten: im Menü SELECT+START+↓ oder SELECT+↓+B wählen, eigene Kombinationen mit `inGameResetKeys`.
- Geht nur mit dem Pico Loader aus unseren Paketen, nicht mit dem offiziellen. Details: [loader/README.md](loader/README.md).

## Suche

SELECT oder die Kachel **Suche** öffnet eine Bildschirmtastatur (Touchscreen oder Steuerkreuz + A; B löscht, X Leerzeichen, START sucht). Schon beim Tippen steht da, wie viele Spiele pro Konsole passen. Die Treffer erscheinen wie ein Ordner, nach Konsole geordnet. Gesucht wird im Dateinamen, ohne Groß-/Kleinschreibung, Akzente und Satzzeichen, in allen Konsolen mit Emulator.

<img src="docs/images/search.png" alt="Such-Tastatur und Suchergebnisse" width="66%">

## Menü und Sprache

Das **Zahnrad** öffnet das Menü mit zwei Seiten (L / R wechselt). Änderungen werden beim Schließen gespeichert.

- **Anzeige:** Ansicht (3×2, 4×3, Coverflow), Sortieren (A–Z, Z–A, zuletzt gespielt), Uhr, installierte Version
- **Spiel & Sprache:** zurück aus dem Spiel ein/aus, die Tasten dafür, **Deutsch / English** (wechselt sofort; auf Englisch ist die Tastatur QWERTY)

<img src="docs/images/menu.png" alt="Menü Seite 1, Menü Seite 2, englische Oberfläche" width="100%">

## Farb-Themes und Musik

**Themes:** Zahnrad → Pinsel → DsRunner Dark (Standard), Hell, OLED, Retro oder Blau wählen. Für ein eigenes Theme einen Ordner in `/_pico/themes` kopieren und in der `theme.json` die Farben unter `"colors"` ändern ([alle Schlüssel](docs/CONFIG.md#farb-themes)).

**Musik:** standardmäßig aus. Eine `.bcstm`-Datei in `/_pico/themes/dsrunner/bgm/` legen oder zum Ausprobieren den Beispiel-Ordner `musik-beispiel` in `bgm` umbenennen. MP3/WAV wandelt `tools/bgm/wav2bcstm.py` um. Abschalten mit `"music": false`, leiser mit `"musicVolume"` (0–100). Anleitung: [docs/CONFIG.md](docs/CONFIG.md#hintergrundmusik).

<img src="docs/images/themes.png" alt="Die fünf Farb-Themes: Dark, Hell, OLED, Retro, Blau" width="100%">

## Bilder für die Spiele

SD-Karte in den PC stecken, **`DsRunnerArt.exe`** starten, Laufwerk wählen und **„Bilder aktualisieren“** klicken. Das Programm erkennt deine Spiele und lädt Titelbilder und Cover aus [libretro-thumbnails](https://github.com/libretro-thumbnails); mit Python läuft es auch unter Linux und macOS ([docs/ART-TOOL.md](docs/ART-TOOL.md)).

**Eigene Bilder:** ein BMP mit 256 × 192 Pixeln und 256 Farben unter `/_dsrunner/art` im selben Pfad wie das Spiel, z. B. `/_dsrunner/art/Games/NDS/Mein Spiel.bmp`. Eigene Bilder haben Vorrang und werden nicht überschrieben.

## Einstellungen

Die meisten Einstellungen gibt es im [Menü](#menü-und-sprache), alle in `/_pico/settings.json` (mit jedem Texteditor bearbeitbar, auf die Kommas achten):

| Einstellung | Bedeutung |
|---|---|
| `"libraryPath": "/Games"` | Ordner mit den Konsolen-Ordnern |
| `"uiLanguage": "en"` | Oberfläche auf Englisch (`"de"` = Deutsch, Standard) |
| `"cleanNames": false` | Spielnamen mit Region und Zusätzen, nur ohne Dateiendung |
| `"inGameReset": false` | zurück aus dem Spiel abschalten |
| `"inGameResetKeys": "SELECT+START+DOWN"` | Tasten dafür: mindestens 3 aus A, B, SELECT, START, UP, DOWN, LEFT, RIGHT, L, R |
| `"music": false` / `"musicVolume": 50` | Hintergrundmusik aus / leiser |

Alle Einstellungen: **[docs/CONFIG.md](docs/CONFIG.md)**.

## DsRunner Stream

> **Experimentell.** Ein Raspberry Pi 5 (oder PC) öffnet ein Browser-Spiel und schickt das Bild per WLAN an den DS; Tasten und Touchscreen des DS steuern das Spiel.

Auf dem DS `Games/Apps/DsRunnerStream.nds` starten. Auf dem Pi `DsRunnerStream-Host.zip` entpacken und `tools/install-pi.sh` ausführen: Der Installer richtet Chromium, die Python-Pakete, ein eigenes offenes WLAN für den DS und das Start-Skript `~/dsstream.sh` ein. Auf DS Lite + Pi 5 getestet: etwa **18 bis 25 Bilder pro Sekunde** in 256 Farben, kein Ton auf dem DS. Anleitung: **[stream/README.md](stream/README.md)**.

## Update auf eine neue Version

1. Aus dem neuen `DsRunner_M3DSReal.zip` die Datei `_nds/TWiLightMenu/main.srldr` und den Ordner `_pico/themes` auf die Karte kopieren.
2. Steht im [Changelog](CHANGELOG.md) **„Pico Loader geändert“**, auch die fünf `.bin`-Dateien aus `_pico` kopieren.
3. Falls noch nicht vorhanden: `_dsrunner/emulators`, `snemul.cfg` und `Games/Apps/DsRunnerStream.nds` einmal kopieren.

Spiele, Spielstände, Einstellungen, Bilder, Favoriten und Statistik bleiben erhalten.

## Kompatible Flashkarten

DsRunner selbst ist ein normales DS-Homebrew-Programm. Spiele startet es über **Pico Loader**, und den gibt es für diese Karten („Plattformen“). **Getestet ist bisher nur die M3 DS Real.**

| Karte | Plattform |
|---|---|
| **M3 DS Real** (getestet), M3i Zero (nicht GMP-Z003), iTouchDS | `M3DS` |
| M3i Zero GMP-Z003 | `G003` |
| Acekard 2, 2.1, 2i · Acekard RPG | `AK2` · `AKRPG` |
| Ace3DS+, Gateway 3DS (blau) | `ACE3DS` |
| DSTT, SuperCard DSONE SDHC | `DSTT` |
| R4iDSN · Original R4DS (zurück aus dem Spiel nur teilweise) | `R4iDSN` · `R4` |
| Stargate 3DS · DSpico · EZ-Flash Parallel · DATEL | `STARGATE` · `DSPICO` · `EZP` · `DATEL` |
| Slot 2: SuperCard, M3 CF, Max Media Dock, GBA Movie Player | `SUPERCARD` · `SUPERCARDCF` · `M3CF` · `MMCF` · `MPCF` |

**Nur eine Plattform wählen, unter der deine Karte mit Namen steht:** Die falsche kann laut Pico Loader die Karte beschädigen. Vollständige Tabelle mit allen Klonen, Autostart und DSi/3DS: **[docs/FLASHCARDS.md](docs/FLASHCARDS.md)**.

### Installation auf anderen Karten

1. Karte sichern, `DsRunner.zip` ins Hauptverzeichnis der SD-Karte entpacken.
2. `PicoLoader-DsRunner_<Plattform>.zip` für **deine** Karte ebenfalls ins Hauptverzeichnis entpacken.
3. `DsRunner.nds` aus dem Menü der Karte starten. Autostart: [docs/FLASHCARDS.md](docs/FLASHCARDS.md#autostart) (Autoboot-Dateien von TWiLight Menu++; bei der DSpico in `/_picoboot.nds` umbenennen).

## Probleme?

| Problem | Lösung |
|---|---|
| Ein Konsolen-Ordner zeigt „Keine Spiele“ | Der Emulator fehlt oder heißt anders, siehe [Emulatoren](#emulatoren-für-andere-konsolen). |
| „Failed to load rom header“ beim Spielstart | Die Emulator-Datei ist kaputt; neu herunterladen. |
| Ein Spiel ignoriert „zurück aus dem Spiel“ | In `/_pico/igr-log.txt` steht nach dem Spielstart, warum ([loader/README.md](loader/README.md)). |
| Falsches Datum in der Spielstatistik | Die Uhr in den DS-Systemeinstellungen stellen. |
| DsRunner Stream verbindet nicht | [Fehlersuche im Stream-README](stream/README.md) |

Fehler und Wünsche: [Issues](https://github.com/HolziDape/DsRunner/issues). Spiele, die schlechter laufen als mit anderen Menüs: [Kompatibilitätsliste](docs/compat/pico-vs-ysmenu.md).

## Für Entwickler

```sh
git clone --recursive https://github.com/HolziDape/DsRunner.git
cd DsRunner
./build.sh            # → DsRunner.nds (Docker mit BlocksDS)
./build.sh package    # → SD-Paket für die M3 DS Real in sd-package/
```

Mehr: [Entwicklung](docs/DEVELOPMENT.md) · [Einstellungen](docs/CONFIG.md) · [Themes](docs/Themes.md) · [Pico Loader](loader/README.md) · [Releases](docs/RELEASE.md) · [DsRunner Art](docs/ART-TOOL.md) · [Stream-Protokoll](stream/PROTOCOL.md) · [Pläne](docs/PLAN.md)

## Danke und Lizenz

DsRunner ist ein Fork von **[Pico Launcher](https://github.com/LNH-team/pico-launcher)** und nutzt **[Pico Loader](https://github.com/LNH-team/pico-loader)**, beide vom [LNH team](https://github.com/LNH-team) ([@Gericom](https://github.com/Gericom), [@XLuma](https://github.com/XLuma), [@Dartz150](https://github.com/Dartz150), [@lifehackerhansol](https://github.com/lifehackerhansol)). „Zurück aus dem Spiel“ und der DLDI-Cache im Pico Loader stammen von [@danyocom](https://github.com/danyocom/pico-loader), Autostart-Dateien und einige Emulatoren aus [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu), Icons von [icons8](https://icons8.com/).

Lizenz: Zlib (`LICENSE.txt`), weitere im Ordner `licenses`.

<p align="right"><sub><a href="#top">↑ nach oben</a></sub></p>
