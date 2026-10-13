# DsRunner: Plan

Stand: 13.10.2026 · Gerät: **DS Lite + M3 DS Real** · Basis: [Pico Launcher](https://github.com/LNH-team/pico-launcher) (Zlib) + [Pico Loader](https://github.com/LNH-team/pico-loader) (Plattform `M3DS`)

## Status

| Phase | Stand |
|---|---|
| 0 Fundament | ✅ erledigt (PR #15). Autostart auf der M3 DS Real funktioniert, DsRunner startet direkt im Pico-Look. |
| 1 Design-System | ⏳ als Nächstes |
| 2 Konsolen-Ebene | ⏳ als Nächstes |
| 3–9 | offen |

**Test auf dem DS (13.10.2026):** Autostart ok, Spiele werden mit Icon und Titel angezeigt. Erwartete Lücken (Theme, Ansicht, Startordner, GBA) sind den Phasen unten zugeordnet.

## Ziel

Ein eigener Launcher als Ersatz für YSMenu:
- **startet beim Einschalten automatisch**
- **sieht modern aus**, im dunklen Stil der AYN-Thor-Launcher (Entwurf: `design-ideen/entwurf/`)
- **zwei Ebenen:** erst Konsolen, dann Spiele. Unten ein Raster aus Kacheln, oben ein großes Artwork zur Auswahl.
- **Artworks** besorgt ein PC-Tool automatisch
- NDS-Spiele laufen über Pico Loader. Im Test lief er gut. Bei Bedarf verbessern wir ihn später.

## Ausgangslage

- Pico Launcher und Pico Loader v1.3.0 / v1.7.1 laufen auf der Karte. Getestet: Start über YSMenu, Spiele, Speichern.
- `HolziDape/DsRunner` enthält die Historie von Pico Launcher. Remote `upstream` = LNH-team/pico-launcher (`develop`).
- Build: `skylyrac/blocksds:slim-v1.16.0` (Docker in WSL), `./build.sh` → `DsRunner.nds`, `./build.sh package` → SD-Paket (Details: [DEVELOPMENT.md](DEVELOPMENT.md))
- Code-Aufbau: `arm9/source/romBrowser/DisplayMode/*` (Ansichten), `romBrowser/Theme/*` + `themes/*` (Aussehen), `romBrowser/CoverRepository` (Cover), `RomBrowserTopScreenView` (oberer Bildschirm, bisher 106×96-Cover als 8-bpp-Bitmap auf Sub-BG3).

## Grundsätze

- **Eigene Dateien statt Pico-Dateien umbauen.** Neue DisplayModes, ein eigener Theme-Typ `dsrunner` und eigene Views. Änderungen an Pico-Dateien nur als kleine Einhängepunkte, damit `git merge upstream/develop` einfach bleibt.
- **Eigene Daten unter `/_dsrunner/`** (Konsolen, Artworks, Verlauf, Favoriten). `/_pico/` bleibt Pico-kompatibel.
- **Jede Phase endet mit einem Test auf dem DS.** Für die UI zusätzlich in melonDS.
- **Arbeitsweise:** Branch → PR → du mergst.

## Phasen

### Phase 0: Fundament (≈ 1 Sitzung)
1. `build.sh`: Build über Docker mit fester BlocksDS-Version, Ausgabe `DsRunner.nds`
2. CI für `main` + Release-Workflow mit Paket für die M3 DS Real: `DsRunner.nds` als `BOOT.NDS`, `_pico` mit Loader M3DS, Autostart-Dateien, Anleitung
3. Branding: Name, Banner und Icon „DsRunner“, About-Info
4. **Entwicklung in melonDS:** prüfen, ob der Launcher mit DLDI in melonDS läuft, damit UI-Arbeit ohne SD-Umstecken geht. Spiele starten dort über Pico Loaders `MELONDS`-Plattform, falls nötig.
5. **Autostart testen:** TWiLight-Autostart-Dateien für M3 DS Real. Der alte `System`-Ordner wird gesichert, damit man zurückwechseln kann.
6. Upstream-Sync dokumentieren: wie man Pico-Updates holt

**Ergebnis:** ✅ Die Karte startet beim Einschalten direkt den selbst gebauten DsRunner (noch im Pico-Look).

**Erkenntnisse:**
- Autostart-Kette: Firmware → `SYSTEM/g6dsload.eng` → `_system_/_sys_data/r4i.sys` (Startprogramm von TWiLight mit M3-DLDI) → **`/_nds/TWiLightMenu/main.srldr`** (nicht `BOOT.NDS`, wie die TWiLight-Anleitung nahelegt). `r4i.sys` und `SRESET.dat` gibt es nur im TWiLight-Release, nicht im Quellcode.
- Backups auf der Karte: `System_YSMenu_backup/` (inkl. der `SRESET.DAT` von YSMenu)
- melonDS: Der Launcher läuft dort, Spiele mit dem M3DS-Loader nicht.
- Später (optional): ein eigenes Startprogramm statt `r4i.sys` von TWiLight, das direkt `/DsRunner.nds` lädt

### Phase 1: Design-System „dsrunner“ (dunkel)
1. Neuer Theme-Typ `dsrunner` (ViewFactory) mit den Farben aus dem Entwurf: Hintergrund #0F1117 → #181C26, Kacheln #232838, Akzent #4CC2FF
2. Kachel-View mit runden Ecken, abgedunkelt wenn nicht ausgewählt, Leucht-Rahmen bei Auswahl, weiche Animation beim Wechseln
3. App-Leiste: Titel, Zurück-Pfeil, Sortier-Pille, Zahnrad
4. Statusleiste oben rechts: Uhrzeit (RTC) und Akku. **Hinweis:** Der DS Lite kennt beim Akku nur „gut“ und „schwach“, keine Prozentanzeige.
5. Das dunkle DsRunner-Theme ist Standard (ersetzt Picos helles „Material“-Theme)

### Phase 2: Konsolen-Ebene
1. Startbildschirm „Konsolen“: Raster aus Konsolen-Kacheln, oben das Artwork der Konsole mit „34 Spiele · zuletzt: …“
2. Konfiguration `/_dsrunner/systems.json`: Konsole → Ordner/Dateiendungen → Start-Programm (NDS: Pico Loader, GBA: GBARunner2, NES: nesDS …). Ohne Datei werden `Games\NDS`, `Games\GBA` usw. automatisch erkannt.
3. Spezial-Kacheln: **Favoriten** und **Apps** (Homebrew)
4. A = Konsole öffnen, B = zurück
5. **DsRunner startet immer auf dem Konsolen-Bildschirm**, nicht im zuletzt geöffneten Ordner. Das zuletzt gespielte Spiel ist in der Konsole vorausgewählt.
6. **GBA-Spiele** öffnen GBARunner2 über die Dateizuordnung. Im Test war der GBA-Ordner noch leer, weil kein Start-Programm hinterlegt war.

### Phase 3: Spiele-Raster mit Covern
1. Neue DisplayModes **3×2** (72-px-Kacheln) und **4×3** (51 px), im Menü umschaltbar. Sie ersetzen Picos Bannerliste als Standard (im Test: „schlechte Auswahl“).
2. Cover-Kacheln als 3D-Texturen aus `/_dsrunner/art/<konsole>/…`. Fallback: Spiel-Icon auf Farbverlauf.
3. Nachladen im Hintergrund und Cache, damit Scrollen ruckelfrei bleibt. **VRAM-Budget beachten.**
4. Sortierung: A–Z, Z–A, **zuletzt gespielt**

### Phase 4: Großes Artwork oben
1. Bildschirmfüllendes Bild (256×192, 8 bpp, 256 Farben + Dithering, ≈ 48 KB) pro Konsole und Spiel. Ersetzt das kleine Cover auf dem oberen Bildschirm.
2. Abdunkelnder Verlauf unten, darüber Titel und Untertitel
3. Weiches Überblenden beim Wechseln der Auswahl
4. Fallback ohne Artwork: Farbverlauf in der Konsolenfarbe

### Phase 5: PC-Tool „DsRunner Art“ (Python, Windows)
1. SD-Karte scannen und Spiele erkennen: NDS/GBA über den Game-Code im ROM-Header, sonst über den Dateinamen
2. Artworks laden. Start mit **libretro-thumbnails** (Boxart, Titel, Screenshots; frei, ohne Konto). Optional später SteamGridDB oder ScreenScraper (brauchen ein Konto bzw. einen API-Key).
3. Zuschneiden und ins DS-Format umwandeln (Farbreduktion mit Dithering), nach `/_dsrunner/art/` schreiben
4. Eigene Bilder in einem Override-Ordner haben Vorrang
5. Einfache Bedienung: Laufwerk wählen → „Artworks aktualisieren“

### Phase 6: Komfort
1. Deutsche Oberfläche (prüfen, ob die Pico-Fonts Umlaute haben)
2. Favoriten (Taste X), Verlauf „zuletzt gespielt“, Menüpunkt „Weiterspielen“
3. Einstellungen im Menü: Rastergröße, Sortierung, Theme, Uhr ein/aus
4. Optional Hintergrundmusik (Pico unterstützt `.bcstm` pro Theme)

### Phase 7: Weitere Konsolen (optional)
Emulator-Homebrew einbinden (GBARunner2, nesDS, GameYob, S8DS, jEnesisDS …) über `systems.json`, mit passenden Kacheln und Artworks.

### Phase 8: Loader (bei Bedarf)
- Spiele, die mit Pico schlechter laufen als mit YSMenu, sammeln und vergleichen → Fork `HolziDape/pico-loader`
- Wunsch-Feature: **zurück zum Launcher aus Retail-Spielen** (Soft-Reset). Pico kann das bisher nur bei Homebrew.

### Phase 9: Releases
Versionierung `v0.x`, Changelog, Release-Paket für die M3 DS Real (siehe Phase 0), Installationsanleitung.

## Risiken

| Risiko | Gegenmaßnahme |
|---|---|
| VRAM-Grenzen (Artwork + 12 Cover-Texturen + UI) | Formate früh festlegen (8 bpp), Budget pro Phase messen, Cover nachladen |
| Langsame SD-Karte beim Laden vieler Bilder | Vorausladen der Nachbarn, Cache, kleine Dateiformate |
| Autostart-Dateien sind verschlüsselt, Ziel `BOOT.NDS` nicht selbst verifiziert | Test in Phase 0, Backup des `System`-Ordners |
| Merge-Konflikte mit Pico-Updates | eigene Dateien, kleine Einhängepunkte |
| Artwork-Rechte | Bilder werden nur auf deine eigene Karte geladen, nie im Repo oder Release verteilt |
| Lizenz | Zlib: Credits für Gericom und das LNH-team im README und About-Screen |

## Nächster Schritt

Phase 1 + 2 als erster sichtbarer Meilenstein: dein Konsolen-Bildschirm im dunklen Design auf dem DS. Entwicklung in melonDS mit Screenshots zur Abnahme, danach Test auf der Karte.
