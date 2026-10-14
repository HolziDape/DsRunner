# DsRunner

Ein moderner Launcher für den **Nintendo DS Lite** mit der Flashkarte **M3 DS Real**. Er startet beim Einschalten automatisch und zeigt deine Spiele in einem dunklen Kachel-Design, mit großen Artworks auf dem oberen Bildschirm. Spiele startet DsRunner über [Pico Loader](https://github.com/LNH-team/pico-loader).

![DsRunner: Konsolen, Spiele und 4×3-Raster](docs/images/DsRunner.png)

<sub>Screenshots aus dem Emulator, Artworks sind Testbilder.</sub>

## Funktionen

- **Startet automatisch** beim Einschalten des DS, ohne Umweg über ein anderes Menü
- **Zwei Ebenen:** erst die Konsolen (NDS, GBA, …), dann deren Spiele
- **Drei Ansichten** mit passendem Bildstil: große Kacheln im Raster 3×2 mit vereinfachten Cover-Bildern, kleine Kacheln im Raster 4×3 mit klaren Pixel-Icons und die **CoverFlow** mit den echten Covern. Konsolen in ihrer eigenen Farbe
- **Artwork oben** für jede Konsole und jedes Spiel, mit weichem Überblenden
- **Spielanzahl** pro Konsole und Sortierung nach **zuletzt gespielt**
- **Favoriten** mit X, gesammelt in der Kachel „★ Favoriten“
- **Weiterspielen** mit START: das zuletzt gespielte Spiel direkt von der Konsolen-Ebene starten
- Uhr (abschaltbar) und Akkuanzeige, Oberfläche auf Deutsch
- **GBA und weitere Konsolen** (NES, Game Boy, Master System, Game Gear, Mega Drive, SNES, PC Engine) über Emulatoren, die DsRunner selbst findet; eigene Konsolen über `systems.json`
- **Cheats** über `usrcheat.dat`
- **Zurück zu DsRunner aus dem Spiel:** L+R+Unten+B 2 Sekunden halten (M3-Paket, [Hinweise](#zurück-zu-dsrunner-aus-einem-spiel))

## Download

**[⬇ Neueste Version herunterladen](https://github.com/HolziDape/DsRunner/releases/latest)**

Dort gibt es diese Dateien:

| Datei | Für wen |
|---|---|
| **`DsRunner_M3DSReal.zip`** | **M3 DS Real**: fertiges Paket für die SD-Karte, inklusive Autostart und Pico Loader (mit Rückkehr aus Spielen) |
| `DsRunner.zip` | andere Flashkarten, siehe [unten](#andere-flashkarten) |
| `DsRunnerArt.exe` | PC-Programm für Windows, lädt die [Artworks](#artworks) auf die Karte |

Was sich in welcher Version geändert hat, steht im **[CHANGELOG](CHANGELOG.md)**. Welche Version auf deiner Karte läuft, siehst du unten rechts im Menü „Anzeige“ (Zahnrad oben rechts).

## Installation (M3 DS Real)

1. **SD-Karte sichern.** Mindestens alle Spielstände (`.sav`), den Ordner `SYSTEM` und die Datei `SRESET.DAT`. Damit kommst du jederzeit zu YSMenu zurück.
2. `DsRunner_M3DSReal.zip` entpacken.
3. Den **Inhalt** des entpackten Ordners ins Hauptverzeichnis der SD-Karte kopieren und vorhandene Dateien ersetzen.
4. Spiele in Ordner pro Konsole legen (siehe nächster Abschnitt).
5. Karte in den DS stecken und einschalten. DsRunner startet von selbst.

### Ordner für die Spiele

DsRunner zeigt die Unterordner von `/Games` als Konsolen an:

```
SD-Karte
├── Games
│   ├── NDS      ← .nds-Spiele
│   ├── GBA      ← .gba-Spiele (brauchen GBARunner2, siehe unten)
│   ├── NES, GB, GBC, SNES, SMS, GG, MD, PCE  ← Spiele anderer Konsolen (brauchen einen Emulator)
│   └── Apps     ← Homebrew-Programme (eigene .nds-Programme, keine Spiele)
├── _dsrunner    ← Artworks, Spielverlauf und Favoriten (legt DsRunner selbst an)
├── _nds
├── _pico
└── ...
```

- Ordner mit den Namen `NDS`, `GBA`, `GBC`, `GB`, `NES`, `SNES`, `SMS`, `GG`, `MD`, `PCE` und `Apps` bekommen eine eigene Farbe. Jeder andere Ordner erscheint ebenfalls als Kachel.
- Gibt es keinen Ordner `/Games`, zeigt DsRunner einfach das Hauptverzeichnis der Karte. Einen anderen Ordner stellst du in `/_pico/settings.json` ein (`libraryPath`).

### Emulatoren für andere Konsolen

DS-Spiele startet DsRunner selbst. Für alle anderen Konsolen braucht es einen Emulator (ein DS-Programm). DsRunner findet ihn automatisch, wenn er unter dem richtigen Namen auf der Karte liegt, am besten in **`/_dsrunner/emulators/`** (oder im Hauptverzeichnis). Ohne Emulator bleibt der Ordner der Konsole leer („Keine Spiele“).

| Konsole | Ordner | Dateien | Emulator | Dateiname auf der Karte |
|---|---|---|---|---|
| Game Boy Advance | `GBA` | `.gba` | [GBARunner2](https://github.com/Gericom/GBARunner2/releases) | `GBARunner2_arm7dldi_ds.nds`, dazu das GBA-BIOS als `/_gba/bios.bin` |
| NES / Famicom | `NES` | `.nes`, `.fds` | [nesDS](https://github.com/DS-Homebrew/NesDS/releases) | `nesDS.nds` |
| Game Boy / Game Boy Color | `GB`, `GBC` | `.gb`, `.gbc`, `.sgb` | [GameYob](https://github.com/Drenn1/GameYob/releases) | `GameYob.nds` |
| Master System / Game Gear | `SMS`, `GG` | `.sms`, `.gg` | [S8DS](https://github.com/FluBBaOfWard/S8DS/releases) | `S8DS.nds` |
| Mega Drive | `MD` | `.gen`, `.md`, `.smd` | jEnesisDS (aus [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases)) | `jEnesisDS.nds` |
| Super Nintendo | `SNES` | `.sfc`, `.smc` | SNEmulDS (aus [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases)) | `SNEmulDS-legacy.nds` |
| PC Engine | `PCE` | `.pce`, `.sgx` | [NitroGrafx](https://github.com/FluBBaOfWard/NitroGrafx/releases) | `NitroGrafx.nds` |

- **Einfachster Weg:** Hast du TWiLight Menu++ schon einmal installiert, liegen die meisten Emulatoren bereits unter `/_nds/TWiLightMenu/emulators/`. Dort findet DsRunner sie auch.
- Getestet auf dem DS ist bisher nur GBARunner2. Die anderen starten genauso wie in TWiLight Menu++, sind mit DsRunner aber noch nicht ausprobiert. Mega Drive: lädt ein `.md`-Spiel nicht, die Datei in `.gen` umbenennen.
- Andere Dateinamen, eigene Konsolen und alle Details: [docs/CONFIG.md](docs/CONFIG.md#emulatoren).

### Update auf eine neue Version

Nur eine Datei ersetzen: `DsRunner.nds` aus dem neuen Download in **`/_nds/TWiLightMenu/main.srldr`** umbenennen und die alte Datei damit überschreiben (optional zusätzlich als `/BOOT.NDS`). Spiele, Spielstände und Einstellungen bleiben erhalten.

Steht im [CHANGELOG](CHANGELOG.md) „Pico Loader geändert“, zusätzlich aus `DsRunner_M3DSReal.zip` die Dateien `picoLoader7.bin`, `picoLoader9.bin`, `aplist.bin`, `patchlist.bin` und `savelist.bin` nach `/_pico` kopieren.

### Zurück zu YSMenu

Die gesicherten Dateien `SYSTEM/g6dsload.1`, `SYSTEM/g6dsload.eng` und `SRESET.DAT` zurückkopieren, dann `boot.*`, `loader.eng` und den Ordner `_system_` löschen.

## Bedienung

| Taste | Funktion |
|---|---|
| Steuerkreuz | Auswahl bewegen |
| A | Konsole öffnen / Spiel starten |
| B | zurück zu den Konsolen |
| L / R | eine Seite weiterblättern |
| X | Spiel zu den Favoriten hinzufügen / entfernen |
| Y | Cheats für das ausgewählte Spiel |
| START | auf der Konsolen-Ebene: zuletzt gespieltes Spiel weiterspielen |
| Zahnrad (oben rechts) | Ansicht, Sortierung und Uhr ein/aus wählen |
| **L + R + Unten + B** (2 s halten, im Spiel) | zurück zu DsRunner |

Der Touchscreen funktioniert ebenfalls.

### Zurück zu DsRunner aus einem Spiel

Im laufenden DS-Spiel **L + R + Unten + B zwei Sekunden lang halten**: der DS startet neu in DsRunner, ohne Aus- und Einschalten. Das geht mit dem Pico Loader aus `DsRunner_M3DSReal.zip` (eigener Build mit Patches, siehe [loader/README.md](loader/README.md)).

- **Vorher im Spiel speichern.** Was nicht gespeichert ist, geht verloren, genau wie beim Ausschalten. Einen laufenden Speichervorgang bricht der Hotkey nicht ab, er wartet, bis er fertig ist.
- **Nicht jedes Spiel reagiert.** Fehlt im Spiel der passende Einsprungpunkt oder der Platz für den Patch, läuft das Spiel ganz normal, nur ohne Hotkey. Dann wie gewohnt ausschalten. Rückmeldungen gerne in die [Kompatibilitätsliste](docs/compat/pico-vs-ysmenu.md).
- **Abschalten:** in `/_pico/settings.json` `"inGameReset": false` eintragen. DsRunner legt dann vor jedem Spielstart `/_pico/igr.off` an, und Pico Loader baut den Hotkey nicht mehr ein.
- Homebrew kehrt wie bisher über das eigene „Beenden“ zurück.

## Artworks

Die großen Bilder für den oberen Bildschirm besorgt das PC-Programm **DsRunner Art** automatisch: SD-Karte einstecken, Programm starten, **„Bilder aktualisieren“** klicken. Es erkennt die Spiele (DS und GBA am Game-Code, andere Konsolen an Prüfsumme oder Dateiname), lädt Titelbilder, Screenshots oder Cover von [libretro-thumbnails](https://github.com/libretro-thumbnails) und erzeugt Bilder für die Konsolen-Ordner. Außerdem legt es für jedes Spiel die Bilder der drei Ansichten an: ein vereinfachtes **Kachel-Bild** für das 3×2-Raster, das echte **Cover** für die CoverFlow und für Spiele ohne eigenes Icon (GBA & Co.) ein kleines **Pixel-Icon** für das 4×3-Raster. Eigene Bilder haben Vorrang.

- Windows: `DsRunnerArt.exe` aus dem [Release](https://github.com/HolziDape/DsRunner/releases/latest), oder mit Python aus `tools/dsrunner-art`
- Anleitung, Optionen und Fehlerbehebung: **[docs/ART-TOOL.md](docs/ART-TOOL.md)**

Von Hand geht es auch: ein BMP mit **256 × 192 Pixeln und 256 Farben** unter `/_dsrunner/art` ablegen, im selben Pfad wie das Spiel:

| Für | Datei |
|---|---|
| Konsole `/Games/NDS` | `/_dsrunner/art/Games/NDS.bmp` |
| Spiel `/Games/NDS/Mario Kart DS.nds` | `/_dsrunner/art/Games/NDS/Mario Kart DS.bmp` |

Dafür reicht Paint: Bild auf 256 × 192 Pixel bringen und mit „Speichern unter“ als **256-Farben-Bitmap** speichern. Von Hand abgelegte Bilder überschreibt DsRunner Art nicht.

Alle Einstellungen und Dateiformate stehen in **[docs/CONFIG.md](docs/CONFIG.md)**.

## Andere Flashkarten

DsRunner läuft überall, wo Pico Loader läuft. Getestet ist bisher nur die M3 DS Real.

1. `DsRunner.zip` entpacken und den Inhalt (`DsRunner.nds`, `_pico`) ins Hauptverzeichnis der SD-Karte kopieren.
2. Bei [Pico Loader](https://github.com/LNH-team/pico-loader/releases) die Dateien für deine Flashkarte herunterladen und `aplist.bin`, `savelist.bin`, `patchlist.bin`, `picoLoader7.bin` und `picoLoader9.bin` nach `/_pico` kopieren.
3. `DsRunner.nds` aus dem Menü deiner Flashkarte starten. Ein Autostart ist für andere Karten nicht vorbereitet.

Mit dem offiziellen Pico Loader gibt es keine Rückkehr aus Spielen. Wer sie ausprobieren will, baut den Loader aus [loader/](loader/README.md) für seine Karte (`PICO_PLATFORM=R4 loader/build.sh`, Plattformnamen wie bei Pico Loader). Getestet ist das nicht.

## Stand

DsRunner ist in Entwicklung. Was fertig ist und was als Nächstes kommt, steht in **[docs/PLAN.md](docs/PLAN.md)**, die Änderungen pro Version im **[CHANGELOG](CHANGELOG.md)**. Fehler und Wünsche gerne als [Issue](https://github.com/HolziDape/DsRunner/issues).

## Selbst bauen

Mit Docker (unter Windows in WSL2):

```sh
git clone --recursive https://github.com/HolziDape/DsRunner.git
cd DsRunner
./build.sh            # → DsRunner.nds
./build.sh package    # → fertiges SD-Paket für die M3 DS Real in sd-package/
loader/build.sh       # → Pico Loader mit Rückkehr aus Spielen in loader/out/ (package ruft das bei Bedarf selbst auf)
```

Details, Test im Emulator und Pico-Updates: **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)**.

## Danke & Lizenz

DsRunner ist ein Fork von **[Pico Launcher](https://github.com/LNH-team/pico-launcher)** und nutzt **[Pico Loader](https://github.com/LNH-team/pico-loader)**, beide vom [LNH team](https://github.com/LNH-team): [@Gericom](https://github.com/Gericom), [@XLuma](https://github.com/XLuma), [@Dartz150](https://github.com/Dartz150), [@lifehackerhansol](https://github.com/lifehackerhansol). Die Rückkehr aus Spielen und der DLDI-Cache im Pico Loader stammen von [@danyocom](https://github.com/danyocom/pico-loader) (Branches `igr-hooks`, `dldi-cache`). Die Autostart-Dateien für die M3 DS Real stammen aus [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu). Icons von [icons8](https://icons8.com/).

Lizenz: Zlib, siehe `LICENSE.txt`. Weitere Lizenzen im Ordner `licenses`.

Die englische Dokumentation von Pico Launcher (Themes, Cheats, eigene Icons) liegt weiterhin unter [docs/](docs/Usage.md). Sie gilt größtenteils auch für DsRunner.
