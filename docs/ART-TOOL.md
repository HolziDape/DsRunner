# DsRunner Art: Artworks automatisch auf die Karte

**DsRunner Art** ist ein kleines PC-Programm für Windows (läuft auch unter Linux und macOS). Es liest die Spiele auf deiner SD-Karte, sucht passende Bilder im Internet und speichert sie im Format, das DsRunner auf dem oberen Bildschirm zeigt. Konsolen-Ordner bekommen ein eigenes Bild in der Farbe der Konsole. Außerdem bekommt jedes Spiel ein kleines [Kachel-Bild](#kachel-bilder) für das Raster im Launcher.

## Installation

### Variante A: fertige `DsRunnerArt.exe`

Bei jedem [Release](https://github.com/HolziDape/DsRunner/releases/latest) liegt eine `DsRunnerArt.exe`. Herunterladen und doppelklicken, mehr braucht es nicht.

Windows zeigt beim ersten Start eventuell „Der Computer wurde durch Windows geschützt“ (SmartScreen), weil die Datei nicht signiert ist. Dann auf **Weitere Informationen → Trotzdem ausführen** klicken.

### Variante B: mit Python

1. **Python installieren** (Version 3.10 oder neuer) von [python.org](https://www.python.org/downloads/). Im Installer den Haken bei **„Add python.exe to PATH“** setzen. „tcl/tk and IDLE“ muss ausgewählt bleiben (Standard), sonst fehlt das Fenster.
2. **DsRunner herunterladen:** auf GitHub *Code → Download ZIP* und entpacken, oder per `git clone`.
3. Im entpackten Ordner nach `tools\dsrunner-art` wechseln, in die Adresszeile des Explorers `cmd` tippen und Enter drücken. Im schwarzen Fenster einmalig:
   ```
   py -m pip install -r requirements.txt
   ```
   Das installiert [Pillow](https://python-pillow.org/) (Bildbearbeitung). Sonst braucht das Tool nichts.
4. Starten: **Doppelklick auf `dsrunner_art.py`** oder `py dsrunner_art.py`.

## Bedienung

1. SD-Karte in den PC stecken.
2. DsRunner Art starten. Bei **SD-Karte** steht die Karte schon drin, wenn darauf DsRunner liegt (erkennbar an „(DsRunner)“). Sonst aus der Liste wählen oder mit **Ordner wählen …** einen Ordner angeben. **↻** liest die Laufwerke neu ein.
3. Optionen prüfen (siehe unten) und **Artworks aktualisieren** klicken.
4. Warten, bis „Fertig“ erscheint. Im Protokoll steht für jedes Spiel, welches Bild es bekommen hat, und bei Spielen ohne Bild der Grund. Mit **Details anzeigen** erscheinen auch die Spiele, an denen sich nichts geändert hat, und woher die Kachel-Bilder stammen.
5. Karte sicher entfernen und in den DS stecken.

Beim nächsten Mal lädt das Tool nur noch, was fehlt: neue Spiele, neue eigene Bilder. Das geht dann in wenigen Sekunden.

**Prüfen** kontrolliert alle Bilder in `/_dsrunner/art` und `/_dsrunner/tiles` so, wie DsRunner sie lädt, und meldet unbrauchbare Dateien (zum Beispiel von Hand gespeicherte Bilder in der falschen Größe). **Artwork-Ordner öffnen** zeigt den Ordner im Explorer.

### Optionen

| Option | Bedeutung |
|---|---|
| **Bildart** | Welche Bilder bevorzugt werden. **Titelbild** (Standard): der Titelbildschirm des Spiels, bei DS-Spielen meist der obere Bildschirm (siehe [Titelbild oder Cover](#titelbild-oder-cover)). **Screenshot**: eine Szene aus dem Spiel. **Cover**: die Spielverpackung, oben angeschnitten, damit das Logo im sichtbaren Teil liegt. Gibt es die gewählte Art nicht, nimmt das Tool die anderen in der Reihenfolge Titelbild → Cover → Screenshot. |
| **Region** | Welche Fassung bevorzugt wird, wenn es ein Spiel mehrfach gibt (Deutschland, Europa, USA, Japan). Steht die Region im Dateinamen (`(Germany)`, `(E)`, `GER`) oder im Game-Code, hat diese Vorrang. |
| **Vorhandene überschreiben** | Lädt alle Bilder neu, die das Tool früher angelegt hat, und sucht auch Bilder erneut, die beim letzten Mal nicht gefunden wurden. Nötig, wenn du die Bildart oder Region wechselst. Von Hand abgelegte Bilder bleiben dabei unangetastet (dafür gibt es die nächste Option). |
| **Auch von Hand abgelegte Bilder ersetzen** | Normalerweise lässt das Tool BMP-Dateien in Ruhe, die es nicht selbst geschrieben hat (z. B. ein Testbild, das du nach `_dsrunner/art` kopiert hast). Mit dieser Option ersetzt es sie, sobald es ein Bild findet. Am Ende jedes Laufs steht, wie viele von Hand abgelegte Bilder behalten wurden. Nur bei Bedarf einschalten, die Option wird nicht gespeichert. |
| **Bilder für Konsolen-Ordner erzeugen** | Jeder Konsolen-Ordner (`NDS`, `GBA`, …) und jeder Unterordner bekommt ein Bild: Farbverlauf in der Konsolenfarbe mit großem Kürzel. Dafür ist kein Internet nötig. |
| **Spiel-Icon als Ersatzbild** | Findet das Tool für ein DS-Spiel kein Bild (z. B. Homebrew, keine Verbindung), baut es eins aus dem Icon des Spiels, auch für die Kachel. Bei der nächsten Aktualisierung wird erneut nach einem richtigen Bild gesucht. |
| **Kachel-Bilder erzeugen** | Jedes Spiel bekommt ein Kachel-Bild (64 × 64) für das Raster, siehe [Kachel-Bilder](#kachel-bilder). Standard: an. |
| **Bilder gelöschter Spiele entfernen** | Löscht Bilder und Kachel-Bilder, die das Tool für Spiele angelegt hat, die nicht mehr auf der Karte sind. |
| **Eigene Bilder** | Ordner mit eigenen Bildern (siehe unten). Leer = `_dsrunner\art-override` auf der Karte. |

## Woher die Bilder kommen

| Quelle | Wofür |
|---|---|
| [libretro-thumbnails](https://github.com/libretro-thumbnails) | Titelbilder, Screenshots und Cover für alle Konsolen. Frei, ohne Konto. |
| [libretro-database](https://github.com/libretro/libretro-database) | Namenslisten (No-Intro) mit Game-Codes und Prüfsummen, um die Spiele zu erkennen. |
| [GameTDB](https://www.gametdb.com) | Cover von DS-Spielen über den Game-Code, falls libretro keins hat. |

**So erkennt das Tool ein Spiel:**

1. **DS und GBA:** über den 4-stelligen Game-Code im ROM (z. B. `AMCP` = Mario Kart DS, Europa). Das ist eindeutig, der Dateiname spielt dann keine Rolle.
2. **Andere Konsolen** (GB, GBC, NES, SNES, …): über die Prüfsumme der Datei (CRC32).
3. **Sonst über den Namen:** Dateiname bzw. Titel aus dem DS-Banner. Klammern wie `(Europe)`, `[!]`, Nummern wie `0042 - ` und Groß-/Kleinschreibung werden ignoriert. Bei leicht abweichenden Namen sucht das Tool den ähnlichsten (Zahlen müssen aber übereinstimmen, „FIFA 08“ ist nicht „FIFA 09“). Fehlen Wörter oder ein Untertitel („Pokemon Rot“ → „Pokemon - Rote Edition“, „Donkey Kong Country 2“ → „… - Diddy's Kong Quest“), klappt es ebenfalls, solange nur ein Spiel passt.

Die Bilder landen **nur auf deiner Karte**. Sie gehören den jeweiligen Rechteinhabern und werden weder im Repository noch in Releases verteilt.

## Titelbild oder Cover

DS-Titelbilder liegen bei libretro meist als beide Bildschirme übereinander vor (256 × 384). Das Tool wählt so:

1. **Übereinander (256 × 384):** der obere Bildschirm. Nur wenn der fast leer ist (eine Farbe bedeckt mehr als 60 %) und der untere nicht, der untere.
2. **Nebeneinander (384 × 256, Spiele für den gedrehten DS wie Dr. Kawashima):** der linke Bildschirm, aufrecht vor einem unscharfen Hintergrund.
3. **Kein Titelbild vorhanden:** das **Cover**, erst danach ein Screenshot. Ein Screenshot ist irgendeine Spielszene (eine Levelkarte, ein Menü), das Cover sieht fast immer besser aus.

Getestet mit 17 DS-Spielen mit Titelbild (u. a. Mario Kart DS, Pokémon Schwarz 1 und 2, Plants vs. Zombies, Nintendogs, Zelda, Tetris, Professor Layton, Dr. Kawashima). Bei 16 zeigt der gewählte Bildschirm das Logo, nur bei Nintendogs nicht (Hund im Zimmer, das Logo liegt unten). Manche Titelbilder stammen aus der japanischen Fassung (z. B. Pokémon Platin). Darum bleibt **Titelbild** der Standard: es ist pixelgenau 256 × 192, das Cover muss beschnitten werden. „Oben“ (Up) hat bei libretro gar kein Titelbild; früher kam deshalb der Screenshot (eine Levelkarte mit Bestzeiten), jetzt das Cover. Wer lieber überall Cover will: **Bildart → Cover** und **Vorhandene überschreiben**.

## Kachel-Bilder

Die Icons der DS-Spiele (32 × 32) sind in den großen Kacheln grob, GBA-Spiele haben gar keins. Deshalb legt das Tool für jedes Spiel ein **Kachel-Bild** (64 × 64) an, das DsRunner in der Kachel statt des Icons zeigt:

- Pfad wie beim Artwork, nur unter `/_dsrunner/tiles`: `/Games/NDS/Mario Kart DS.nds` → `/_dsrunner/tiles/Games/NDS/Mario Kart DS.bmp`. Ordner bekommen keins.
- Inhalt: das **Cover**, auf 64 × 64 gefüllt (der Streifen „NINTENDO DS“ bzw. „GAME BOY ADVANCE“ am linken Rand wird abgeschnitten). Ersatz der Reihe nach: Titelbild, Screenshot, das Icon des Spiels doppelt groß (pixelgenau) auf dunklem Grund.
- Verkleinert mit Lanczos, auf 255 Farben mit Dithering, Format wie in [CONFIG.md](CONFIG.md#kachel-bilder).
- Es gelten dieselben Regeln wie bei den Artworks: Liste in `/_dsrunner/tiles/.dsrunner-art.json`, von Hand abgelegte Kachel-Bilder bleiben (außer mit **Auch von Hand abgelegte Bilder ersetzen**), Icon-Ersatz wird beim nächsten Lauf erneut gesucht.

## Eigene Bilder

Eigene Bilder haben immer Vorrang, auch vor einem von Hand abgelegten BMP. Erlaubt sind PNG, JPG, BMP, GIF und WebP in beliebiger Größe. Das Tool schneidet sie auf 256 × 192 zu (etwas oberhalb der Mitte) und wandelt sie um. Eigene **Kachel-Bilder** kommen genauso in den Ordner `_dsrunner/tile-override` (gleicher Pfad oder nur der Name) und werden auf 64 × 64 zugeschnitten.

Ablage im Ordner `_dsrunner/art-override` auf der Karte (oder im gewählten Ordner auf dem PC), auf zwei Arten:

| Art | Beispiel | gilt für |
|---|---|---|
| gleicher Pfad wie auf der Karte | `_dsrunner/art-override/Games/NDS/Mario Kart DS.png` | genau dieses Spiel |
| nur der Name | `_dsrunner/art-override/Mario Kart DS.jpg` | jedes Spiel bzw. jeden Ordner mit diesem Namen |
| Konsolen-Ordner | `_dsrunner/art-override/Games/GBA.png` oder `_dsrunner/art-override/GBA.png` | den Ordner `/Games/GBA` |

Ändert sich ein eigenes Bild, wird es bei der nächsten Aktualisierung neu umgewandelt.

**Tipp zur Gestaltung:** Unten liegen Titel und Untertitel (ab Zeile 92 wird abgedunkelt), rechts oben Uhr und Akku. Wichtiges gehört in die oberen ~90 Zeilen.

## Was auf der Karte passiert

- Bilder liegen unter `/_dsrunner/art/` im selben Pfad wie das Spiel, mit Endung `.bmp` (Details: [CONFIG.md](CONFIG.md#artworks)):
  `/Games/NDS/Mario Kart DS.nds` → `/_dsrunner/art/Games/NDS/Mario Kart DS.bmp`, Ordner `/Games/NDS` → `/_dsrunner/art/Games/NDS.bmp`.
- Format: BMP 256 × 192, 8 bit, höchstens 255 Farben aus dem 15-bit-Farbraum des DS, Floyd-Steinberg-Dithering, Palettenfarbe 0 bleibt frei. Etwa 50 KB pro Bild.
- Kachel-Bilder liegen genauso unter `/_dsrunner/tiles/` (nur Spiele, 64 × 64, siehe [Kachel-Bilder](#kachel-bilder)).
- Die Datei `/_dsrunner/art/.dsrunner-art.json` (bzw. `/_dsrunner/tiles/.dsrunner-art.json`) merkt sich, welche Bilder vom Tool stammen. Zusätzlich trägt jedes BMP des Tools eine Kennung im Dateikopf, damit es auch dann erkannt wird, wenn die Liste verloren geht (z. B. Fenster während des Laufs geschlossen). **Bilder, die du selbst dorthin kopiert hast, überschreibt das Tool nur mit „Auch von Hand abgelegte Bilder ersetzen“** (`--replace-manual`). Wie viele es behalten hat, steht am Ende des Protokolls; kann DsRunner so ein Bild gar nicht anzeigen (falsche Größe o. Ä.), wird das gemeldet.
- Welcher Ordner die Bibliothek ist, liest das Tool aus `/_pico/settings.json` (`libraryPath`, Standard `/Games`). Gibt es ihn nicht, nimmt es das Hauptverzeichnis (ohne Ordner wie `_nds`, `_pico`, `SYSTEM`).

## Kommandozeile

Mit Argumenten startet das Tool ohne Fenster, praktisch für Skripte:

```
py dsrunner_art.py --sd E:\
py dsrunner_art.py --sd E:\ --type boxart --force
py dsrunner_art.py --sd E:\ --offline
py dsrunner_art.py --sd E:\ --check
```

Gleichwertig: `py -m dsrunnerart --sd E:\` im Ordner `tools\dsrunner-art`. Die `DsRunnerArt.exe` ist eine reine Fensteranwendung, für die Kommandozeile bitte Python verwenden.

| Option | Bedeutung |
|---|---|
| `--sd LAUFWERK` | SD-Karte oder Ordner (`E:\`, `/media/sd`) |
| `--type title\|snap\|boxart` | bevorzugte Bildart (auch `titel`, `screenshot`, `cover`) |
| `--region de\|eu\|us\|jp` | bevorzugte Region |
| `--force` | vom Tool erzeugte Bilder neu laden |
| `--offline` | nichts herunterladen: nur eigene Bilder, Konsolen-Bilder, Icons |
| `--override ORDNER` | Ordner mit eigenen Bildern |
| `--sources libretro,gametdb` | Quellen und ihre Reihenfolge |
| `--no-console-art`, `--no-icon` | keine Konsolen-Bilder bzw. Icon-Ersatzbilder |
| `--no-tiles` | keine Kachel-Bilder erzeugen |
| `--tile-override ORDNER` | Ordner mit eigenen Kachel-Bildern (Standard: `_dsrunner\tile-override` auf der Karte) |
| `--replace-manual` | auch von Hand abgelegte Bilder ersetzen |
| `--no-crc` | keine Prüfsummen berechnen (schneller bei sehr vielen großen Dateien) |
| `--clean` | Bilder und Kachel-Bilder gelöschter Spiele entfernen |
| `--dry-run` | nur anzeigen, was passieren würde |
| `--check` | vorhandene Bilder und Kachel-Bilder prüfen |
| `--library PFAD` | anderer Bibliotheksordner als in `settings.json` |
| `--retry-missing` | auch Bilder neu suchen, die in den letzten 14 Tagen nicht gefunden wurden |
| `--cache ORDNER` | Cache-Ordner |
| `-v` | ausführliches Protokoll |

## Cache auf dem PC

Damit nichts doppelt geladen wird, speichert das Tool Namenslisten, heruntergeladene Bilder, Prüfsummen und „nicht gefunden“-Einträge in

- Windows: `%LOCALAPPDATA%\DsRunnerArt\cache`
- Linux/macOS: `~/.cache/dsrunner-art`

Der Ordner darf jederzeit gelöscht werden, er wird beim nächsten Lauf neu angelegt. Die Namenslisten werden nach 30 Tagen erneuert, „nicht gefunden“ wird nach 14 Tagen erneut versucht.

## Fehlerbehebung

| Problem | Lösung |
|---|---|
| **„Spiel nicht erkannt“** | Den Dateinamen an den offiziellen Namen anpassen (z. B. `Golden Sun.gba` statt `gs_1.gba`), oder ein eigenes Bild in den Override-Ordner legen. DS- und GBA-Spiele werden normalerweise über den Game-Code erkannt; Homebrew und ROM-Hacks haben oft keinen. |
| **„kein Bild gefunden“** obwohl das Spiel erkannt wurde | Für manche Spiele hat libretro-thumbnails (noch) keine Bilder. Andere Bildart wählen, später erneut versuchen (`--retry-missing`) oder ein eigenes Bild verwenden. DS-Spiele bekommen dann das Icon-Ersatzbild. |
| **Ein Spiel hat gar kein Bild** | Im Protokoll steht hinter „kein Bild gefunden“ der Grund, z. B. „Icon-Ersatzbild ist ausgeschaltet“, „das ROM hat kein Icon“ oder „nur DS-Spiele haben ein Icon“. Steht dort nichts zu dem Spiel, liegt es außerhalb der Bibliothek (siehe unten) oder tiefer als vier Ordner. |
| **Ein altes Testbild erscheint statt des richtigen Bilds** | Das Bild wurde von Hand abgelegt und bleibt deshalb. **Auch von Hand abgelegte Bilder ersetzen** einschalten (`--replace-manual`) oder die Datei löschen. |
| **Falsches Bild** | Eigenes Bild in den Override-Ordner legen. Oder die BMP-Datei löschen und mit anderer Region bzw. Bildart neu laden. |
| **„… nicht erreichbar, diese Quelle wird übersprungen“** | Keine Internetverbindung, Firewall oder der Server ist gerade nicht erreichbar. Später erneut starten; was schon geladen wurde, bleibt. |
| **„Dateiliste von libretro-thumbnails nicht verfügbar“** | Die GitHub-API erlaubt ohne Anmeldung nur 60 Anfragen pro Stunde. Das Tool sucht die Bilder dann direkt, das ist nur etwas langsamer. |
| **Fenster öffnet sich nicht / „tkinter fehlt“** | Python neu installieren und „tcl/tk and IDLE“ ausgewählt lassen. Oder die Kommandozeile mit `--sd` verwenden. |
| **Doppelklick auf `dsrunner_art.py` öffnet einen Editor** | Rechtsklick → Öffnen mit → Python, oder im Ordner `py dsrunner_art.py` eingeben. |
| **Bild erscheint nicht auf dem DS** | Artworks und Kachel-Bilder zeigt DsRunner in den Kachel-Ansichten (3×2, 4×3). Mit **Prüfen** kontrollieren, ob alle Dateien in Ordnung sind. Liegt das Bild im richtigen Pfad (siehe oben)? |
| **„Pfad zu lang für den Launcher“** | DsRunner kann Bildpfade bis 255 Zeichen laden. Den Spiel- oder Ordnernamen kürzen. |

## Für Entwickler

- Code: `tools/dsrunner-art/dsrunnerart/` (`sdcard.py` Pfade und Scan, `romheader.py`, `identify.py`, `sources.py`, `imaging.py`, `updater.py`, `gui.py`, `cli.py`)
- Tests (ohne Netz, mit erzeugten ROM-Köpfen): `py -m unittest discover -s tests -t .` im Ordner `tools/dsrunner-art`
- Weitere Quelle (z. B. SteamGridDB oder ScreenScraper, die einen API-Key brauchen): in `sources.py` von `ArtSource` ableiten, `candidates()` (und bei Bedarf `load()`) umsetzen und in `build_sources()` eintragen.
- Die Pfadregel in `sdcard.art_path` entspricht `DsRunnerFileInfoCardView::RequestArt`, die Prüfung in `imaging.validate_bmp` entspricht `DsRunnerTopArt::Load`. Kachel-Bilder nutzen dieselbe Regel mit `sdcard.TILE_FOLDER` (`tile_path_for_entry`) und `validate_bmp(data, (64, 64))`. Ändert sich eins davon im Launcher, müssen Tool und Tests mitziehen.
- Die Wahl der Bildschirmhälfte steht in `imaging.pick_screen`, die Reihenfolge der Bildarten in `sources.FALLBACK_ORDER` und `sources.TILE_ORDER`.
- Release: Der Workflow `release.yml` baut `DsRunnerArt.exe` mit PyInstaller auf `windows-latest` und hängt sie an das Release.
