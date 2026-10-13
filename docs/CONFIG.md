# Einstellungen

DsRunner liest `/_pico/settings.json` auf der SD-Karte (das Format kommt von Pico Launcher und lässt sich mit jedem Texteditor bearbeiten). Die Datei wird beim ersten Start angelegt und von DsRunner geschrieben, wenn sich etwas ändert.

```json
{
  "language": "english",
  "romBrowserLayout": "HorizontalIconGrid",
  "romBrowserSortMode": "NameAscending",
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
| `romBrowserLayout` | `HorizontalIconGrid` (Standard), `VerticalIconGrid`, `BannerList`, `FileList`, `CoverFlow`. Umschalten geht auch im Menü (Zahnrad). |
| `romBrowserSortMode` | `NameAscending`, `NameDescending`, `LastModified` |
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
