# DsRunner

Ein moderner Launcher für den **Nintendo DS**. DsRunner startet beim Einschalten automatisch und zeigt deine Spiele in einem dunklen Kachel-Design: erst die Konsolen, dann ihre Spiele, mit großen Bildern auf dem oberen Bildschirm. Gestartet werden die Spiele über [Pico Loader](https://github.com/LNH-team/pico-loader).

Getestet ist DsRunner auf dem DS Lite mit der Flashkarte **M3 DS Real**. Laufen sollte er auf allen Flashkarten, die Pico Loader unterstützt, z. B. Acekard 2, Ace3DS+, DSTT, R4 und DSpico: siehe [Kompatible Flashkarten](#kompatible-flashkarten).

![DsRunner: Konsolen-Ebene im 3×2-Raster, im 4×3-Raster und im Coverflow](docs/images/DsRunner.png)

<sub>Screenshots aus dem Emulator.</sub>

## Funktionen

- **Startet automatisch** beim Einschalten, ohne Umweg über ein anderes Menü (fertig eingerichtet für die M3 DS Real, für viele andere Karten [von Hand](docs/FLASHCARDS.md#autostart))
- **Konsolen und Spiele:** erst NDS, GBA, NES & Co. als Kacheln mit eigenem Bild, dann die Spiele der Konsole
- **Drei Ansichten:** große Kacheln (3×2), kleine Kacheln (4×3) und Coverflow
- **Bilder für jedes Spiel** auf dem oberen Bildschirm und in den Kacheln, automatisch geladen mit dem PC-Programm [DsRunner Art](#bilder-für-die-spiele)
- **Andere Konsolen:** GBA, NES, Game Boy und SNES laufen sofort, die Emulatoren sind im Paket. Master System, Game Gear, Mega Drive und PC Engine mit einem zusätzlichen Download
- **Favoriten** (Taste X), **zuletzt gespielt** als Sortierung und **Weiterspielen** mit START
- **Zurück zu DsRunner aus dem Spiel**, ohne den DS auszuschalten
- Cheats über `usrcheat.dat`, Uhr und Akkuanzeige, Oberfläche auf Deutsch
- **Experimentell:** Browser-Spiele über einen Raspberry Pi auf dem DS spielen ([DsRunner Stream](#dsrunner-stream-experimentell))

## Download

**[⬇ Neueste Version herunterladen](https://github.com/HolziDape/DsRunner/releases/latest)**

| Datei | Wofür |
|---|---|
| **`DsRunner_M3DSReal.zip`** | **M3 DS Real:** komplettes Paket für die SD-Karte, mit Autostart und Emulatoren |
| `DsRunner.zip` | andere Flashkarten, siehe [unten](#kompatible-flashkarten) |
| `PicoLoader-DsRunner_<Plattform>.zip` | andere Flashkarten: Pico Loader mit „zurück zu DsRunner aus dem Spiel“, eine Datei pro Kartentyp |
| `DsRunnerArt.exe` | PC-Programm für Windows, lädt die Bilder für deine Spiele auf die Karte |

Was in welcher Version neu ist, steht im **[Changelog](CHANGELOG.md)**. Die installierte Version siehst du in DsRunner unten rechts im Menü „Anzeige“ (Zahnrad oben rechts).

## Installation (M3 DS Real)

1. **SD-Karte sichern.** Mindestens alle Spielstände (`.sav`), den Ordner `SYSTEM` und die Datei `SRESET.DAT`. Damit kommst du jederzeit zu deinem alten Menü (z. B. YSMenu) zurück.
2. `DsRunner_M3DSReal.zip` entpacken.
3. Den **Inhalt** des entpackten Ordners ins Hauptverzeichnis der SD-Karte kopieren und vorhandene Dateien ersetzen.
4. Deine Spiele in einen Ordner pro Konsole unter `/Games` legen (siehe unten).
5. Karte in den DS stecken und einschalten. DsRunner startet von selbst.

### Ordner für die Spiele

Jeder Unterordner von `/Games` erscheint als eine Konsole:

```
SD-Karte
├── Games
│   ├── NDS     ← DS-Spiele (.nds)
│   ├── GBA     ← Game-Boy-Advance-Spiele (.gba)
│   ├── NES     ← weitere Konsolen, siehe Tabelle unten
│   ├── GB
│   ├── SNES
│   └── Apps    ← Homebrew-Programme
├── _dsrunner   ← Emulatoren, Bilder, Favoriten (legt das Paket bzw. DsRunner an)
├── _nds
└── _pico
```

- Die Ordnernamen `NDS`, `GBA`, `GB`, `GBC`, `NES`, `SNES`, `SMS`, `GG`, `MD`, `PCE` und `Apps` erkennt DsRunner und zeigt sie mit passendem Bild und eigener Farbe. Andere Ordner erscheinen als normale Kachel.
- Ohne Ordner `/Games` zeigt DsRunner das Hauptverzeichnis der Karte. Einen anderen Ordner kannst du in den [Einstellungen](#einstellungen) wählen.

## Andere Konsolen (Emulatoren)

DS-Spiele startet DsRunner direkt. Für andere Konsolen braucht es einen Emulator, ein kleines DS-Programm. DsRunner findet ihn von selbst, wenn er in **`/_dsrunner/emulators/`** liegt. Fehlt der Emulator, bleibt der Ordner der Konsole leer („Keine Spiele“).

| Konsole | Ordner | Dateien | Emulator |
|---|---|---|---|
| Game Boy Advance | `GBA` | `.gba` | GBARunner2: **im Paket** |
| NES | `NES` | `.nes`, `.fds` | nesDS: **im Paket** |
| Game Boy / Color | `GB`, `GBC` | `.gb`, `.gbc` | GameYob: **im Paket** |
| Super Nintendo | `SNES` | `.sfc`, `.smc` | SNEmulDS: **im Paket** |
| Master System / Game Gear | `SMS`, `GG` | `.sms`, `.gg` | [S8DS](https://github.com/FluBBaOfWard/S8DS/releases) herunterladen, `S8DS.nds` aus der ZIP-Datei |
| Mega Drive | `MD` | `.gen`, `.md` | `jEnesisDS.nds` aus dem [Virtual-Console-Add-on von TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases) |
| PC Engine | `PCE` | `.pce` | [NitroGrafx](https://github.com/FluBBaOfWard/NitroGrafx/releases) herunterladen, `NitroGrafx.nds` aus der ZIP-Datei |

- Heruntergeladene Emulatoren nach `/_dsrunner/emulators/` kopieren. Die Dateinamen müssen wie in der Tabelle heißen.
- **GBA-BIOS:** GBARunner2 läuft auch ohne, mit dem originalen BIOS laufen aber mehr Spiele. Das BIOS darf nicht mitgeliefert werden. Wer es hat, legt es als `/_gba/bios.bin` auf die Karte.
- Hast du TWiLight Menu++ installiert, findet DsRunner auch dessen Emulatoren unter `/_nds/TWiLightMenu/emulators/`.
- Lädt ein Mega-Drive-Spiel mit der Endung `.md` nicht, die Datei in `.gen` umbenennen.
- Herkunft und Lizenzen der mitgelieferten Emulatoren: [licenses/emulators](licenses/emulators/LIZENZEN.txt). Alle Details und eigene Konsolen: [docs/CONFIG.md](docs/CONFIG.md#emulatoren).

## Bedienung

| Taste | Funktion |
|---|---|
| Steuerkreuz | Auswahl bewegen |
| A | Konsole öffnen / Spiel starten |
| B | zurück zu den Konsolen |
| L / R | eine Seite weiterblättern |
| X | Spiel zu den Favoriten hinzufügen / entfernen |
| Y | Cheats für das ausgewählte Spiel |
| START | auf der Konsolen-Ebene: das zuletzt gespielte Spiel weiterspielen |
| Zahnrad (oben rechts) | Ansicht, Sortierung und Uhr einstellen |

Der Touchscreen funktioniert ebenfalls.

### Zurück zu DsRunner aus einem Spiel

Im laufenden DS-Spiel **L + R + Unten + B zwei Sekunden halten**. Der Bildschirm wird kurz hell, dann startet DsRunner neu.

- **Vorher im Spiel speichern.** Nicht Gespeichertes geht verloren, wie beim Ausschalten. Ein laufender Speichervorgang wird nicht abgebrochen.
- **Nicht jedes Spiel unterstützt das.** Dann läuft das Spiel ganz normal weiter, nur ohne diese Funktion.
- **Andere Tasten**, z. B. wenn L oder R schlecht reagieren: in den [Einstellungen](#einstellungen) `"inGameResetKeys": "SELECT+START+DOWN"` eintragen.
- Funktioniert mit dem Pico Loader aus `DsRunner_M3DSReal.zip` bzw. `PicoLoader-DsRunner_<Plattform>.zip`, nicht mit dem offiziellen Pico Loader. Technische Details und Fehlersuche: [loader/README.md](loader/README.md).

## Bilder für die Spiele

Die Bilder für den oberen Bildschirm, die Kacheln und den Coverflow lädt das PC-Programm **DsRunner Art** automatisch:

1. SD-Karte in den PC stecken.
2. `DsRunnerArt.exe` aus dem [Download](https://github.com/HolziDape/DsRunner/releases/latest) starten.
3. Laufwerk wählen und **„Bilder aktualisieren“** klicken.

Das Programm erkennt deine Spiele und lädt passende Titelbilder und Cover aus [libretro-thumbnails](https://github.com/libretro-thumbnails). Es läuft unter Windows, mit Python auch unter Linux und macOS. Anleitung und Optionen: **[docs/ART-TOOL.md](docs/ART-TOOL.md)**.

**Eigene Bilder** gehen auch: ein BMP mit 256 × 192 Pixeln und 256 Farben unter `/_dsrunner/art` ablegen, im selben Pfad wie das Spiel, z. B. `/_dsrunner/art/Games/NDS/Mario Kart DS.bmp` für `/Games/NDS/Mario Kart DS.nds`. Dafür reicht Paint („Speichern unter“ → 256-Farben-Bitmap). Eigene Bilder haben immer Vorrang und werden von DsRunner Art nicht überschrieben.

## Einstellungen

Die meisten Einstellungen (Ansicht, Sortierung, Uhr) gibt es im Menü unter dem Zahnrad. Weitere stehen in der Datei `/_pico/settings.json` auf der SD-Karte. Sie lässt sich mit jedem Texteditor bearbeiten:

| Einstellung | Bedeutung |
|---|---|
| `"libraryPath": "/Games"` | Ordner mit den Konsolen-Ordnern |
| `"inGameReset": false` | „Zurück zu DsRunner aus dem Spiel“ abschalten |
| `"inGameResetKeys": "SELECT+START+DOWN"` | andere Tasten dafür: mindestens 3 aus A, B, SELECT, START, UP, DOWN, LEFT, RIGHT, L, R (nicht X/Y) |

Beim Bearbeiten auf die Kommas zwischen den Zeilen achten. Alle Einstellungen: **[docs/CONFIG.md](docs/CONFIG.md)**.

## DsRunner Stream (experimentell)

Mit **DsRunner Stream** spielst du Browser-Spiele auf dem DS. Ein Raspberry Pi (oder PC) öffnet das Spiel und schickt das Bild per WLAN an den DS, die Tasten und der Touchscreen des DS steuern das Spiel.

- Auf dem DS läuft dafür die App `DsRunnerStream.nds` aus `Games/Apps`.
- Der Pi stellt ein eigenes offenes WLAN nur für den DS bereit und hängt selbst per Kabel am Router.
- Etwa 10 bis 20 Bilder pro Sekunde in 256 Farben, kein Ton auf dem DS. Für ruhigere Spiele gut, für schnelle Action eher nicht.

Noch nicht auf echter Hardware getestet. Einrichtung, Tastenbelegung und Sicherheitshinweise: **[stream/README.md](stream/README.md)**.

## Update auf eine neue Version

1. Aus dem neuen `DsRunner_M3DSReal.zip` die Datei `_nds/TWiLightMenu/main.srldr` auf die Karte kopieren und die alte ersetzen.
2. Steht im [Changelog](CHANGELOG.md) bei der Version **„Pico Loader geändert“**, zusätzlich die fünf `.bin`-Dateien aus `_pico` kopieren.
3. Fehlen dir noch die Emulatoren, einmal den Ordner `_dsrunner/emulators` und die Datei `snemul.cfg` kopieren.

Spiele, Spielstände, Einstellungen, Bilder und Favoriten bleiben dabei erhalten. Andere Flashkarten: siehe [docs/FLASHCARDS.md](docs/FLASHCARDS.md#autostart).

## Kompatible Flashkarten

**DsRunner ist nicht auf die M3 DS Real beschränkt.** Getestet ist bisher allerdings nur die M3 DS Real. Was wo geht:

- **Überall gleich:** das Menü selbst (ein normales DS-Homebrew-Programm), die Emulatoren, DsRunner Art (läuft am PC) und DsRunner Stream (DS, DS Lite, DSi und 3DS im DS-Modus; WLAN nur offen oder WEP).
- **Hängt von der Karte ab:** Spiele startet DsRunner über **Pico Loader**, und den gibt es nur für bestimmte Karten („Plattformen“). Ohne passende Plattform startet DsRunner, aber kein Spiel. Auch der **Autostart** ist je Karte verschieden.
- **Zurück zu DsRunner aus dem Spiel** steckt in unserem Pico Loader und sollte auf allen Plattformen gleich funktionieren, außer `R4` (dort nur in manchen Spielen).

| Karte (bekannte Namen und Klone) | Pico-Loader-Plattform | Spiele starten | Autostart möglich | Zurück aus dem Spiel | Getestet |
|---|---|---|---|---|---|
| **M3 DS Real** | `M3DS` | ja | ja, fertiges Paket | ja | **ja** (DS Lite) |
| M3i Zero (nicht GMP-Z003), iTouchDS, r4rts.com, r4isdhc.com RTS (schwarz) | `M3DS` | ja | ja, Paket der M3 DS Real | ja | sollte funktionieren (ungetestet) |
| M3i Zero (GMP-Z003) | `G003` | ja | ja | ja | sollte funktionieren (ungetestet) |
| Acekard 2, 2.1, 2i, r4ids.cn und Ableger | `AK2` | ja | ja | ja | sollte funktionieren (ungetestet) |
| Acekard RPG | `AKRPG` | ja | nein | ja | sollte funktionieren (ungetestet) |
| Ace3DS+, Gateway 3DS (blau), r4isdhc.com.cn, r4isdhc.hk ab 2020 und Ableger | `ACE3DS` | ja | ja | ja | sollte funktionieren (ungetestet) |
| DSTT, SuperCard DSONE SDHC, r4isdhc.com ab 2014, r4i-sdhc.com und Ableger | `DSTT` | ja | ja | ja | sollte funktionieren (ungetestet) |
| r4idsn.com | `R4iDSN` | ja | ja | ja | sollte funktionieren (ungetestet) |
| Original R4DS (ohne SDHC), M3 DS Simply | `R4` | ja | ja | nur in manchen Spielen | sollte funktionieren (ungetestet) |
| Stargate 3DS | `STARGATE` | ja | ja | ja | sollte funktionieren (ungetestet) |
| DSpico | `DSPICO` | ja | ja | ja | sollte funktionieren (ungetestet) |
| EZ-Flash Parallel | `EZP` | ja | ja | ja | sollte funktionieren (ungetestet) |
| DATEL Games n' Music, Action Replay DS(i) Media Edition | `DATEL` | ja | nur Games n' Music | ja | sollte funktionieren (ungetestet) |
| Slot 2: SuperCard SD/Lite/Rumble, SuperChis · SuperCard CF · M3 CF · Max Media Dock CF · GBA Movie Player CF | `SUPERCARD` · `SUPERCARDCF` · `M3CF` · `MMCF` · `MPCF` | ja | nur GBA Movie Player | ja | sollte funktionieren (ungetestet) |

Namen und Klone wie im [Pico-Loader-README](https://github.com/LNH-team/pico-loader#supported-platforms). **Wähle nur eine Plattform, unter der deine Karte mit Namen steht:** Die falsche kann laut Pico Loader die Karte beschädigen. Nicht dabei sind z. B. SuperCard DSTWO, EZ-Flash V/Vi und iSmart Premium. Auf DSi und 3DS gilt dasselbe, solange DsRunner von der Flashkarte läuft. Alle Details, Hinweise zu Klonen, DSi/3DS und GBARunner2: **[docs/FLASHCARDS.md](docs/FLASHCARDS.md)**.

### Installation auf anderen Karten

1. SD-Karte sichern (Spielstände und die Menü-Dateien deiner Karte).
2. `DsRunner.zip` entpacken und den Inhalt ins Hauptverzeichnis der SD-Karte kopieren.
3. `PicoLoader-DsRunner_<Plattform>.zip` für deine Karte aus der Tabelle entpacken und den Inhalt ebenfalls ins Hauptverzeichnis kopieren (landet in `/_pico`). Fehlt diese Datei (bei Releases bis 0.3.1), gehen auch die Dateien `aplist.bin`, `savelist.bin`, `patchlist.bin`, `picoLoader7.bin` und `picoLoader9.bin` aus dem [offiziellen Pico Loader](https://github.com/LNH-team/pico-loader/releases), dann aber ohne „zurück aus dem Spiel“.
4. `DsRunner.nds` aus dem Menü deiner Karte starten.

**Autostart:** Für die meisten Karten liefert [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases) in `TWiLightMenu-Flashcard.7z` (Ordner `Autoboot`) Startdateien, die `/_nds/TWiLightMenu/main.srldr` starten. Die Dateien deiner Karte ins Hauptverzeichnis kopieren und `DsRunner.nds` als `/_nds/TWiLightMenu/main.srldr` ablegen. Bei der DSpico reicht es, `DsRunner.nds` in `/_picoboot.nds` umzubenennen. Ausnahmen und Schritt für Schritt: [docs/FLASHCARDS.md](docs/FLASHCARDS.md#autostart).

## Zurück zum alten Menü (M3 DS Real)

Die gesicherten Dateien `SYSTEM/g6dsload.1`, `SYSTEM/g6dsload.eng` und `SRESET.DAT` zurückkopieren, dann `boot.*`, `loader.eng` und den Ordner `_system_` löschen.

## Probleme?

- **Ein Konsolen-Ordner zeigt „Keine Spiele“:** Der Emulator fehlt oder heißt anders, siehe [Andere Konsolen](#andere-konsolen-emulatoren).
- **Ein Spiel reagiert nicht auf „Zurück zu DsRunner“:** In `/_pico/igr-log.txt` steht nach dem Spielstart, warum. Siehe [loader/README.md](loader/README.md).
- **Ein Spiel läuft schlechter als mit einem anderen Menü:** gerne melden, siehe [Kompatibilitätsliste](docs/compat/pico-vs-ysmenu.md).
- Fehler und Wünsche als [Issue](https://github.com/HolziDape/DsRunner/issues).

## Für Entwickler

DsRunner wird mit [BlocksDS](https://blocksds.skylyrac.net/) in Docker gebaut (unter Windows in WSL2):

```sh
git clone --recursive https://github.com/HolziDape/DsRunner.git
cd DsRunner
./build.sh            # → DsRunner.nds
./build.sh package    # → komplettes SD-Paket für die M3 DS Real in sd-package/
```

Details, Test im Emulator, Pico Loader und Releases: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md), [loader/README.md](loader/README.md), [docs/RELEASE.md](docs/RELEASE.md). Stand und Pläne: [docs/PLAN.md](docs/PLAN.md).

## Danke & Lizenz

DsRunner ist ein Fork von **[Pico Launcher](https://github.com/LNH-team/pico-launcher)** und nutzt **[Pico Loader](https://github.com/LNH-team/pico-loader)**, beide vom [LNH team](https://github.com/LNH-team): [@Gericom](https://github.com/Gericom), [@XLuma](https://github.com/XLuma), [@Dartz150](https://github.com/Dartz150), [@lifehackerhansol](https://github.com/lifehackerhansol). „Zurück zu DsRunner aus dem Spiel“ und der DLDI-Cache im Pico Loader stammen von [@danyocom](https://github.com/danyocom/pico-loader). Die Autostart-Dateien für die M3 DS Real und einige Emulatoren stammen aus [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu). Icons von [icons8](https://icons8.com/).

Lizenz: Zlib, siehe `LICENSE.txt`. Weitere Lizenzen im Ordner `licenses`, die der mitgelieferten Emulatoren in `licenses/emulators`.
