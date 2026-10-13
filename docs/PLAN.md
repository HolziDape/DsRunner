# DsRunner: Plan

Stand: 13.10.2026 · Gerät: **DS Lite + M3 DS Real** · Basis: [Pico Launcher](https://github.com/LNH-team/pico-launcher) (Zlib) + [Pico Loader](https://github.com/LNH-team/pico-loader) (Plattform `M3DS`)

## Status

| Phase | Stand |
|---|---|
| 0 Fundament | ✅ erledigt (PR #15). Autostart auf der M3 DS Real funktioniert, DsRunner startet direkt im Pico-Look. |
| 1 Design-System | ✅ im Code und in melonDS geprüft, **Test auf dem DS steht aus** |
| 2 Konsolen-Ebene | ✅ im Code und in melonDS geprüft, **Test auf dem DS steht aus** (ohne Spielanzahl und Favoriten, siehe unten) |
| 3 Kachel-Raster | ✅ im Code, in DeSmuME geprüft, **Test auf dem DS steht aus** (ohne Cover in den Kacheln, siehe unten) |
| 4 Großes Artwork oben | ✅ im Code, in DeSmuME geprüft, **Test auf dem DS steht aus** |
| 6 Komfort | ✅ im Code (deutsche Oberfläche, Favoriten, Weiterspielen, Uhr im Menü), **Prüfung in DeSmuME und Test auf dem DS stehen aus** (siehe unten) |
| 5, 7–9 | offen |

**Test auf dem DS (13.10.2026, Phase 0):** Autostart ok, Spiele werden mit Icon und Titel angezeigt. Aufgefallen waren das helle Pico-Theme, die Bannerliste, der Start im zuletzt geöffneten Ordner und der leere GBA-Ordner. Phase 1 und 2 beheben das.

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

**Ergebnis:** ✅ Theme `DsRunner` (`_pico/themes/dsrunner`), Statusleiste nur in diesem Theme, Kacheln mit 4 px Abstand und Leucht-Rahmen, Verlauf auf dem oberen Bildschirm in der Farbe der gewählten Konsole.

**Umsetzung / Abweichungen vom Entwurf:**
- Obere Hälfte des oberen Bildschirms ist ein Farbverlauf (192 Palettenzeilen, nur die Palette wird beim Wechsel neu geschrieben), darunter ein flaches dunkles Band für den Text. Große Konsolen- und Spiel-Artworks folgen in Phase 4.
- Schriften: Es gibt nur NotoSansJP bis 11 pt. Der große Titel aus dem Entwurf ist damit nicht möglich.
- Das Kachel-Raster ist noch Picos Raster (44-px-Zellen, spaltenweise). Die großen 3×2/4×3-Kacheln kommen in Phase 3.
- Die Statusleiste liest die Uhr einmal pro Sekunde und den Akku alle 64 Bilder. Akku: nur „gut“ / „schwach“ / „lädt“.
### Phase 2: Konsolen-Ebene
1. Startbildschirm „Konsolen“: Kacheln der Unterordner der Bibliothek, oben Name der Konsole
2. A = Konsole öffnen, B = zurück. Auf der Konsolen-Ebene bleibt B wirkungslos.
3. **DsRunner startet immer auf dem Konsolen-Bildschirm**, nicht im zuletzt geöffneten Ordner. Die Konsole des zuletzt gespielten Spiels ist vorausgewählt, beim Zurückgehen ebenfalls der Ordner, aus dem man kommt.
4. **GBA-Spiele** öffnen GBARunner2 über die Dateizuordnung. Ist keine `gba`-Zuordnung eingetragen, wird GBARunner2 automatisch an den üblichen Pfaden gesucht.

**Ergebnis:** ✅ Umgesetzt (siehe [CONFIG.md](CONFIG.md)):
- Bibliothek = Einstellung `libraryPath` (Standard `/Games`). Fehlt der Ordner, startet DsRunner im SD-Root wie Pico.
- Konsolen werden am **Ordnernamen** erkannt (NDS, GBA, GBC, GB, NES, SNES, SMS, GG, MD, PCE, Apps): farbige Kachel mit Kürzel, passender Verlauf oben. Unbekannte Ordner bekommen eine neutrale Kachel mit den ersten Buchstaben.
- Der App-Leisten-Titel ist „Konsolen“ auf der Bibliotheksebene, sonst der Ordnername.

**Zurückgestellt:**
- **Spielanzahl** („34 Spiele“): Ein Zählen im Hintergrund braucht eine eigene Aufgabe in der IO-Warteschlange und einen Cache. Kommt mit Phase 3, wenn Cover ohnehin im Hintergrund geladen werden.
- **Favoriten-Kachel:** braucht Speicherung der Favoriten, siehe Phase 6 (dort umgesetzt).
- **`systems.json`** (frei konfigurierbare Konsolen mit eigenem Start-Programm): erst nötig, wenn weitere Emulatoren dazukommen (Phase 7). Bis dahin reichen Ordnername und Dateizuordnungen in `settings.json`.

### Phase 3: Spiele-Raster mit Covern
1. Neue DisplayModes **3×2** (72-px-Kacheln) und **4×3** (51 px), im Menü umschaltbar. Sie ersetzen Picos Bannerliste als Standard (im Test: „schlechte Auswahl“).
2. Cover-Kacheln als 3D-Texturen aus `/_dsrunner/art/<konsole>/…`. Fallback: Spiel-Icon auf Farbverlauf.
3. Nachladen im Hintergrund und Cache, damit Scrollen ruckelfrei bleibt. **VRAM-Budget beachten.**
4. Sortierung: A–Z, Z–A, **zuletzt gespielt**

**Ergebnis:** ✅ Umgesetzt (siehe [CONFIG.md](CONFIG.md#kachelraster)):
- Ansichten `TileGrid3x2` (76 × 70 px, Icons 2× vergrößert) und `TileGrid4x3` (55 × 44 px), im Menü als erste beiden Layouts, gespeichert in `settings.json`. `TileGrid3x2` ist der neue Standard.
- Die Kacheln zeichnet die 3D-Engine (runde Ecken, Farbverlauf, Akzent-Rahmen und Leuchten bei Auswahl), das Icon liegt als Sprite darauf. Dadurch kosten die Kacheln kein OBJ-VRAM und keine OBJ-Palettenzeilen. Zum Vergrößern werden die Sprites der Icons nachträglich auf affine Doppelgröße umgestellt, die Palettenzeilen werden für die doppelte Höhe reserviert.
- Reihenfolge zeilenweise (A B C / D E F) statt Picos spaltenweiser Reihenfolge, Blättern nach unten. Kacheln, die unter die App-Leiste scrollen, werden dort abgeschnitten (das Icon verschwindet, sobald es hineinragen würde).
- Spielanzahl pro Konsole auf dem oberen Bildschirm, im Hintergrund gezählt (IO-Warteschlange, einmal pro Start, beim Spielstart abgebrochen).
- Sortierung „zuletzt gespielt“ über den Spielverlauf `/_dsrunner/recent.txt`. Ersetzt im Menü Picos „zuletzt geändert“, das nie umgesetzt war.

**Offen:**
- **Cover in den Kacheln** (3D-Texturen): Ein 128×96-Cover mit 256 Farben braucht 12 KB Textur-VRAM. Das 3×2-Raster hält 13 Kacheln vor, das 4×3-Raster 22, das passt nicht neben die übrigen Texturen in Bank D (128 KB). Machbar wäre es mit kleineren Covern (z. B. 64×48 mit 16 Farben = 1,5 KB) und eigenem Nachladen. Kommt, wenn das PC-Tool (Phase 5) Cover erzeugt.
- Spielanzahl auf der Kachel selbst (bisher nur oben).

### Phase 4: Großes Artwork oben
1. Bildschirmfüllendes Bild (256×192, 8 bpp, 256 Farben + Dithering, ≈ 48 KB) pro Konsole und Spiel. Ersetzt das kleine Cover auf dem oberen Bildschirm.
2. Abdunkelnder Verlauf unten, darüber Titel und Untertitel
3. Weiches Überblenden beim Wechseln der Auswahl
4. Fallback ohne Artwork: Farbverlauf in der Konsolenfarbe

**Ergebnis:** ✅ Umgesetzt (Format und Ordner in [CONFIG.md](CONFIG.md#artworks)):
- BMP 256 × 192, 8 bpp unter `/_dsrunner/art/<Pfad wie auf der SD-Karte>.bmp`, für Konsolen-Ordner und Spiele. Geladen auf der IO-Warteschlange (6 Bilder Verzögerung, damit Halten des Steuerkreuzes nicht jede Datei liest) in einen 48-KB-Puffer, angezeigt auf Sub-BG2 (256-Farben-Bitmap in VRAM C ab 64 KB).
- Überblenden und Abdunkeln über Alpha-Blending mit dem Farbverlauf (BG0): eine HBlank-DMA (Kanal 1) schreibt pro Bildschirmzeile einen eigenen `BLDALPHA`-Wert. So gibt es den Verlauf nach unten und das Ein-/Ausblenden ohne Zusatzspeicher.
- Wechsel: altes Bild in den Farbverlauf ausblenden (≈ 8 Bilder), neues einblenden. Ein direktes Überblenden Bild-zu-Bild bräuchte ein zweites Bitmap und ist offen.
- Picos kleines Cover wird in den Kachel-Ansichten nicht mehr gezeigt. In Picos Ansichten liegt es wie bisher über dem Bild.

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

**Ergebnis:** ✅ Umgesetzt (siehe [CONFIG.md](CONFIG.md#favoriten)):
- **Deutsch:** Die NotoSansJP-Schriften (alle vier Größen bis Medium 11) enthalten ä ö ü Ä Ö Ü ß, ein Ersatz (ae/oe/ue) ist nicht nötig. Übersetzt: Menü „Anzeige“ (Ansicht, Sortieren, Uhr), Cheats („Keine Cheats gefunden.“), Theme-Auswahl („Keine Vorschau“), Beschreibung des DsRunner-Themes. Picos `language` wird wie bisher nicht ausgewertet, die Oberfläche ist immer deutsch. Nicht enthalten: `–` (Gedankenstrich), deshalb nicht in Texten verwenden.
- **Favoriten:** X schaltet das ausgewählte Spiel um (in allen Ansichten), gespeichert in `/_dsrunner/favorites.txt` (Format wie `recent.txt`, Schreiben auf der IO-Warteschlange mit eigener Kopie des Inhalts). In den Kachelrastern zeigt ein goldener Stern auf der rechten oberen Ecke den Favoriten an. Der Stern ist ein 3D-Polygon wie die Kachel selbst, kostet also keine OBJ-Palettenzeile. Oben steht ein ★ vor dem Dateinamen.
- **Kachel „★ Favoriten“:** Ein virtueller Ordner auf der Konsolen-Ebene (immer vorne, gold, Stern als Kürzel), sobald es Favoriten gibt. Er wird beim Laden der Bibliothek an die Dateiliste angehängt. Öffnen baut die Liste aus den Favoriten (jeder Ordner wird einmal gelesen, Dateien mit `FastFileRef` wie in einem echten Ordner). Der Controller merkt sich zu jeder Datei ihren Ordner (`GetItemFolderPath`) für Start, Artwork und Stern. Das aktuelle Verzeichnis bleibt dabei die Bibliothek.
- **Weiterspielen:** START auf der Konsolen-Ebene. DsRunner öffnet den Ordner des zuletzt gespielten Spiels mit dem Spiel ausgewählt und startet es über `LaunchFile`, also genau wie mit A. Der Hinweis „START: Weiterspielen (Name)“ steht oben in der untersten Zeile (statt des Ordnernamens). Das Spiel wird beim Laden der Konsolen-Ebene gesucht (neuester vorhandener Eintrag aus `recent.txt`, sonst `lastUsedFilePath`).
- **Menü:** dritte Zeile „Uhr“ im Menü „Anzeige“ (Taste mit Uhr-Symbol, `tools/make_menu_icons.py`), gespeichert als `showClock` in `settings.json`. Ohne Uhr zeigt die Statusleiste nur den Akku und liest die RTC nicht mehr. Rastergröße, Sortierung und Theme waren schon im Menü.
- **Hintergrundmusik:** nur dokumentiert ([CONFIG.md](CONFIG.md#hintergrundmusik)), Picos `bgm`-Ordner pro Theme funktioniert unverändert.

**Offen:**
- **Build und Test:** In der Sitzung, in der Phase 6 entstand, war Docker nicht erreichbar. Der Code ist nur mit dem Host-Compiler auf Syntax geprüft. Vor dem Merge: `./build.sh`, DeSmuME-Screenshots (Konsolen-Ebene mit Favoriten-Kachel, Stern auf Kacheln, Menü mit Uhr) und Test auf dem DS (Spielstart aus den Favoriten und mit START, auch GBA).
- Favoriten-Stern nur in den Kachelrastern, nicht in Picos Ansichten (dort nur der ★ oben).
- Die Favoriten-Liste sortiert nach Namen. „Zuletzt gespielt“ bräuchte den Spielverlauf pro Datei statt pro Ordner.
- Ein Favorit, der in der Favoriten-Liste mit X entfernt wird, bleibt bis zum nächsten Öffnen sichtbar.
- Englische Oberfläche über `language`: bei Bedarf eine kleine Texttabelle statt der festen deutschen Texte.

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

Phase 3 + 4 auf dem DS testen (Kacheln, Blättern, Spielanzahl, Sortierung nach Spielstart, Artworks mit Test-Bildern). Phase 6 bauen, in DeSmuME prüfen und auf dem DS testen (Favoriten, Weiterspielen, Uhr). Danach Phase 5: das PC-Tool erzeugt die Artworks im Format aus [CONFIG.md](CONFIG.md#artworks).
