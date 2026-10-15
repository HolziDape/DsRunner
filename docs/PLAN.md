# DsRunner: Plan

Stand: 13.10.2026 · Gerät: **DS Lite + M3 DS Real** · Basis: [Pico Launcher](https://github.com/LNH-team/pico-launcher) (Zlib) + [Pico Loader](https://github.com/LNH-team/pico-loader) (Plattform `M3DS`)

## Status

| Phase | Stand |
|---|---|
| 0 Fundament | ✅ erledigt (PR #15). Autostart auf der M3 DS Real funktioniert, DsRunner startet direkt im Pico-Look. |
| 1 Design-System | ✅ erledigt, auf dem DS getestet |
| 2 Konsolen-Ebene | ✅ erledigt, auf dem DS getestet (Spielanzahl kam mit Phase 3, Favoriten mit Phase 6) |
| 3 Kachel-Raster | ✅ auf dem DS getestet, zuletzt mit den drei Ansichten (14.10.2026). Danach: eingebaute Bilder für die Konsolen-Ordner (siehe unten), **Test auf dem DS steht aus** |
| 4 Großes Artwork oben | ✅ erledigt, auf dem DS getestet (Testbilder werden angezeigt und überblendet) |
| 5 PC-Tool „DsRunner Art“ | ✅ im Code (Tests, echte Downloads von libretro-thumbnails, erzeugte Bilder in DeSmuME angezeigt), **Test unter Windows mit der Karte steht aus** |
| 6 Komfort | ✅ im Code, gebaut und in DeSmuME geprüft (deutsche Oberfläche, Favoriten, Weiterspielen, Uhr im Menü), **Test auf dem DS steht aus** |
| 7 Weitere Konsolen | ✅ im Code, gebaut und in DeSmuME geprüft (Konsolen-Kacheln, Spielanzahl, `systems.json`), **Test mit echten Emulatoren auf dem DS steht aus** |
| 8 Loader | ✅ im Code: eigener Pico-Loader-Build mit „zurück zu DsRunner“ (L+R+Unten+B 2 s), gebaut, **Test auf dem DS steht aus** ([LOADER.md](LOADER.md)) |
| 9 Releases | ✅ Version aus `VERSION`, Changelog, Release-Workflow mit Versionsprüfung und Release-Notizen, `LIESMICH.txt` im SD-Paket ([RELEASE.md](RELEASE.md)) |

**Test auf dem DS (13.10.2026, Phase 1–4):** dunkles Theme, Konsolen-Ebene mit Spielanzahl und Artworks laufen. Die neue Kachel-Ansicht war noch nicht eingeschaltet, weil eine vorhandene `settings.json` die alte Ansicht behält.

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

**Ergebnis:** ✅ Umgesetzt (siehe [CONFIG.md](CONFIG.md#ansichten)):
- Ansichten `TileGrid3x2` (76 × 70 px, Icons 2× vergrößert) und `TileGrid4x3` (55 × 44 px), im Menü als erste beiden Layouts, gespeichert in `settings.json`. `TileGrid3x2` ist der neue Standard.
- Die Kacheln zeichnet die 3D-Engine (runde Ecken, Farbverlauf, Akzent-Rahmen und Leuchten bei Auswahl), das Icon liegt als Sprite darauf. Dadurch kosten die Kacheln kein OBJ-VRAM und keine OBJ-Palettenzeilen. Zum Vergrößern werden die Sprites der Icons nachträglich auf affine Doppelgröße umgestellt, die Palettenzeilen werden für die doppelte Höhe reserviert.
- Reihenfolge zeilenweise (A B C / D E F) statt Picos spaltenweiser Reihenfolge, Blättern nach unten. Kacheln, die unter die App-Leiste scrollen, werden dort abgeschnitten (das Icon verschwindet, sobald es hineinragen würde).
- Spielanzahl pro Konsole auf dem oberen Bildschirm, im Hintergrund gezählt (IO-Warteschlange, einmal pro Start, beim Spielstart abgebrochen).
- Sortierung „zuletzt gespielt“ über den Spielverlauf `/_dsrunner/recent.txt`. Ersetzt im Menü Picos „zuletzt geändert“, das nie umgesetzt war.

**Offen:**
- Spielanzahl auf der Kachel selbst (bisher nur oben).

**Nach dem Test auf dem DS (13.10.2026):**
- **Hänger behoben:** Nach GBA → zurück → NDS hing DsRunner (Streifen oben, falsche Icon-Farben). Ursache: Der IO-Thread hatte nur 2 KB Stack. Er braucht schon ohne Artworks gut 1,6 KB (FatFs mit langen Dateinamen); ein Überlauf schreibt in die Thread-Verwaltung direkt darunter, danach hängt der VBlank-Interrupt. Jetzt 8 KB Stack für IO- und Hintergrund-Thread. Außerdem: die Aufgaben-Warteschlangen haben 64 statt 32 Plätze und warten bei Überfüllung, statt Speicher zu überschreiben; Artworks laden höchstens einzeln und erst, wenn die Auswahl 20 Bilder lang stehen bleibt.
- **Kachel-Bilder:** Die 32×32-Icons wirkten in den großen Kacheln zu grob, GBA-Spiele haben keine. DsRunner zeigt jetzt ein 64×64-Bild (256 Farben, 4 KB Textur pro Kachel) aus `/_dsrunner/tiles`, das DsRunner Art aus dem Cover erzeugt ([CONFIG.md](CONFIG.md#kachel-bilder)). Im 3×2-Raster 1:1, im 4×3-Raster verkleinert auf 38 px. Ohne Bild bleibt das Icon.

**Drei Ansichten, drei Bildstile (13.10.2026):** Die Kachel-Bilder wirkten für ihre kleine Pixelgröße zu detailreich. Jetzt gibt es im Menü nur noch drei Ansichten, jede mit dem passenden Bild ([CONFIG.md](CONFIG.md#ansichten)):
- **Raster 3 × 2:** Kachel-Bild 64 × 64, 1:1, von DsRunner Art jetzt vereinfacht erzeugt (leicht geglättet, höchstens 24 Farben ohne Dithering). Alte Kachel-Bilder erneuert das Tool einmal automatisch.
- **Raster 4 × 3:** nur noch das Icon 32 × 32 in Originalgröße (klare Pixelgrafik), keine Kachel-Bilder und kein Textur-VRAM mehr. DS-Spiele zeigen ihr Banner-Icon, andere Spiele ein Icon, das DsRunner Art in `/_pico/icons/user` anlegt (15 Farben, Ausschnitt um Logo und Hauptfigur).
- **CoverFlow (Pico):** die echten Cover, von DsRunner Art in `/_pico/covers/user` angelegt (128 × 96, Bild in 106 × 96, 255 Farben mit Dithering). Bisher zeigte die Ansicht nur „?“.
- Picos Icon-Raster (waagrecht/senkrecht) und die Bannerliste sind aus dem Menü entfernt. Eine `settings.json` mit einer dieser Ansichten (oder `FileList`) öffnet das 3×2-Raster.
- **Abdunkeln oben:** langer, weicher Verlauf ab Zeile 64 statt eines kurzen harten Übergangs.

**Eingebaute Konsolen-Bilder (14.10.2026):** Wunsch nach dem Test: ein Icon und ein passender Hintergrund für die Konsolen-Ordner und die Favoriten.
- Jeder Konsolen-Ordner (NDS, GBA, GB, GBC, NES, SNES, SMS, GG, MD, PCE, Apps) und ★ Favoriten zeigen eine eigene gezeichnete Konsole bzw. einen Stern: Kachel 3×2 (64×64, Kürzel darunter), 4×3 (Icon 32×32, Kürzel darunter), Hintergrund oben (256×192), Coverflow-Cover. Eigene Konsolen aus `systems.json`: Modul (CART), sonst Ordner (FOLDER).
- Bilder in `_pico/themes/dsrunner/consoles/`, erzeugt von `tools/console-art/make_console_art.py` (eigene Zeichnungen, keine Logos). Es sind Graustufen-Vorlagen, DsRunner färbt sie in der Konsolenfarbe ein (`DsRunnerConsoleArt`), dadurch passen sie auch zu Farben aus `systems.json`.
- Vorrang: eigene Bilder (`/_dsrunner/art`, `/_dsrunner/tiles`, `icon.bmp`/`cover.bmp` im Ordner) vor den eingebauten. Ohne `FOLDER-icon.bmp` im Theme bleibt alles wie früher.
- DsRunner Art erzeugt dann keine Ordner-Bilder mehr und löscht seine alten (selbst abgelegte bleiben).
- Offen: Farben und Kontrast auf dem DS Lite prüfen (graue Konsolen, ausgewählte Favoriten-Kachel). Console-Cover entstehen nur, wenn beim Laden des Ordners der Coverflow eingestellt ist.

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

**Ergebnis:** ✅ Umgesetzt in `tools/dsrunner-art/` (Anleitung: [ART-TOOL.md](ART-TOOL.md)):
- Python 3.10+ mit Pillow, sonst nur Standardbibliothek. Fenster (tkinter, deutsch) und Kommandozeile (`py dsrunner_art.py --sd E:\`). Der Release-Workflow baut zusätzlich eine `DsRunnerArt.exe` (PyInstaller, eigener Job auf `windows-latest`).
- Scan der Bibliothek aus `libraryPath`. Erkennung: NDS/GBA über den Game-Code im Header, nachgeschlagen in den No-Intro-Listen von libretro-database (dort steht der Code als `serial`), andere Konsolen über CRC32, sonst über Dateiname bzw. DS-Bannertitel (exakt, dann ähnlichster Name mit gleichen Zahlen).
- Quellen: libretro-thumbnails (Titelbild, Screenshot, Cover; Symlinks der Repos werden aufgelöst), GameTDB-Cover für NDS als Ausweichquelle. Quellen sind austauschbar (`sources.py`), SteamGridDB/ScreenScraper lassen sich dort ergänzen.
- Standard ist das **Titelbild**: Bei DS-Spielen ist es in libretro-thumbnails 256 × 384 (beide Bildschirme), der obere Bildschirm passt genau. Cover werden oben angeschnitten bzw. bei hohen Verpackungen vor einen unscharfen Hintergrund gesetzt.
- Umwandlung: 256 × 192, Palette aus 255 DS-Farben (15 bit), Floyd-Steinberg-Dithering, Palettenfarbe 0 bleibt frei. Jede Datei wird vor dem Schreiben wie in `DsRunnerTopArt::Load` geprüft.
- Konsolen-Ordner und Unterordner bekommen ein erzeugtes Bild (Verlauf in der Konsolenfarbe, großes Kürzel). DS-Spiele ohne Fund bekommen ein Ersatzbild aus dem Banner-Icon, das bei späteren Läufen durch ein echtes Bild ersetzt wird.
- Eigene Bilder in `/_dsrunner/art-override/` (gleicher Pfad oder nur der Name) haben Vorrang. `/_dsrunner/art/.dsrunner-art.json` merkt sich, welche Bilder vom Tool stammen: von Hand abgelegte werden nie überschrieben, weitere Läufe laden nur, was fehlt. Cache auf dem PC für Listen, Bilder, Prüfsummen und Fehlversuche.

**Offen:**
- Test unter Windows mit echter Karte und größerer Sammlung (Trefferquote der Namenssuche bei frei benannten Dateien, Laufzeit der Prüfsummen über USB).
- GameTDB-Adressen (`art.gametdb.com/ds/coverHQ|coverM|cover|coverS/<Region>/<Code>`) konnten in der Entwicklungsumgebung nicht geprüft werden (Host gesperrt). Das Tool überspringt die Quelle, wenn sie nicht antwortet.
- Die Dateiliste der Thumbnail-Repos kommt über die GitHub-API (ohne Anmeldung 60 Anfragen pro Stunde, eine pro Konsole und Woche). Ohne sie werden Bilder direkt per Name versucht.
- ~~Kleine Cover für die Kacheln~~: erledigt, siehe Kachel-Bilder in Phase 3.
- ~~Bilder für alle Ansichten~~: erledigt, das Tool legt auch Cover (`/_pico/covers/user`) und kleine Icons (`/_pico/icons/user`) an, siehe „Drei Ansichten“ in Phase 3.

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
- Ein Favorit, der in der Favoriten-Liste mit X entfernt wird, bleibt bis zum nächsten Öffnen sichtbar.
- Englische Oberfläche über `language`: bei Bedarf eine kleine Texttabelle statt der festen deutschen Texte.

**Suche (15.10.2026):** Wunsch: alle Spiele durchsuchen, nach Konsole geordnet ([CONFIG.md](CONFIG.md#suche)).
- Kachel „Suche“ (Lupe, `SEARCH` in den eingebauten Bildern) auf der Konsolen-Ebene nach ★ Favoriten, außerdem SELECT in jedem Ordner. Eigener Zustand `SearchInput`: die Tastatur (`DsRunnerSearchKeyboardView`, QWERTZ ohne Umlaute, Tasten als 3D-Polygone, Text als ein Sprite pro Taste, höchstens 4 OBJ-Palettenzeilen pro Bildschirmzeile, OBJ-VRAM aus dem Bereich der Dialoge) ersetzt das Raster.
- `DsRunnerSearch`: beim ersten Öffnen liest der IO-Thread alle Konsolen-Ordner (2 Ebenen Unterordner) in einen kompakten Index (Name, Dateityp, `FastFileRef`, ≈ 32 Byte plus Name pro Spiel, höchstens 2000 Spiele / 64 KB Namen). Zählen beim Tippen läuft im Speicher, die Trefferliste (höchstens 200) entsteht ohne Zugriff auf die Karte. Vergleich ohne Groß-/Kleinschreibung, Akzente und Satzzeichen, nur Dateinamen (kein Bannertitel).
- Treffer als virtueller Ordner wie die Favoriten (`GetItemFolderPath` pro Datei), nach Konsole gruppiert, Kacheln in Konsolenfarbe, Konsole oben vor dem Dateinamen, L/R filtert, B zurück zur Tastatur.
- Virtuelle Ordner sortieren „zuletzt gespielt“ jetzt mit dem Ordner jeder Datei: die Favoriten folgen damit auch dieser Sortierung.
- Offen: Test auf dem DS (Tempo des Einlesens bei großen Sammlungen, Touch-Genauigkeit). SELECT ließ sich in DeSmuME nicht auslösen (Tastenbelegung), der Weg über die Kachel ist geprüft.

### Phase 7: Weitere Konsolen (optional)
Emulator-Homebrew einbinden (GBARunner2, nesDS, GameYob, S8DS, jEnesisDS …) über `systems.json`, mit passenden Kacheln und Artworks.

**Ergebnis:** ✅ Umgesetzt (siehe [CONFIG.md](CONFIG.md#emulatoren)):
- **Emulatoren werden gefunden** wie bisher GBARunner2: nesDS (NES, FDS), GameYob bzw. lameboy (GB/GBC), S8DS (SMS/GG), jEnesisDS (Mega Drive), SNEmulDS (SNES), NitroGrafx (PC Engine). Gesucht wird in `/_dsrunner/emulators`, `/_dsrunner`, `/`, `/_nds`, `/_nds/TWiLightMenu/emulators` und `/Emulators`. Jeder Ordner wird beim Start einmal gelesen. Das Spiel wird wie bei Picos Dateizuordnungen per argv übergeben.
- **Reihenfolge:** `fileAssociations` in `settings.json` vor `emulator` in `systems.json` vor gefundenen Emulatoren. Gefundene Emulatoren werden nicht mehr in die `settings.json` geschrieben (bisher landete GBARunner2 dort beim ersten Spielstart und blieb stehen). Ohne Emulator bleiben die Spiele einer Konsole ausgeblendet.
- **`/_dsrunner/systems.json`** (optional): Ordnername → Name, Kürzel, Farbe, Endungen, Emulator, libretro-System. Überschreibt eingebaute Konsolen oder legt neue an (z. B. WonderSwan mit NitroSwan). Höchstens 16 Konsolen mit je 4 Endungen, 8 KB. Gelesen einmal auf dem IO-Thread vor dem ersten Ordner, ungültige Dateien werden mit einer Logzeile ignoriert.
- Neue Endungen der eingebauten Konsolen: `.sgb`, `.fds`, `.md`/`.gen`/`.smd`, `.sgx`, Ordner `FC` und `SFC` als weitere Namen. Spiele ohne eigenes Icon bekommen die farbige Kachel ihrer Konsole mit Kürzel.
- Code: `DsRunnerEmulators` (Tabelle und Suche), `DsRunnerSystems` (systems.json, Dateitypen), Einhängepunkte nur in `ExtensionFileTypeProvider` und `RomBrowserController`.
- **DsRunner Art** kennt dieselben Konsolen und Endungen, liest `systems.json` (Farben, Feld `libretro`, sonst die Endung) und meldet fehlerhafte Einträge und fehlende Emulatoren. Ein Test vergleicht `consoles.py` mit `DsRunnerConsoles.h` und `DsRunnerEmulators.h`.

**Offen:**
- Test auf dem DS mit jedem Emulator (Start per argv, Spielstände). Unsicher: lameboy (argv nicht dokumentiert), das neue SNEmulDS (TGDS), jEnesisDS mit `.md` statt `.gen`.
- `.md` gilt kartenweit: eine `README.md` erscheint als Mega-Drive-Spiel, sobald jEnesisDS gefunden wurde.
- Spiele anderer Konsolen heißen wie ihre Datei, mit Endung („Metroid.nes“).

### Phase 8: Loader (bei Bedarf)
- Spiele, die mit Pico schlechter laufen als mit YSMenu, sammeln und vergleichen → Fork `HolziDape/pico-loader`
- Wunsch-Feature: **zurück zum Launcher aus Retail-Spielen** (Soft-Reset). Pico kann das bisher nur bei Homebrew.

**Ergebnis (14.10.2026):** ✅ im Code, Test auf dem DS steht aus (siehe [LOADER.md](LOADER.md), [loader/README.md](../loader/README.md)):
- Pico Loader als Patch-Serie in `loader/`: Upstream `b9f923e` (v1.7.1 plus Fixes), danyocoms DLDI-Cache und In-Game-Reset, drei DsRunner-Patches. Gebaut mit `loader/build.sh` (BlocksDS 1.20, .NET 9 für die Listen), in der CI als eigener Job. Das M3-Paket nimmt standardmäßig diesen Build, `DSRUNNER_LOADER=upstream` packt wie bisher das offizielle v1.7.1.
- Hotkey L+R+Unten+B 2 s. Der Loader merkt sich den Launcher-Pfad in `/_pico/launcher.txt` (Fallback `/_nds/TWiLightMenu/main.srldr`). DsRunner prüft `argv[0]` und nimmt sonst einen bekannten Pfad. Abschaltbar mit `inGameReset` in `settings.json` (→ `/_pico/igr.off`).
- Offen: Test nach dem Testplan in [compat/pico-vs-ysmenu.md](compat/pico-vs-ysmenu.md), Ergebnisse in die Spalte „Hotkey → Launcher“. Auch ohne Hotkey ist es ein anderer Build als das offizielle v1.7.1: zuerst normales Spielen und Speichern prüfen. Bei jedem Upstream-Release `loader/` nachziehen.

**Recherche (13.10.2026):** siehe [LOADER.md](LOADER.md). Kurz:
- Pico kann „zurück zum Launcher“ bisher nur bei Homebrew. Bei Retail-Spielen fehlen ein Tasten-Auslöser, ein Launcher-Pfad, der den Neustart übersteht, und ein DLDI-Treiber für den Launcher.
- Am besten passt der Fork-Branch `igr-hooks` von danyocom (L+R+Unten+B 2 s halten, wartet auf laufende Speicherzugriffe, plattformunabhängig) zusammen mit dem DLDI-Cache aus Pico-PR #220. Auf der M3 DS Real ist er noch nicht getestet.
- Plan, falls gewünscht: Fork `HolziDape/pico-loader`, beides übernehmen, Launcher-Pfad für DsRunner festlegen, M3DS bauen (≈ 1–2 Tage), dann Test mit 20–30 Spielen auf dem DS.
- Spiele, die mit Pico schlechter laufen als mit YSMenu, kommen in [compat/pico-vs-ysmenu.md](compat/pico-vs-ysmenu.md).
- Vorher prüfen: ob die Autostart-Kette DsRunner überhaupt seinen eigenen Pfad (`argv[0]`) übergibt. Sonst klappt die Rückkehr nicht einmal bei Homebrew.

### Phase 9: Releases
Versionierung `v0.x`, Changelog, Release-Paket für die M3 DS Real (siehe Phase 0), Installationsanleitung.

**Ergebnis:** ✅ Umgesetzt (siehe [RELEASE.md](RELEASE.md)):
- **Version:** Die Datei `VERSION` im Hauptverzeichnis ist die einzige Quelle (jetzt `0.2.0`, die nächste Version). Der Launcher zeigt sie unten rechts im Menü „Anzeige“, das Banner in der Titelzeile, DsRunner Art im Fenstertitel und mit `--version`. Nur der Release-Workflow baut mit `DSRUNNER_RELEASE=1` die reine Nummer, jeder andere Build zeigt `0.2.0-dev+<commit>`.
- **`tools/version.sh`:** `set X.Y.Z` setzt VERSION und DsRunner Art und macht aus „Unveröffentlicht“ im Changelog den Abschnitt der Version, `check vX.Y.Z` prüft vor dem Release, `notes` gibt die Release-Notizen aus.
- **Changelog:** `CHANGELOG.md` auf Deutsch im Stil von Keep a Changelog. Picos Changelog bis zum Fork liegt unter `docs/PicoLauncher-CHANGELOG.md`.
- **Release-Workflow:** prüft zuerst, dass der Tag `v` + VERSION ist und DsRunner Art und Changelog passen (sonst wird nichts gebaut), baut die drei Dateien und setzt zum Schluss den Changelog-Abschnitt an den Anfang der Release-Beschreibung.
- **SD-Paket:** `LIESMICH.txt` mit Version, Spiele-Ordnern, Update-Anleitung, Hinweis auf DsRunner Art und dem Weg zurück zu YSMenu.

**Offen:** Der erste Release mit dem neuen Workflow (v0.2.0) prüft den Notizen-Job zum ersten Mal echt.

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

Auf dem DS testen: die eingebauten Konsolen-Bilder (Kacheln, Hintergrund oben, alle drei Ansichten) und der neue Pico Loader nach dem Testplan in [compat/pico-vs-ysmenu.md](compat/pico-vs-ysmenu.md): erst normales Spielen und Speichern, dann L+R+Unten+B im Spiel. Danach Release v0.3.0.
