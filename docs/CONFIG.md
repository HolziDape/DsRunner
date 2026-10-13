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
  "showClock": true,
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
| `showClock` | `true` (Standard): Uhrzeit oben rechts in der Statusleiste. `false`: nur die Akkuanzeige. Umschalten im Menü (Zahnrad → Uhr). Gilt nur im DsRunner-Theme. |
| `language` | Stammt von Pico Launcher und wird nicht ausgewertet. Die Oberfläche von DsRunner ist immer deutsch (siehe [Sprache](#sprache)). |
| `fileAssociations` | Programm, das Dateien mit einer Endung öffnet (z.B. `gba`, `nes`). Der Dateipfad wird per argv übergeben. Hat Vorrang vor `systems.json` und den Emulatoren, die DsRunner selbst findet (siehe [Emulatoren](#emulatoren)). |

## Tasten

| Taste | Funktion |
|---|---|
| Steuerkreuz | Auswahl bewegen |
| A | Konsole öffnen / Spiel starten |
| B | zurück (auf der Konsolen-Ebene ohne Wirkung) |
| L / R | eine Seite weiterblättern |
| X | ausgewähltes Spiel zu den [Favoriten](#favoriten) hinzufügen oder entfernen |
| Y | Cheats für das ausgewählte NDS-Spiel |
| START | auf der Konsolen-Ebene: [Weiterspielen](#weiterspielen) |
| Zahnrad (oben rechts) | Menü „Anzeige“: Ansicht, Sortierung, Uhr ein/aus, Theme |

## Ordnerstruktur der Bibliothek

```
/Games
  /NDS     Nintendo-DS-Spiele (.nds)
  /GBA     Game-Boy-Advance-Spiele (.gba), brauchen GBARunner2
  /GB      Game Boy (.gb), /GBC Game Boy Color (.gbc)
  /NES     NES (.nes, .fds), /SNES Super Nintendo (.sfc, .smc)
  /SMS     Master System (.sms), /GG Game Gear (.gg)
  /MD      Mega Drive (.md, .gen, .smd), /PCE PC Engine (.pce, .sgx)
  /Apps    Homebrew
  /...     weitere Ordner werden ebenfalls als Konsole angezeigt
```

Konsolen mit eigener Farbe und eigenem Kürzel: `NDS` (auch `DS`), `GBA`, `GBC`, `GB`, `NES` (auch `FC`), `SNES` (auch `SFC`), `SMS`, `GG`, `MD` (auch `GEN`, `MEGADRIVE`), `PCE`, `Apps` (auch `Homebrew`). Die Namen werden ohne Beachtung der Groß-/Kleinschreibung verglichen. Jeder andere Ordner bekommt eine neutrale Kachel mit den ersten vier Buchstaben, außer er steht in der [`systems.json`](#eigene-konsolen-systemsjson).

Spiele anderer Konsolen als DS werden nur angezeigt, wenn es ein Programm (einen [Emulator](#emulatoren)) für sie gibt. Ohne eigenes Icon bekommen sie die Kachel ihrer Konsole (Farbe und Kürzel), bis DsRunner Art ein [Kachel-Bild](#kachel-bilder) für sie anlegt.

## Emulatoren

DsRunner startet Spiele anderer Konsolen über Emulatoren für den DS (Homebrew). Der Pfad des Spiels wird dabei wie bei Picos Dateizuordnungen als Argument übergeben (argv), der Emulator startet das Spiel also direkt, ohne eigene Dateiauswahl.

**Welches Programm eine Endung öffnet**, in dieser Reihenfolge:

1. `fileAssociations` in `/_pico/settings.json` (siehe [Einstellungen](#einstellungen))
2. `emulator` der Konsole in [`/_dsrunner/systems.json`](#eigene-konsolen-systemsjson)
3. ein Emulator aus der Tabelle unten, den DsRunner beim Start auf der Karte findet

Gibt es für eine Endung nichts davon, zeigt DsRunner die Dateien nicht an (der Ordner bleibt leer, „0 Spiele“).

**Gefundene Emulatoren** (pro Konsole gilt der erste gefundene, von oben nach unten):

| Konsole | Endungen | Emulator | Dateiname | Spiel per argv | Download |
|---|---|---|---|---|---|
| Game Boy Advance | `.gba`, `.agb` | GBARunner2 | `GBARunner2_arm7dldi_ds.nds`, `GBARunner2.nds`, `GBARunner2_arm9dldi_ds.nds` | ja | [GBARunner2](https://github.com/Gericom/GBARunner2/releases), für den DS Lite die Datei `GBARunner2_arm7dldi_ds.nds` |
| NES | `.nes`, `.fds` | nesDS | `nesDS.nds` | ja | [nesDS](https://github.com/DS-Homebrew/NesDS/releases) |
| Game Boy, Game Boy Color | `.gb`, `.sgb`, `.gbc` | GameYob | `GameYob.nds` | ja | [GameYob](https://github.com/Drenn1/GameYob/releases) oder aus TWiLight Menu++ (siehe unten) |
| Game Boy, Game Boy Color | `.gb`, `.sgb`, `.gbc` | lameboy (nur ohne GameYob) | `lameboy.nds` | nicht dokumentiert, startet evtl. mit eigener Dateiauswahl | [lameboy](https://lameboy.nutki.com/) |
| Master System, Game Gear | `.sms`, `.gg` | S8DS | `S8DS.nds` | ja | [S8DS](https://github.com/FluBBaOfWard/S8DS/releases) (`S8DS.nds` aus der ZIP-Datei) |
| Mega Drive | `.md`, `.gen`, `.smd` | jEnesisDS | `jEnesisDS.nds` | ja (Version aus TWiLight Menu++) | aus TWiLight Menu++ (siehe unten) |
| Super Nintendo | `.sfc`, `.smc` | SNEmulDS | `SNEmulDS-legacy.nds`, `SNEmulDS.nds` | `SNEmulDS-legacy.nds` ja. Das neue `SNEmulDS.nds` (TGDS) startet TWiLight auf eigene Art, mit argv ungetestet | `SNEmulDS-legacy.nds` aus TWiLight Menu++, neue Version: [snemulds](https://github.com/cotodevel/snemulds) |
| PC Engine | `.pce`, `.sgx` | NitroGrafx | `NitroGrafx.nds` | ja | [NitroGrafx](https://github.com/FluBBaOfWard/NitroGrafx/releases) (`NitroGrafx.nds` aus der ZIP-Datei) |

**Wohin damit:** DsRunner sucht in diesen Ordnern, in dieser Reihenfolge (Groß-/Kleinschreibung egal):

1. `/_dsrunner/emulators/` (empfohlen)
2. `/_dsrunner/`
3. `/` (Hauptverzeichnis der Karte)
4. `/_nds/`
5. `/_nds/TWiLightMenu/emulators/` (dort legt [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases) seine Emulatoren ab, z. B. `nesds.nds`, `gameyob.nds`, `S8DS.nds`, `jEnesisDS.nds`, `SNEmulDS-legacy.nds`, `NitroGrafx.nds`)
6. `/Emulators/`

Jeder Ordner wird beim Start einmal gelesen. Andere Dateinamen (z. B. `nesDS_1.3.nds`) findet DsRunner nicht, dafür die Datei umbenennen oder den Pfad in `systems.json` bzw. `fileAssociations` eintragen. Gefundene Emulatoren schreibt DsRunner nicht in die `settings.json`. Eine dort schon stehende `gba`-Zuordnung (frühere Versionen haben GBARunner2 dort eingetragen) gilt weiter und hat Vorrang.

**Hinweise zu den Emulatoren:**

- **GBARunner2** braucht das GBA-BIOS (`bios.bin`) neben GBARunner2 bzw. unter `/_gba/bios.bin`, wie in seiner Anleitung.
- **nesDS:** `.fds`-Spiele (Famicom Disk System) brauchen zusätzlich das FDS-BIOS, siehe Anleitung von nesDS.
- **jEnesisDS** spielt Mega-Drive-Spiele bis 3 MB. TWiLight Menu++ startet es nur mit `.gen`-Dateien. Lädt ein `.md`-Spiel nicht, die Datei in `.gen` umbenennen.
- **lameboy** und andere Emulatoren ohne argv starten mit ihrer eigenen Dateiauswahl, das Spiel muss man dann dort noch einmal wählen.
- Picos `.md`-Zuordnung gilt für die ganze Karte: Liegt eine `README.md` in einem Ordner, erscheint sie dort als Mega-Drive-Spiel, sobald jEnesisDS gefunden wurde.

## Eigene Konsolen (systems.json)

`/_dsrunner/systems.json` ist optional. Damit lassen sich weitere Konsolen anlegen (z. B. WonderSwan mit NitroSwan) und die eingebauten ändern (Farbe, Kürzel, eigener Emulator). Ohne die Datei gilt die eingebaute Liste.

```json
{
  "systems": {
    "WS": {
      "name": "WonderSwan",
      "label": "WS",
      "color": "#C08020",
      "extensions": ["ws", "wsc"],
      "emulator": "/_dsrunner/emulators/NitroSwan.nds",
      "libretro": "Bandai - WonderSwan"
    },
    "SNES": {
      "emulator": "/Emulators/SNEmulDS-legacy.nds"
    },
    "SMS": {
      "label": "MS",
      "color": [224, 64, 128]
    }
  }
}
```

Jeder Schlüssel unter `systems` ist ein **Ordnername** der Bibliothek (Groß-/Kleinschreibung egal). Alle Felder sind optional:

| Feld | Bedeutung | Ohne Angabe |
|---|---|---|
| `name` | Name der Konsole oben auf dem Bildschirm | eingebaute Konsole: ihr Name, sonst der Ordnername |
| `label` | Kürzel auf der Kachel (höchstens 4 Zeichen werden gezeigt) | eingebaute Konsole: ihr Kürzel, sonst der Ordnername |
| `color` | Farbe der Kachel: `"#RRGGBB"` oder `[R, G, B]` (0 bis 255). Der Verlauf oben wird daraus etwas dunkler berechnet. | eingebaute Farbe bzw. das neutrale Grau-Blau |
| `heroColor` | eigene Farbe für den Verlauf oben, gleiches Format | aus `color` berechnet |
| `extensions` | Dateiendungen dieser Konsole, höchstens 4, ohne Punkt (`"ws"`) | eingebaute Konsole: ihre Endungen, sonst keine |
| `emulator` | Pfad des Emulators für diese Endungen, das Spiel wird per argv übergeben | der gefundene Emulator (siehe [Emulatoren](#emulatoren)) |
| `libretro` | nur für [DsRunner Art](ART-TOOL.md): libretro-System für die Artworks (z. B. `"Bandai - WonderSwan"`), auch als Liste | aus der Endung, siehe ART-TOOL.md |

Regeln:

- Eine Endung, die mehrere Konsolen nennen, gehört zur ersten. Farbe und Kürzel einer Datei richten sich nach ihrer Endung (`systems.json` zuerst, dann die eingebaute Liste).
- Existiert der `emulator`-Pfad nicht, wird er ignoriert (es gilt dann ein gefundener Emulator, sonst bleiben die Dateien unsichtbar).
- `.nds`-Dateien startet immer Pico Loader, ein Emulator für `nds` wird ignoriert.
- Höchstens 16 Konsolen und 8 KB Dateigröße. Ist die Datei kein gültiges JSON oder fehlt `systems`, ignoriert DsRunner sie ganz und nutzt die eingebaute Liste. DsRunner Art meldet solche Fehler beim Scannen im Protokoll, ebenso fehlende Emulatoren.
- DsRunner liest die Datei einmal beim Start. Nach Änderungen DsRunner neu starten.
- Texte (`name`, `label`) ohne `–` (Gedankenstrich) schreiben, die Schrift kennt ihn nicht.

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

## Favoriten

**X** fügt das ausgewählte Spiel zu den Favoriten hinzu oder nimmt es wieder heraus (in allen Ansichten). Favoriten bekommen in den Kachelrastern einen goldenen Stern in der rechten oberen Ecke, auf dem oberen Bildschirm steht ein ★ vor dem Dateinamen. Ordner können keine Favoriten sein.

Sobald es mindestens einen Favoriten gibt, steht auf der Konsolen-Ebene als erste Kachel **★ Favoriten** (gold, mit der Anzahl auf dem oberen Bildschirm). A öffnet die Liste aller Favoriten aus allen Ordnern, B führt zurück zur Konsolen-Ebene. Spiele starten von dort wie aus ihrem eigenen Ordner (Spielverlauf, Cheats, GBARunner2). In der Liste sind die Favoriten nach Namen sortiert (A–Z bzw. Z–A, „zuletzt gespielt“ sortiert hier ebenfalls nach Namen). Wer in der Liste einen Favoriten mit X entfernt, sieht ihn dort bis zum nächsten Öffnen weiter (ohne Stern). Mit leerem `libraryPath` gibt es keine Konsolen-Ebene und damit auch keine Favoriten-Kachel, X und der Stern funktionieren trotzdem.

`/_dsrunner/favorites.txt` hält die Favoriten in der Reihenfolge, in der sie hinzugefügt wurden, eine pro Zeile (UTF-8, höchstens 200 Einträge), im selben Format wie der [Spielverlauf](#spielverlauf):

```
/Games/NDS/Mario Kart DS.nds
/Games/GBA/Golden Sun.gba
```

Die Datei lässt sich auch am PC bearbeiten. Einträge, deren Datei es nicht mehr gibt, erscheinen in der Liste nicht und bleiben in der Datei stehen, bis man sie dort löscht.

## Weiterspielen

Auf der Konsolen-Ebene startet **START** das zuletzt gespielte Spiel, ohne es erst suchen zu müssen. Welches Spiel das ist, steht auf dem oberen Bildschirm in der untersten Zeile („START: Weiterspielen (Cars GER)“). DsRunner öffnet dafür kurz den Ordner des Spiels und startet es dann wie mit A, also über Pico Loader bzw. die Dateizuordnung.

Das Spiel kommt aus dem [Spielverlauf](#spielverlauf) (der neueste der ersten drei Einträge, den es noch gibt), sonst aus `lastUsedFilePath`. Gibt es keins, fehlt der Hinweis und START bleibt ohne Wirkung.

## Sprache

Die Oberfläche von DsRunner ist deutsch. Die Schriften (NotoSansJP) enthalten ä, ö, ü, Ä, Ö, Ü und ß, Umlaute in Ordner- und Dateinamen werden richtig angezeigt. Picos Einstellung `language` gibt es weiterhin in der `settings.json`, sie wird aber (wie in Pico Launcher) nicht ausgewertet. Eine englische Oberfläche ist nicht vorgesehen. Texte, die aus Dateien kommen (Spieltitel aus dem NDS-Banner, Theme-Namen aus `theme.json`, Cheat-Namen aus `usrcheat.dat`), erscheinen so, wie sie dort stehen.

## Hintergrundmusik

Pico Launcher spielt Hintergrundmusik pro Theme, DsRunner übernimmt das unverändert:

- Musik als **`.bcstm`** mit **DSP-ADPCM**-Kodierung in den Ordner `bgm` des Themes legen, für das DsRunner-Theme also `/_pico/themes/dsrunner/bgm/`, z. B. `/_pico/themes/dsrunner/bgm/menu.bcstm`.
- Schleifen (Loop-Punkte in der Datei) werden unterstützt. Liegen mehrere Dateien im Ordner, wählt DsRunner bei jedem Start zufällig eine aus. Deshalb nur `.bcstm`-Dateien in den Ordner legen, andere Dateien werden nicht aussortiert.
- Mono und Stereo gehen (von mehr Kanälen werden nur die ersten zwei gespielt).
- Umwandeln am PC z. B. mit [LoopingAudioConverter](https://github.com/libertyernie/LoopingAudioConverter) (Ausgabe „BCSTM“, Kodierung „DSP-ADPCM“). Mono und eine niedrige Abtastrate halten die Datei klein.
- Ohne `bgm`-Ordner bleibt DsRunner still. Zum Abschalten den Ordner löschen oder umbenennen.

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

## Kachel-Bilder

Im Kachelraster zeigt ein Spiel statt seines Icons (32 × 32, bei GBA-Spielen gar keins) ein **Kachel-Bild**, wenn es eins gibt. Sonst bleibt es beim Icon.

**Ablage:** gleiche Regel wie bei den [Artworks](#artworks), nur mit `/_dsrunner/tiles` statt `/_dsrunner/art`, und **nur für Spiele** (Dateien), nicht für Ordner:

| Spiel | Kachel-Bild |
|---|---|
| `/Games/NDS/Mario Kart DS.nds` | `/_dsrunner/tiles/Games/NDS/Mario Kart DS.bmp` |
| `/Games/GBA/Rennen/Golden Sun.gba` | `/_dsrunner/tiles/Games/GBA/Rennen/Golden Sun.bmp` |

Regel: Pfad ab SD-Root, Endung durch `.bmp` ersetzen (nur die letzte, `Platin.v1.2.nds` → `Platin.v1.2.bmp`). Der ganze Pfad muss wie bei den Artworks in 255 Bytes (UTF-8) passen. Groß-/Kleinschreibung spielt keine Rolle (FAT).

**Format:**

- Windows-BMP, genau **64 × 64 Pixel**, **8 bit pro Pixel** mit Palette, unkomprimiert (`BI_RGB`), Zeilen von unten nach oben oder von oben nach unten (negative Höhe). Die Dateien des Tools sind 5174 Bytes groß.
- Höchstens 255 Farben, **Palettenfarbe 0 bleibt unbenutzt** (der DS behandelt sie als durchsichtig). Farben werden auf 15 bit gekürzt.
- Sonst gelten dieselben Prüfregeln wie bei den Artworks, nur die Größe ist anders. Andere Dateien werden ignoriert (dann das Icon).

**Inhalt** (erzeugt von [DsRunner Art](ART-TOOL.md#kachel-bilder)): das Cover des Spiels, auf 64 × 64 gefüllt und mittig beschnitten (beim DS- und GBA-Cover ohne den Streifen „NINTENDO DS“ bzw. „GAME BOY ADVANCE“). Ersatz der Reihe nach: Titelbild (oberer Bildschirm), Screenshot, zuletzt das Icon des Spiels doppelt groß (ohne Glätten) auf dunklem Grund.

**DsRunner Art** führt die Liste der eigenen Kachel-Bilder in `/_dsrunner/tiles/.dsrunner-art.json` und liest eigene Kachel-Bilder aus `/_dsrunner/tile-override/` (gleicher Pfad wie das Spiel oder nur der Name, beliebiges Bildformat). DsRunner selbst nutzt beides nicht.

