# DsRunner

Ein moderner Launcher für den **Nintendo DS Lite** mit der Flashkarte **M3 DS Real**. Er startet beim Einschalten automatisch und zeigt deine Spiele in einem dunklen Kachel-Design, mit großen Artworks auf dem oberen Bildschirm. Spiele startet DsRunner über [Pico Loader](https://github.com/LNH-team/pico-loader).

![DsRunner: Konsolen, Spiele und 4×3-Raster](docs/images/DsRunner.png)

<sub>Screenshots aus dem Emulator, Artworks sind Testbilder.</sub>

## Funktionen

- **Startet automatisch** beim Einschalten des DS, ohne Umweg über ein anderes Menü
- **Zwei Ebenen:** erst die Konsolen (NDS, GBA, …), dann deren Spiele
- **Große Kacheln** im Raster 3×2 oder 4×3, Konsolen in ihrer eigenen Farbe
- **Artwork oben** für jede Konsole und jedes Spiel, mit weichem Überblenden
- **Spielanzahl** pro Konsole und Sortierung nach **zuletzt gespielt**
- **Favoriten** mit X, gesammelt in der Kachel „★ Favoriten“
- **Weiterspielen** mit START: das zuletzt gespielte Spiel direkt von der Konsolen-Ebene starten
- Uhr (abschaltbar) und Akkuanzeige, Oberfläche auf Deutsch
- **GBA-Spiele** über GBARunner2, **Cheats** über `usrcheat.dat`

## Download

**[⬇ Neueste Version herunterladen](https://github.com/HolziDape/DsRunner/releases/latest)**

Dort gibt es zwei Dateien:

| Datei | Für wen |
|---|---|
| **`DsRunner_M3DSReal.zip`** | **M3 DS Real**: fertiges Paket für die SD-Karte, inklusive Autostart und Pico Loader |
| `DsRunner.zip` | andere Flashkarten, siehe [unten](#andere-flashkarten) |

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
│   └── Apps     ← Homebrew
├── _dsrunner    ← Artworks, Spielverlauf und Favoriten (legt DsRunner selbst an)
├── _nds
├── _pico
└── ...
```

- Ordner mit den Namen `NDS`, `GBA`, `GBC`, `GB`, `NES`, `SNES`, `SMS`, `GG`, `MD`, `PCE` und `Apps` bekommen eine eigene Farbe. Jeder andere Ordner erscheint ebenfalls als Kachel.
- Gibt es keinen Ordner `/Games`, zeigt DsRunner einfach das Hauptverzeichnis der Karte. Einen anderen Ordner stellst du in `/_pico/settings.json` ein (`libraryPath`).
- **GBA:** [GBARunner2](https://github.com/Gericom/GBARunner2) (`GBARunner2_arm7dldi_ds.nds`) ins Hauptverzeichnis legen, dazu das GBA-BIOS als `/_gba/bios.bin`. DsRunner findet es automatisch.

### Update auf eine neue Version

Nur eine Datei ersetzen: `DsRunner.nds` aus dem neuen Download in **`/_nds/TWiLightMenu/main.srldr`** umbenennen und die alte Datei damit überschreiben (optional zusätzlich als `/BOOT.NDS`). Spiele, Spielstände und Einstellungen bleiben erhalten.

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

Der Touchscreen funktioniert ebenfalls.

## Artworks

Für das große Bild oben legst du ein BMP mit **256 × 192 Pixeln und 256 Farben** unter `/_dsrunner/art` ab, im selben Pfad wie das Spiel:

| Für | Datei |
|---|---|
| Konsole `/Games/NDS` | `/_dsrunner/art/Games/NDS.bmp` |
| Spiel `/Games/NDS/Mario Kart DS.nds` | `/_dsrunner/art/Games/NDS/Mario Kart DS.bmp` |

Zum Ausprobieren reicht Paint: Bild auf 256 × 192 Pixel bringen und mit „Speichern unter“ als **256-Farben-Bitmap** speichern. Ein PC-Tool, das Artworks automatisch besorgt, ist in Arbeit.

Alle Einstellungen und Dateiformate stehen in **[docs/CONFIG.md](docs/CONFIG.md)**.

## Andere Flashkarten

DsRunner läuft überall, wo Pico Loader läuft. Getestet ist bisher nur die M3 DS Real.

1. `DsRunner.zip` entpacken und den Inhalt (`DsRunner.nds`, `_pico`) ins Hauptverzeichnis der SD-Karte kopieren.
2. Bei [Pico Loader](https://github.com/LNH-team/pico-loader/releases) die Dateien für deine Flashkarte herunterladen und `aplist.bin`, `savelist.bin`, `patchlist.bin`, `picoLoader7.bin` und `picoLoader9.bin` nach `/_pico` kopieren.
3. `DsRunner.nds` aus dem Menü deiner Flashkarte starten. Ein Autostart ist für andere Karten nicht vorbereitet.

## Stand

DsRunner ist in Entwicklung. Was fertig ist und was als Nächstes kommt, steht in **[docs/PLAN.md](docs/PLAN.md)**. Fehler und Wünsche gerne als [Issue](https://github.com/HolziDape/DsRunner/issues).

## Selbst bauen

Mit Docker (unter Windows in WSL2):

```sh
git clone --recursive https://github.com/HolziDape/DsRunner.git
cd DsRunner
./build.sh            # → DsRunner.nds
./build.sh package    # → fertiges SD-Paket für die M3 DS Real in sd-package/
```

Details, Test im Emulator und Pico-Updates: **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)**.

## Danke & Lizenz

DsRunner ist ein Fork von **[Pico Launcher](https://github.com/LNH-team/pico-launcher)** und nutzt **[Pico Loader](https://github.com/LNH-team/pico-loader)**, beide vom [LNH team](https://github.com/LNH-team): [@Gericom](https://github.com/Gericom), [@XLuma](https://github.com/XLuma), [@Dartz150](https://github.com/Dartz150), [@lifehackerhansol](https://github.com/lifehackerhansol). Die Autostart-Dateien für die M3 DS Real stammen aus [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu). Icons von [icons8](https://icons8.com/).

Lizenz: Zlib, siehe `LICENSE.txt`. Weitere Lizenzen im Ordner `licenses`.

Die englische Dokumentation von Pico Launcher (Themes, Cheats, eigene Icons) liegt weiterhin unter [docs/](docs/Usage.md). Sie gilt größtenteils auch für DsRunner.
