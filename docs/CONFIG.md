# Einstellungen

DsRunner liest `/_pico/settings.json` auf der SD-Karte (das Format kommt von Pico Launcher und lässt sich mit jedem Texteditor bearbeiten). Die Datei wird beim ersten Start angelegt und von DsRunner geschrieben, wenn sich etwas ändert.

```json
{
  "language": "english",
  "romBrowserLayout": "TileGrid3x2",
  "romBrowserSortMode": "LastPlayed",
  "theme": "dsrunner",
  "lastUsedFilePath": "/Games/NDS/Cars GER.nds",
  "libraryPath": "/Games",
  "fileAssociations": {
    "gba": { "appPath": "/GBARunner2_arm7dldi_ds.nds" }
  }
}
```

| Schlüssel | Bedeutung |
|---|---|
| `theme` | Ordnername in `/_pico/themes`. `dsrunner` ist das dunkle DsRunner-Theme. Mit `material` oder `raspberry` kommen Picos Themes zurück. |
| `libraryPath` | Ordner, dessen Unterordner die Konsolen sind. DsRunner startet immer hier. Standard `/Games`. Leer lassen (`""`) für das Verhalten von Pico Launcher (letzter Ordner). Existiert der Ordner nicht, startet DsRunner im SD-Root. |
| `romBrowserLayout` | `TileGrid3x2` (Standard: große Kacheln, 3 nebeneinander, 2 untereinander), `TileGrid4x3` (4 × 3 kleinere Kacheln), dazu Picos Ansichten `HorizontalIconGrid`, `VerticalIconGrid`, `BannerList`, `CoverFlow`. Umschalten im Menü (Zahnrad → Layout), die Wahl wird hier gespeichert. Eine bestehende `settings.json` behält ihre Ansicht, bis man im Menü umschaltet. |
| `romBrowserSortMode` | `NameAscending` (A–Z), `NameDescending` (Z–A), `LastPlayed` (zuletzt gespielt zuerst, siehe [Spielverlauf](#spielverlauf)). Ordner stehen immer vorne und sind nach Namen sortiert. `LastModified` von Pico ist nicht umgesetzt und sortiert wie A–Z. |
| `lastUsedFilePath` | Zuletzt gestartetes Spiel, wird automatisch gesetzt. |
| `fileAssociations` | Programm, das Dateien mit einer Endung öffnet (z.B. `gba`, `nes`). Der Dateipfad wird per argv übergeben. |

## Ordnerstruktur der Bibliothek

```
/Games
  /NDS     Nintendo-DS-Spiele (.nds)
  /GBA     Game-Boy-Advance-Spiele (.gba), brauchen GBARunner2
  /Apps    Homebrew
  /...     weitere Ordner werden ebenfalls als Konsole angezeigt
```

Konsolen mit eigener Farbe und eigenem Kürzel: `NDS`, `GBA`, `GBC`, `GB`, `NES`, `SNES`, `SMS`, `GG`, `MD`, `PCE`, `Apps`. Die Namen werden ohne Beachtung der Groß-/Kleinschreibung verglichen. Jeder andere Ordner bekommt eine neutrale Kachel mit den ersten vier Buchstaben.

## GBA-Spiele

`.gba`-Dateien werden nur angezeigt, wenn ein Programm dafür eingetragen ist. Ist in `fileAssociations` kein `gba`-Eintrag, sucht DsRunner GBARunner2 selbst, in dieser Reihenfolge:

1. `/_dsrunner/GBARunner2.nds`
2. `/GBARunner2_arm7dldi_ds.nds`
3. `/GBARunner2.nds`
4. `/_nds/GBARunner2_arm7dldi_ds.nds`
5. `/_nds/GBARunner2.nds`

Das GBA-BIOS (`bios.bin`) gehört neben GBARunner2 bzw. nach `/_gba/bios.bin`, wie in der Anleitung von GBARunner2.

## Kachelraster

- Die Kacheln füllen sich **zeilenweise** (Lesereihenfolge) und das Raster blättert nach unten. Steuerkreuz bewegt die Auswahl, **L/R** blättern eine Seite (2 bzw. 3 Reihen).
- Konsolen-Ordner bekommen die Farbe der Konsole, Spiele zeigen ihr Icon (im 3×2-Raster doppelt so groß).
- In diesen Ansichten zeigt der obere Bildschirm statt Picos kleinem Cover das [Artwork](#artworks).
- Auf der Konsolen-Ebene steht unter dem Namen der Konsole die **Spielanzahl** („34 Spiele“). DsRunner zählt dafür nach dem Start einmal im Hintergrund die Dateien jedes Konsolen-Ordners, die es auch anzeigen würde (bekannte Dateitypen, keine Unterordner, keine versteckten Dateien). Solange gezählt wird, bleibt die Zeile leer.

## Spielverlauf

`/_dsrunner/recent.txt` hält die zuletzt gestarteten Dateien, die neueste zuerst, eine pro Zeile (UTF-8, höchstens 100 Einträge):

```
/Games/NDS/Mario Kart DS.nds
/Games/GBA/Golden Sun.gba
```

DsRunner schreibt die Datei bei jedem Start eines Spiels. Ein Laufwerk vor dem Pfad (`fat:/Games/...`) wird ignoriert. Die Sortierung `LastPlayed` nutzt die Reihenfolge; Spiele, die nicht in der Liste stehen, folgen danach von A bis Z.

## Artworks

Der obere Bildschirm zeigt ein bildschirmfüllendes Artwork für die ausgewählte Konsole bzw. das ausgewählte Spiel. Fehlt es, bleibt der Farbverlauf in der Konsolenfarbe. Beim Wechsel der Auswahl blendet das alte Bild in den Farbverlauf aus und das neue ein. Der untere Teil (ab Zeile 92, ab Zeile 118 voll) wird abgedunkelt, damit Titel und Untertitel lesbar bleiben.

**Ablage:** Die Artworks spiegeln den Pfad auf der SD-Karte unter `/_dsrunner/art`:

| Eintrag | Artwork |
|---|---|
| Konsolen-Ordner `/Games/NDS` | `/_dsrunner/art/Games/NDS.bmp` |
| Spiel `/Games/NDS/Mario Kart DS.nds` | `/_dsrunner/art/Games/NDS/Mario Kart DS.bmp` |
| Spiel `/Games/GBA/Rennen/Golden Sun.gba` | `/_dsrunner/art/Games/GBA/Rennen/Golden Sun.bmp` |

Regel: Pfad ab SD-Root, bei Dateien die Endung durch `.bmp` ersetzen, bei Ordnern `.bmp` anhängen. Groß-/Kleinschreibung spielt keine Rolle (FAT).

**Format** (das PC-Tool [DsRunner Art](ART-TOOL.md) erzeugt die Bilder automatisch):

- Windows-BMP, **256 × 192 Pixel**, **8 bit pro Pixel** mit Palette (bis 256 Farben), unkomprimiert (`BI_RGB`). Zeilen von unten nach oben (Standard) oder von oben nach unten (negative Höhe) gehen beide. Dateigröße ≈ 50 KB.
- Farben werden auf 15 bit (5 bit pro Kanal) gekürzt. Gutes Ergebnis: auf 255 Farben mit Dithering reduzieren.
- Palettenfarbe 0 ist beim DS durchsichtig. DsRunner zeichnet Pixel mit Farbe 0 deshalb mit der ähnlichsten anderen Farbe (oder einem freien Paletteneintrag). Das Tool kann Farbe 0 einfach ungenutzt lassen.
- Andere Größen, Farbtiefen oder komprimierte Dateien werden ignoriert (dann bleibt der Farbverlauf).
- Mit Python/Pillow: `img.convert("RGB").resize((256, 192)).quantize(255, dither=Image.Dither.FLOYDSTEINBERG).save("x.bmp")`

**Gestaltung:** Text liegt unten (Titel ab Zeile 122). Wichtige Bildteile gehören in die oberen ~90 Zeilen. Rechts oben (Zeilen 4–20) liegt die Statusleiste mit Uhr und Akku.

**DsRunner Art** legt außerdem `/_dsrunner/art/.dsrunner-art.json` an (welche Bilder vom Tool stammen) und liest eigene Bilder aus `/_dsrunner/art-override/` (gleicher Pfad wie das Spiel, beliebiges Bildformat). DsRunner selbst nutzt beides nicht.

