# Changelog

Alle wichtigen Änderungen an DsRunner und am PC-Programm DsRunner Art. Das Format folgt [Keep a Changelog](https://keepachangelog.com/de/1.1.0/), die Versionsnummern folgen [Semantic Versioning](https://semver.org/lang/de/). Vor Version 1.0 steht eine neue mittlere Zahl (0.**2**.0) für neue Funktionen, eine neue letzte Zahl (0.1.**2**) für Fehlerbehebungen und kleine Verbesserungen.

Downloads: [Releases](https://github.com/HolziDape/DsRunner/releases). Wie ein Release entsteht: [docs/RELEASE.md](docs/RELEASE.md).

## Unveröffentlicht

### Hinzugefügt
- **Eigene Bilder für die Konsolen-Ordner, schon eingebaut:** Jeder Ordner auf der Konsolen-Ebene (NDS, GBA, GB, GBC, NES, SNES, Master System, Game Gear, Mega Drive, PC Engine, Apps) und die Kachel ★ Favoriten zeigen eine gezeichnete Konsole bzw. einen Stern in der Konsolenfarbe, mit dem Kürzel darunter. Oben gibt es dazu ein passendes Hintergrundbild, im Coverflow ein passendes Cover. Eigene Konsolen aus `systems.json` bekommen ein Modul, andere Ordner einen Ordner, jeweils in ihrer Farbe. Die Bilder liegen im Theme (`/_pico/themes/dsrunner/consoles`) und funktionieren ohne DsRunner Art. Eigene Bilder in `/_dsrunner/art` und `/_dsrunner/tiles` haben weiter Vorrang, siehe [docs/CONFIG.md](docs/CONFIG.md#eingebaute-konsolen-bilder).
- **Zurück zu DsRunner aus dem Spiel:** im laufenden DS-Spiel **L + R + Unten + B zwei Sekunden halten**, dann startet der DS neu in DsRunner. Vorher speichern, nicht Gespeichertes geht verloren; ein laufender Speichervorgang wird nicht abgebrochen. Klappt nicht in jedem Spiel (dann läuft das Spiel normal weiter, nur ohne Hotkey). Noch nicht auf der echten M3 DS Real getestet.
- Einstellung `inGameReset` in `/_pico/settings.json` (Standard `true`): mit `false` baut Pico Loader den Hotkey nicht ein.
- Das Paket für die M3 DS Real enthält in `_pico/picoLoader-DsRunner.txt`, welcher Pico-Loader-Build drin ist.
- **Emulatoren im Paket:** `DsRunner_M3DSReal.zip` und `DsRunner.zip` bringen in `/_dsrunner/emulators/` die Emulatoren für GBA (GBARunner2), NES (nesDS 2.1), Game Boy / Game Boy Color (GameYob) und SNES (SNEmulDS 0.6, dazu `/snemul.cfg`) mit, samt Lizenzen in `LIZENZEN.txt`. Diese Konsolen funktionieren damit ohne weitere Downloads. Das GBA-BIOS ist nicht dabei (optional selbst als `/_gba/bios.bin` ablegen), ebenso wenig S8DS, NitroGrafx und jEnesisDS: Ihre Autoren nennen keine Lizenz, die die Weitergabe erlaubt. Wer schon installiert hat, kopiert einmal den Ordner `_dsrunner/emulators` und `snemul.cfg` aus dem Paket, siehe [README](README.md#emulatoren-für-andere-konsolen).

### Geändert
- **Pico Loader geändert:** Das M3-Paket bringt jetzt einen eigenen Build von Pico Loader mit (Stand v1.7.1 plus aktuelle Fixes aus der Entwicklung von Pico, dazu die Rückkehr aus Spielen von danyocom und kleine DsRunner-Patches, siehe [loader/README.md](loader/README.md)). Beim Update deshalb auch `picoLoader7.bin`, `picoLoader9.bin`, `aplist.bin`, `patchlist.bin` und `savelist.bin` nach `/_pico` kopieren. Pico legt neu `/_pico/dldi.bin` und `/_pico/launcher.txt` an.
- Fehlt beim Start der eigene Pfad (`argv[0]`), merkt sich DsRunner `/_nds/TWiLightMenu/main.srldr`, `/BOOT.NDS` oder `/DsRunner.nds` als Rückkehrziel, je nachdem, was auf der Karte liegt. Vorher kehrten Homebrew-Programme dann nicht zurück.
- DsRunner Art erzeugt keine Ordner-Bilder mehr, wenn das Theme auf der Karte eingebaute Konsolen-Bilder hat, und löscht seine früher erzeugten Ordner-Bilder (Farbverlauf mit Kürzel), damit die neuen sichtbar werden. Selbst abgelegte Bilder bleiben.

## [0.2.0](https://github.com/HolziDape/DsRunner/releases/tag/v0.2.0) - 2026-10-14

### Hinzugefügt
- **Jede Ansicht mit eigenem Bildstil:** 3×2 zeigt vereinfachte Kachel-Bilder, 4×3 klare kleine Icons in Originalgröße, Coverflow die echten Cover. DsRunner Art legt dafür zusätzlich Cover (`/_pico/covers/user`) und Icons (`/_pico/icons/user`) an; abschaltbar mit „Cover“ und „Icons“ bzw. `--no-covers` und `--no-icons`.
- **Weitere Konsolen:** DsRunner findet die Emulatoren nesDS (NES), GameYob oder lameboy (GB/GBC), S8DS (Master System, Game Gear), jEnesisDS (Mega Drive), SNEmulDS (SNES) und NitroGrafx (PC Engine) selbst, wie bisher GBARunner2. Anleitung: [docs/CONFIG.md](docs/CONFIG.md#emulatoren).
- **Eigene Konsolen** über `/_dsrunner/systems.json`: Name, Kürzel, Farbe, Dateiendungen und Emulator pro Ordner. DsRunner Art liest die Datei ebenfalls.
- Versionsnummer unten rechts im Menü „Anzeige“ (Zahnrad oben rechts) und in der Titelzeile des Banners. Selbst gebaute Zwischenstände tragen den Zusatz `-dev`.
- DsRunner Art trägt dieselbe Versionsnummer wie DsRunner (Fenstertitel und `--version`).
- Die Release-Seite zeigt die Änderungen aus diesem Changelog.

### Geändert
- Im Menü „Anzeige“ gibt es nur noch die Ansichten 3×2, 4×3 und Coverflow. Wer eine der entfernten Ansichten eingestellt hatte, startet im 3×2-Raster.
- Kachel-Bilder im 3×2-Raster: weniger Farben, ruhiger, näher an Logo und Hauptfigur, bei GBA-Covern ohne den silbernen Rand. DsRunner Art erneuert vorhandene Kachel-Bilder einmal automatisch.
- Ein gefundener Emulator wird nicht mehr fest in die `settings.json` geschrieben.
- Ausführlichere `LIESMICH.txt` im Paket für die M3 DS Real: mit Version, Ordnern für die Spiele, Update-Anleitung und Hinweis auf DsRunner Art.

## [0.1.2](https://github.com/HolziDape/DsRunner/releases/tag/v0.1.2) - 2026-10-13

### Hinzugefügt
- Die Kacheln zeigen das Cover des Spiels statt des kleinen Icons, auch bei GBA-Spielen. Die Kachel-Bilder legt DsRunner Art in `/_dsrunner/tiles` an. Ohne Kachel-Bild bleibt das Icon.
- DsRunner Art: Option „Kachel-Bilder erzeugen“ (Kommandozeile: `--no-tiles`, `--tile-override`).
- DsRunner Art: Option „Auch von Hand abgelegte Bilder ersetzen“ (`--replace-manual`). Am Ende steht, wie viele eigene Bilder behalten wurden und welche der Launcher nicht anzeigen kann.

### Geändert
- Weicheres Abdunkeln des Artworks hinter dem Text auf dem oberen Bildschirm.
- Beim schnellen Blättern lädt DsRunner erst ein Artwork, wenn die Auswahl kurz stehen bleibt.
- DsRunner Art: Fehlt ein Titelbild, nimmt es das Cover statt eines Screenshots. Bei DS-Bildern mit beiden Bildschirmen wird der obere verwendet, bei DS-Covern fällt der Rand mit dem Schriftzug weg.

### Behoben
- DsRunner konnte nach dem Wechsel zwischen Ordnern hängen bleiben (gestreifter oberer Bildschirm, falsche Farben).
- DsRunner Art: Ein abgebrochener Download oder ein Fehler beim Umwandeln ließ ein Spiel ohne Bild, jetzt wird das nächste Bild oder das Icon versucht. Spiele ohne Bild werden mit Grund aufgelistet.
- DsRunner Art: Nach einem abgebrochenen Lauf hielt das Programm seine eigenen Bilder für von Hand abgelegte und aktualisierte sie nie wieder.

## [0.1.1](https://github.com/HolziDape/DsRunner/releases/tag/v0.1.1) - 2026-10-13

### Hinzugefügt
- PC-Programm **DsRunner Art** (`DsRunnerArt.exe` für Windows): lädt die großen Bilder für den oberen Bildschirm automatisch auf die SD-Karte. Es erkennt die Spiele am Game-Code (DS, GBA), an der Prüfsumme oder am Dateinamen, lädt Titelbilder, Screenshots oder Cover von libretro-thumbnails und erzeugt Bilder für die Konsolen-Ordner. Eigene Bilder haben Vorrang. Anleitung: [docs/ART-TOOL.md](docs/ART-TOOL.md).
- **Favoriten:** X fügt das ausgewählte Spiel hinzu oder entfernt es. Favoriten tragen einen goldenen Stern und erscheinen gesammelt in der Kachel „★ Favoriten“ auf der Konsolen-Ebene.
- **Weiterspielen:** START auf der Konsolen-Ebene startet das zuletzt gespielte Spiel.
- Uhr in der Statusleiste im Menü „Anzeige“ ein- und ausschaltbar.

### Geändert
- Oberfläche auf Deutsch: Menü „Anzeige“, Cheats und Theme-Auswahl.

## [0.1.0](https://github.com/HolziDape/DsRunner/releases/tag/v0.1.0) - 2026-10-13

Erste Version von DsRunner, einem Fork von [Pico Launcher](https://github.com/LNH-team/pico-launcher) für den DS Lite mit der M3 DS Real.

### Hinzugefügt
- **Autostart auf der M3 DS Real:** fertiges Paket `DsRunner_M3DSReal.zip` für die SD-Karte mit Pico Loader und den Autostart-Dateien aus TWiLight Menu++. DsRunner startet beim Einschalten direkt.
- Eigener Name, eigenes Icon und Banner „DsRunner“.
- Dunkles DsRunner-Design mit Statusleiste (Uhr und Akku, ein schwacher Akku erscheint als kurzer roter Balken).
- **Konsolen-Ebene:** Die Unterordner von `/Games` erscheinen als Konsolen, bekannte Konsolen (NDS, GBA, GBC, GB, NES, SNES, SMS, GG, MD, PCE, Apps) in eigenen Farben. DsRunner startet immer dort und wählt die Konsole des zuletzt gespielten Spiels vor. Der Ordner ist über `libraryPath` in `/_pico/settings.json` einstellbar.
- GBA-Spiele starten automatisch über GBARunner2, wenn es auf der Karte liegt.
- **Kachel-Raster** 3×2 (Standard) und 4×3, mit Leuchtrand um die Auswahl.
- Spielanzahl pro Konsole auf dem oberen Bildschirm.
- Sortierung „zuletzt gespielt“ (Spielverlauf in `/_dsrunner/recent.txt`).
- **Großes Artwork** pro Konsole und Spiel auf dem oberen Bildschirm (`/_dsrunner/art`, BMP mit 256 × 192 Pixeln und 256 Farben), mit weichem Überblenden.
- Aus Pico Launcher übernommen: Spiele über Pico Loader starten, Cheats, Touch-Bedienung, Theme-Auswahl, eigene Icons, Banner und Ordner-Cover. Die Änderungen von Pico Launcher bis zum Fork stehen in [docs/PicoLauncher-CHANGELOG.md](docs/PicoLauncher-CHANGELOG.md).
