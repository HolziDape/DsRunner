# Changelog

Alle wichtigen Änderungen an DsRunner und am PC-Programm DsRunner Art. Das Format folgt [Keep a Changelog](https://keepachangelog.com/de/1.1.0/), die Versionsnummern folgen [Semantic Versioning](https://semver.org/lang/de/). Vor Version 1.0 steht eine neue mittlere Zahl (0.**2**.0) für neue Funktionen, eine neue letzte Zahl (0.1.**2**) für Fehlerbehebungen und kleine Verbesserungen.

Downloads: [Releases](https://github.com/HolziDape/DsRunner/releases). Wie ein Release entsteht: [docs/RELEASE.md](docs/RELEASE.md).

## Unveröffentlicht

### Hinzugefügt
- **Spielstatistik:** Oben unter dem Spielnamen steht jetzt, wie oft ein Spiel gestartet wurde und wann zuletzt, z. B. „12× gespielt · zuletzt gestern“, „zuletzt am 03.10.“ oder „Noch nie gespielt“. DsRunner zählt die Starts in der neuen Datei `/_dsrunner/stats.txt` (höchstens 500 Spiele, Datum von der Uhr des DS); die Zählung beginnt mit diesem Update. Details: [docs/CONFIG.md](docs/CONFIG.md#spielstatistik).
- Einstellung `cleanNames` in `/_pico/settings.json` (Standard `true`): Spielnamen ohne Region, Sprachen und Dump-Zusätze von No-Intro und GoodTools, z. B. „Golden Sun“ statt „Golden Sun (Europe) (En,Fr,De)“. `(Rev 1)`, `(Beta)` und ähnliche bleiben. Details: [docs/CONFIG.md](docs/CONFIG.md#spielnamen).
- **Suche über alle Spiele:** Die neue Kachel „Suche“ auf der Konsolen-Ebene (mit Lupe) oder SELECT in jedem Ordner öffnet eine Bildschirmtastatur (Touchscreen oder Steuerkreuz + A; B löscht, X Leerzeichen, START sucht, SELECT zurück). Schon beim Tippen steht da, wie viele Spiele pro Konsole passen. Die Treffer erscheinen wie ein Ordner, nach Konsole geordnet und in der Farbe der Konsole; L / R zeigen nur eine Konsole oder alle. Gesucht wird im Dateinamen, ohne Groß-/Kleinschreibung, Akzente und Satzzeichen. Höchstens 200 Treffer werden gezeigt. Details: [docs/CONFIG.md](docs/CONFIG.md#suche).
- **DsRunner Stream im Release:** Die DS-App liegt jetzt in beiden SD-Paketen (`DsRunner_M3DSReal.zip`, `DsRunner.zip`) unter `Games/Apps/DsRunnerStream.nds`. Für den Raspberry Pi gibt es `DsRunnerStream-Host.zip` (ohne git herunterladen) mit `QUICKSTART.txt` und dem Installer `tools/install-pi.sh`: ein Befehl installiert Chromium und die Python-Pakete, setzt bei Bedarf das WLAN-Land, legt den Hotspot für den DS an, schreibt `~/.config/dsstream.env` (Adresse des Spiels) und das Start-Skript `~/dsstream.sh`, auf Wunsch auch den Autostart. Er fragt vor jeder Änderung und darf mehrmals laufen. Anleitung: [stream/README.md](stream/README.md#schnellstart).
- DsRunner Stream: Option `--jpeg` (Chromium liefert JPEG statt PNG; optional, auf dem Raspberry Pi 5 brachte es im Test keine zusätzlichen Bilder pro Sekunde). Das leichte JPEG-Rauschen wird beim Vergleich der Bildkacheln ignoriert, damit nicht jedes Bild neu übertragen wird.
- **Experimentell: DsRunner Stream:** Browser-Spiele vom Raspberry Pi 5 oder PC auf dem DS spielen. Der Host (`stream/host`, Python + Chromium) streamt eine beliebige Webseite in 256×192 (oder 128×96, oder über beide Bildschirme) mit 256 Farben per WLAN an das neue DS-Programm `DsRunnerStream.nds`; Tasten und Touchscreen gehen als Tastatur und Maus zurück (Belegung per JSON-Datei). Der Pi stellt dafür ein offenes WLAN nur für den DS bereit (`stream/host/tools/hotspot.sh`). Einstellungen am DS optional in `/_dsrunner/stream.ini`. Auf DS Lite + Raspberry Pi 5 getestet: etwa 18-25 Bilder pro Sekunde. Kein Ton auf dem DS. Anleitung: [stream/README.md](stream/README.md).
- **Andere Flashkarten:** DsRunner ist nicht auf die M3 DS Real beschränkt. Das README zeigt jetzt unter [Kompatible Flashkarten](README.md#kompatible-flashkarten), auf welchen Karten DsRunner laufen sollte (alle, die Pico Loader unterstützt, z. B. Acekard 2/2i, Ace3DS+, DSTT, Original R4, R4iDSN, M3i Zero, Stargate 3DS, DSpico, EZ-Flash Parallel und Slot-2-Karten wie die SuperCard), wie man es dort einrichtet und wie der Autostart mit den Dateien von TWiLight Menu++ geht. Details: [docs/FLASHCARDS.md](docs/FLASHCARDS.md). Getestet ist weiterhin nur die M3 DS Real.
- **Pico Loader für andere Flashkarten:** Zu jedem Release gibt es `PicoLoader-DsRunner_<Plattform>.zip` für 17 Kartentypen (z. B. `PicoLoader-DsRunner_DSTT.zip`). Damit geht „zurück zu DsRunner aus dem Spiel“ auch dort (ungetestet; auf der Plattform `R4` nur in manchen Spielen).
- **Tasten für „zurück zu DsRunner“ einstellbar** (Pico Loader geändert): `"inGameResetKeys"` in `/_pico/settings.json`, z. B. `"SELECT+START+DOWN"` oder `"SELECT+DOWN+B"`, wenn L oder R ausgeleiert sind. Erlaubt sind A, B, SELECT, START, Steuerkreuz, L und R (nicht X/Y), mindestens 3 Tasten. Standard bleibt L + R + Unten + B. Details: [README](README.md#zurück-zu-dsrunner-aus-einem-spiel), [docs/CONFIG.md](docs/CONFIG.md).

### Geändert
- DsRunner Stream: Der Host schickt jetzt höchstens 30 statt 20 Bilder pro Sekunde (`--fps`). Der DS Lite zeigt 25-30 problemlos an, der Browser auf dem Pi 5 liefert etwa 25.
- Spiele anderer Konsolen (und DS-Spiele ohne Banner) heißen oben jetzt ohne Dateiendung, also „Zelda“ statt „Zelda.nes“, auch bei „START: Weiterspielen“ und in den Suchergebnissen. Die unterste Zeile zeigt weiter den vollständigen Dateinamen.
- Die Liste ★ Favoriten folgt jetzt auch der Sortierung „zuletzt gespielt“: die zuletzt gespielten Favoriten zuerst, die übrigen danach von A bis Z. Bisher war sie dann nach Namen sortiert.
- „Zurück zu DsRunner“ (L + R + Unten + B) ist jetzt auf DS Lite + M3 DS Real getestet und funktioniert.
- Fehlt beim Start der eigene Pfad, merkt sich DsRunner als Rückkehrziel jetzt auch `/_picoboot.nds` (so startet die DSpico DsRunner).

### Behoben
- Leere oder kaputte Emulator-Dateien (kleiner als 16 KB) nimmt DsRunner nicht mehr, statt beim Start mit „Failed to load rom header“ stehen zu bleiben.
- DsRunner Stream: Auf dem DS kamen keine Bilder an, weil die Bildpakete größer waren, als der DS annimmt (1300 Byte). Die Pakete sind jetzt standardmäßig 1200 Byte groß, einstellbar mit `--packet-size`. Auf dem DS Lite getestet: etwa 18 Bilder pro Sekunde.
- DsRunner Stream: Die WLAN-Liste auf dem DS springt beim Neu-Sortieren nicht mehr auf ein anderes Netz, und „DsRunner-Stream“ wird automatisch gewählt, sobald es auftaucht. Der Pi beantwortet die Suche des DS zusätzlich per Broadcast; neue Option `--broadcast` sendet auch die Bilder so, für WLAN-Chips, deren Datenrate der DS Lite nicht versteht.
- DsRunner Stream: Bekommt der DS im Netz „DsRunner-Stream“ keine Adresse per DHCP (Anzeige „hole IP-Adresse (DHCP)“, dann „Zeitueberschreitung“), nimmt die App nach 8 Sekunden die feste Adresse 192.168.4.5. Neue Einstellung `ip=` in `stream.ini`.
- DsRunner Stream: Wird die Seite geschlossen oder stürzt sie ab (z. B. durch ein Werbe-Pop-up), öffnet der Host sie von selbst neu. Pop-ups werden automatisch geschlossen (`--allow-popups` streamt sie stattdessen).

## [0.3.1](https://github.com/HolziDape/DsRunner/releases/tag/v0.3.1) - 2026-10-14

### Hinzugefügt
- **Emulatoren im Paket:** `DsRunner_M3DSReal.zip` und `DsRunner.zip` bringen in `/_dsrunner/emulators/` die Emulatoren für GBA (GBARunner2), NES (nesDS 2.1), Game Boy / Game Boy Color (GameYob) und SNES (SNEmulDS 0.6, dazu `/snemul.cfg`) mit, samt Lizenzen in `LIZENZEN.txt`. Diese Konsolen funktionieren damit ohne weitere Downloads. Das GBA-BIOS ist nicht dabei (optional selbst als `/_gba/bios.bin` ablegen), ebenso wenig S8DS, NitroGrafx und jEnesisDS: Ihre Autoren nennen keine Lizenz, die die Weitergabe erlaubt. Wer schon installiert hat, kopiert einmal den Ordner `_dsrunner/emulators` und `snemul.cfg` aus dem Paket, siehe [README](README.md#emulatoren-für-andere-konsolen).
- **Diagnose für die Rückkehr aus Spielen** (Pico Loader geändert): Pico Loader schreibt bei jedem Spielstart `/_pico/igr-log.txt`. Darin steht, ob der Hotkey ins Spiel eingebaut wurde und wenn nicht, warum, und ob die Rückkehr danach geklappt hat. Hält man L + R + Unten + B lange genug, wird der Bildschirm kurz hell, bevor DsRunner startet. Anleitung: [loader/README.md](loader/README.md#diagnose-_picoigr-logtxt).

## [0.3.0](https://github.com/HolziDape/DsRunner/releases/tag/v0.3.0) - 2026-10-14

### Hinzugefügt
- **Eigene Bilder für die Konsolen-Ordner, schon eingebaut:** Jeder Ordner auf der Konsolen-Ebene (NDS, GBA, GB, GBC, NES, SNES, Master System, Game Gear, Mega Drive, PC Engine, Apps) und die Kachel ★ Favoriten zeigen eine gezeichnete Konsole bzw. einen Stern in der Konsolenfarbe, mit dem Kürzel darunter. Oben gibt es dazu ein passendes Hintergrundbild, im Coverflow ein passendes Cover. Eigene Konsolen aus `systems.json` bekommen ein Modul, andere Ordner einen Ordner, jeweils in ihrer Farbe. Die Bilder liegen im Theme (`/_pico/themes/dsrunner/consoles`) und funktionieren ohne DsRunner Art. Eigene Bilder in `/_dsrunner/art` und `/_dsrunner/tiles` haben weiter Vorrang, siehe [docs/CONFIG.md](docs/CONFIG.md#eingebaute-konsolen-bilder).
- **Zurück zu DsRunner aus dem Spiel:** im laufenden DS-Spiel **L + R + Unten + B zwei Sekunden halten**, dann startet der DS neu in DsRunner. Vorher speichern, nicht Gespeichertes geht verloren; ein laufender Speichervorgang wird nicht abgebrochen. Klappt nicht in jedem Spiel (dann läuft das Spiel normal weiter, nur ohne Hotkey). Noch nicht auf der echten M3 DS Real getestet.
- Einstellung `inGameReset` in `/_pico/settings.json` (Standard `true`): mit `false` baut Pico Loader den Hotkey nicht ein.
- Das Paket für die M3 DS Real enthält in `_pico/picoLoader-DsRunner.txt`, welcher Pico-Loader-Build drin ist.

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
