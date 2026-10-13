# DsRunner Art: Artworks automatisch auf die Karte

**DsRunner Art** ist ein kleines PC-Programm für Windows (läuft auch unter Linux und macOS). Es liest die Spiele auf deiner SD-Karte, sucht passende Bilder im Internet und speichert sie im Format, das DsRunner auf dem oberen Bildschirm zeigt. Konsolen-Ordner bekommen ein eigenes Bild in der Farbe der Konsole.

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
4. Warten, bis „Fertig“ erscheint. Im Protokoll steht für jedes Spiel, welches Bild es bekommen hat. Mit **Details anzeigen** erscheinen auch die Spiele, an denen sich nichts geändert hat.
5. Karte sicher entfernen und in den DS stecken.

Beim nächsten Mal lädt das Tool nur noch, was fehlt: neue Spiele, neue eigene Bilder. Das geht dann in wenigen Sekunden.

**Prüfen** kontrolliert alle Bilder in `/_dsrunner/art` so, wie DsRunner sie lädt, und meldet unbrauchbare Dateien (zum Beispiel von Hand gespeicherte Bilder in der falschen Größe). **Artwork-Ordner öffnen** zeigt den Ordner im Explorer.

### Optionen

| Option | Bedeutung |
|---|---|
| **Bildart** | Welche Bilder bevorzugt werden. **Titelbild** (Standard): der Titelbildschirm des Spiels, bei DS-Spielen der obere Bildschirm. **Screenshot**: eine Szene aus dem Spiel. **Cover**: die Spielverpackung, oben angeschnitten, damit das Logo im sichtbaren Teil liegt. Gibt es die gewählte Art nicht, nimmt das Tool die anderen (Titelbild → Screenshot → Cover). |
| **Region** | Welche Fassung bevorzugt wird, wenn es ein Spiel mehrfach gibt (Deutschland, Europa, USA, Japan). Steht die Region im Dateinamen (`(Germany)`, `(E)`, `GER`) oder im Game-Code, hat diese Vorrang. |
| **Vorhandene überschreiben** | Lädt alle Bilder neu, die das Tool früher angelegt hat, und sucht auch Bilder erneut, die beim letzten Mal nicht gefunden wurden. Nötig, wenn du die Bildart oder Region wechselst. Von Hand abgelegte Bilder bleiben trotzdem unangetastet. |
| **Bilder für Konsolen-Ordner erzeugen** | Jeder Konsolen-Ordner (`NDS`, `GBA`, …) und jeder Unterordner bekommt ein Bild: Farbverlauf in der Konsolenfarbe mit großem Kürzel. Dafür ist kein Internet nötig. |
| **Spiel-Icon als Ersatzbild** | Findet das Tool für ein DS-Spiel kein Bild (z. B. Homebrew), baut es eins aus dem Icon des Spiels. Bei der nächsten Aktualisierung wird erneut nach einem richtigen Bild gesucht. |
| **Bilder gelöschter Spiele entfernen** | Löscht Bilder, die das Tool für Spiele angelegt hat, die nicht mehr auf der Karte sind. |
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
3. **Sonst über den Namen:** Dateiname bzw. Titel aus dem DS-Banner. Klammern wie `(Europe)`, `[!]`, Nummern wie `0042 - ` und Groß-/Kleinschreibung werden ignoriert. Bei leicht abweichenden Namen sucht das Tool den ähnlichsten (Zahlen müssen aber übereinstimmen, „FIFA 08“ ist nicht „FIFA 09“).

Die Bilder landen **nur auf deiner Karte**. Sie gehören den jeweiligen Rechteinhabern und werden weder im Repository noch in Releases verteilt.

## Eigene Bilder

Eigene Bilder haben immer Vorrang, auch vor einem von Hand abgelegten BMP. Erlaubt sind PNG, JPG, BMP, GIF und WebP in beliebiger Größe. Das Tool schneidet sie auf 256 × 192 zu (etwas oberhalb der Mitte) und wandelt sie um.

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
- Die Datei `/_dsrunner/art/.dsrunner-art.json` merkt sich, welche Bilder vom Tool stammen. **Bilder, die du selbst dorthin kopiert hast, überschreibt das Tool nie.** Willst du so ein Bild ersetzen lassen, lösch es oder leg ein eigenes Bild in den Override-Ordner.
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
| `--no-crc` | keine Prüfsummen berechnen (schneller bei sehr vielen großen Dateien) |
| `--clean` | Bilder gelöschter Spiele entfernen |
| `--dry-run` | nur anzeigen, was passieren würde |
| `--check` | vorhandene Bilder prüfen |
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
| **„kein Bild gefunden“** obwohl das Spiel erkannt wurde | Für manche Spiele hat libretro-thumbnails (noch) keine Bilder. Andere Bildart wählen, später erneut versuchen (`--retry-missing`) oder ein eigenes Bild verwenden. |
| **Falsches Bild** | Eigenes Bild in den Override-Ordner legen. Oder die BMP-Datei löschen und mit anderer Region bzw. Bildart neu laden. |
| **„… nicht erreichbar, diese Quelle wird übersprungen“** | Keine Internetverbindung, Firewall oder der Server ist gerade nicht erreichbar. Später erneut starten; was schon geladen wurde, bleibt. |
| **„Dateiliste von libretro-thumbnails nicht verfügbar“** | Die GitHub-API erlaubt ohne Anmeldung nur 60 Anfragen pro Stunde. Das Tool sucht die Bilder dann direkt, das ist nur etwas langsamer. |
| **Fenster öffnet sich nicht / „tkinter fehlt“** | Python neu installieren und „tcl/tk and IDLE“ ausgewählt lassen. Oder die Kommandozeile mit `--sd` verwenden. |
| **Doppelklick auf `dsrunner_art.py` öffnet einen Editor** | Rechtsklick → Öffnen mit → Python, oder im Ordner `py dsrunner_art.py` eingeben. |
| **Bild erscheint nicht auf dem DS** | Artworks zeigt DsRunner in den Kachel-Ansichten (3×2, 4×3). Mit **Prüfen** kontrollieren, ob alle Dateien in Ordnung sind. Liegt das Bild im richtigen Pfad (siehe oben)? |
| **„Pfad zu lang für den Launcher“** | DsRunner kann Bildpfade bis 255 Zeichen laden. Den Spiel- oder Ordnernamen kürzen. |

## Für Entwickler

- Code: `tools/dsrunner-art/dsrunnerart/` (`sdcard.py` Pfade und Scan, `romheader.py`, `identify.py`, `sources.py`, `imaging.py`, `updater.py`, `gui.py`, `cli.py`)
- Tests (ohne Netz, mit erzeugten ROM-Köpfen): `py -m unittest discover -s tests -t .` im Ordner `tools/dsrunner-art`
- Weitere Quelle (z. B. SteamGridDB oder ScreenScraper, die einen API-Key brauchen): in `sources.py` von `ArtSource` ableiten, `candidates()` (und bei Bedarf `load()`) umsetzen und in `build_sources()` eintragen.
- Die Pfadregel in `sdcard.art_path` entspricht `DsRunnerFileInfoCardView::RequestArt`, die Prüfung in `imaging.validate_bmp` entspricht `DsRunnerTopArt::Load`. Ändert sich eins davon im Launcher, müssen Tool und Tests mitziehen.
- Release: Der Workflow `release.yml` baut `DsRunnerArt.exe` mit PyInstaller auf `windows-latest` und hängt sie an das Release.
