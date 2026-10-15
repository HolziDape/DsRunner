# DsRunner entwickeln

## Bauen

Voraussetzung: Docker (unter Windows in WSL2). Die Toolchain ist auf `skylyrac/blocksds:slim-v1.16.0` festgelegt, dieselbe Version wie in der CI.

```sh
./build.sh            # → DsRunner.nds
./build.sh clean
./build.sh package    # Build + fertiges SD-Paket für die M3 DS Real in sd-package/ (mit DsRunner Stream)
loader/build.sh       # Pico Loader mit DsRunner-Patches → loader/out/ (siehe unten)
stream/ds/build.sh    # DsRunner Stream → stream/ds/DsRunnerStream.nds (siehe stream/README.md)
stream/host/tools/make-host-bundle.sh   # DsRunnerStream-Host.zip für den Raspberry Pi
```

Das SD-Paket (`tools/make-sd-package.sh`) enthält:

| Datei | Herkunft |
|---|---|
| `_nds/TWiLightMenu/main.srldr` | DsRunner. Diese Datei startet die Autostart-Kette. |
| `BOOT.NDS` | DsRunner, zum manuellen Starten aus anderen Menüs |
| `_pico/` | Themes aus diesem Repo + Pico Loader `M3DS`: standardmäßig der eigene Build aus `loader/out/` (mit Rückkehr aus Spielen, Infodatei `_pico/picoLoader-DsRunner.txt`), mit `DSRUNNER_LOADER=upstream` das unveränderte Release (Version und SHA-256 im Skript) |
| `SYSTEM/g6dsload.*`, `boot.*`, `loader.eng`, `_system_/_sys_data/r4i.sys`, `SRESET.dat` | Autostart-Dateien der M3 DS Real aus dem TWiLight-Menu++-Release (Version und SHA-256 im Skript festgelegt) |
| `_dsrunner/emulators/`, `snemul.cfg` | `tools/fetch-emulators.sh`: GBARunner2 (Release des Autors), nesDS, GameYob, SNEmulDS-legacy und `snemul.cfg` (Virtual-Console-Add-on von TWiLight Menu++), Versionen und SHA-256 im Skript; dazu `LIZENZEN.txt` und Lizenztexte aus `licenses/emulators/`. Kein GBA-BIOS. Dasselbe kommt auch in `DsRunner.zip` |
| `Games/Apps/DsRunnerStream.nds` | DsRunner Stream (experimentell), nur wenn `stream/ds/build.sh` gelaufen ist; `./build.sh package` und die Release-CI bauen es immer mit. Dieselbe Datei kommt auch in `DsRunner.zip` |
| `LIESMICH.txt` | kurze Anleitung (Installation, Emulatoren, Update, Zurück zu YSMenu) mit der Version |

Autostart-Kette der M3 DS Real: Firmware → `SYSTEM/g6dsload.eng` → `_system_/_sys_data/r4i.sys` (Startprogramm von TWiLight mit M3-DLDI-Treiber) → `/_nds/TWiLightMenu/main.srldr`. Der Pfad ist im TWiLight-Startprogramm fest eingebaut. Später können wir ein eigenes Startprogramm bauen, das direkt `/DsRunner.nds` lädt.

Bei jedem GitHub-Release baut die CI zuerst den Pico Loader (Job „Build Pico Loader (loader/)“) und dann das Paket als `DsRunner_M3DSReal.zip`. Versionsnummer (`VERSION`), Changelog und die Schritte für ein Release: [RELEASE.md](RELEASE.md).

## Testen

**melonDS** (`H:\dev\ds\DsRunner-emu`): Der Launcher selbst läuft dort, die SD-Karte ist der Ordner `sd\` (DLDI-Ordner-Sync). Gut geeignet für alle UI-Arbeiten. Spiele starten dort mit dem M3DS-Loader nicht. Dafür bräuchte man einen Loader für die Plattform `MELONDS`: `PICO_PLATFORM=MELONDS loader/build.sh <ordner>` baut ihn (mit den DsRunner-Patches, also auch mit dem Hotkey), getestet ist das noch nicht.

```powershell
Copy-Item DsRunner.nds H:\dev\ds\DsRunner-emu\sd\
H:\dev\ds\DsRunner-emu\melonDS\melonDS.exe H:\dev\ds\DsRunner-emu\sd\DsRunner.nds
```

Tastatur: X = A, Z = B, S = X, A = Y, Q/W = L/R, Enter = Start, Backspace = Select.

**DeSmuME unter Linux (ohne Fenster):** `tools/desmume/screenshot.sh` patcht den R4-DLDI-Treiber in eine Kopie von `DsRunner.nds`, startet DeSmuME in Xvfb mit einem Ordner als SD-Karte, drückt Tasten und speichert beide Bildschirme als PNG. Gut für schnelle UI-Prüfungen und Screenshots in PRs (auch in der Cloud-Umgebung). Pakete: `apt install desmume xvfb xdotool imagemagick`.

```sh
tools/desmume/screenshot.sh ~/sd shot.png 10 Down x wait wait   # 10 s warten, Runter, A, 2 s warten
```

DeSmuME 0.9.11 rechnet Farbverläufe gröber als die Hardware (sichtbare Stufen im Hintergrund). Spiele starten dort ebenfalls nicht.

**Echte Hardware:** Spiele, Speicherstände und Autostart immer auf dem DS Lite prüfen.

## Icon

`icon.png` wird von `tools/icon/make_icon.py` erzeugt (32×32, RGBA, höchstens 15 Farben plus Transparenz). Kein `.bmp` verwenden: ndstool baut die Banner-Palette aus den Farben neu auf und erkennt Transparenz nur am Alpha-Kanal, Farbe 0 einer BMP wird sonst als normale Farbe (Magenta) angezeigt.

## Konsolen-Bilder

Die eingebauten Bilder der Konsolen-Ordner (`_pico/themes/dsrunner/consoles/`, siehe [CONFIG.md](CONFIG.md#eingebaute-konsolen-bilder)) zeichnet `tools/console-art/make_console_art.py` (Pillow). Nach einer Änderung an den Zeichnungen das Skript laufen lassen und die erzeugten BMPs mit committen; `--preview vorschau.png` zeigt alle Bilder eingefärbt nebeneinander. Die Zuordnung Ordnername → Bild steht doppelt: `DsRunnerConsoleArt::kArtKeys` und `ART_KEYS` in `tools/dsrunner-art/dsrunnerart/consoles.py` (ein Test prüft, dass beide gleich sind).

## Pico-Updates übernehmen

Remote `upstream` = [LNH-team/pico-launcher](https://github.com/LNH-team/pico-launcher), Branch `develop`.

```sh
git fetch upstream
git checkout -b chore/upstream-sync main
git merge upstream/develop      # Konflikte lösen, bauen, in melonDS testen
git push -u origin chore/upstream-sync   # → PR nach main
```

Damit das einfach bleibt: Eigene Funktionen kommen in **eigene Dateien** (eigener Theme-Typ, eigene DisplayModes und Views). Pico-Dateien ändern wir nur an kleinen, klar kommentierten Einhängepunkten.

## Pico Loader (loader/)

DsRunner bringt für die M3 DS Real einen **eigenen Build von Pico Loader** mit: Upstream-Commit plus Patch-Serie, damit man aus Retail-Spielen mit L+R+Unten+B zurück zu DsRunner kommt. Im Repo liegen nur `loader/UPSTREAM` (Commit, Toolchain), `loader/patches/*.patch` und `loader/build.sh`, kein kompletter Quellcode. Alles Weitere, auch die Herkunft jedes Patches: **[loader/README.md](../loader/README.md)**.

```sh
loader/build.sh                              # klont nach loader/work/, wendet die Patches an, baut → loader/out/
PICO_PLATFORM=R4 loader/build.sh /tmp/r4     # andere Plattform, anderer Zielordner
loader/build.sh --id                         # Kennung der Quellen (Commit + Hash der Patches)
DSRUNNER_LOADER=upstream ./build.sh package  # Paket mit dem unveränderten Release
```

- **Toolchain:** BlocksDS **1.20** (`skylyrac/blocksds:slim-v1.20.0`) und .NET 9 für die Listen, wie die CI von pico-loader. DsRunner selbst bleibt bei 1.16. Ohne lokale Installation nimmt `build.sh` Docker. Hinter einem Proxy werden `HTTPS_PROXY` und `SSL_CERT_FILE` an den .NET-Container durchgereicht (NuGet).
- **Paket:** `tools/make-sd-package.sh` vergleicht die Kennung in `loader/out/BUILDINFO.txt` mit den aktuellen Quellen und baut neu, wenn sie nicht passt. Schlägt der Bau fehl, bricht das Paket mit einem Hinweis auf `DSRUNNER_LOADER=upstream` ab, statt still den Loader ohne Rückkehr einzupacken.
- **CI:** Job `pico_loader` in `release.yml` und `nightly.yml` (Container 1.20, `actions/setup-dotnet`, Cache des Klons), Artefakt `PicoLoader_M3DS_DsRunner`. Ein Schritt prüft, dass das Container-Image zu `loader/UPSTREAM` passt; beim Wechsel beide Stellen ändern.
- **Aktualisieren** (neuer Upstream-Stand, neue Patches von danyocom, eigene Änderungen): Ablauf in [loader/README.md](../loader/README.md#aktualisieren). Kurz: im Klon `loader/work/pico-loader` auf den neuen Commit wechseln, die Patches mit `git am` anwenden, Konflikte lösen, mit `git format-patch --zero-commit --no-signature` neu exportieren, `UPSTREAM` anpassen, bauen, auf der Hardware testen und im CHANGELOG „Pico Loader geändert“ vermerken (dann müssen Nutzer die `_pico/*.bin` mit ersetzen).
- **Eigene Loader-Änderungen** kommen als neuer Patch hinten an die Serie, Code-Stellen mit `// DsRunner:` kommentiert, Commit-Text auf Englisch wie upstream.
- Die Version des unveränderten Releases (Fallback) steht weiter in `tools/make-sd-package.sh` (`PICO_LOADER_VERSION`). Neue Loader-Releases bringen oft Spiel-Fixes, also regelmäßig prüfen und dann auch `loader/UPSTREAM` nachziehen.
- Zum Ausprobieren ohne M3: Der Build für die Plattform `R4` startet in DeSmuME (`--slot1=R4`) Homebrew aus DsRunner. Retail-Spiele und damit der Hotkey lassen sich so nicht testen.

## Branches

- `main`: aktueller Stand von DsRunner (Default)
- `develop`: Spiegel von Pico `develop` zum Vergleich
- `feature/*`, `fix/*`, `docs/*`: Arbeitszweige → PR nach `main`

## Stolpersteine

- **Data Cache und DMA:** melonDS emuliert den ARM9-Datencache nicht, die Hardware schon. Puffer, die die CPU beschrieben hat, müssen vor einem DMA-Transfer mit `DC_FlushRange` aus dem Cache geschrieben werden, sonst liest der DMA alte Daten (zu sehen als fehlende Grafik nur auf dem DS). Alternativ mit `memcpy` direkt nach VRAM schreiben.
- **VRAM:** OBJ-/BG-VRAM nimmt keine Byte-Schreibzugriffe an, nur 16/32 Bit.
- **Thread-Stacks:** IO- und Hintergrund-Thread haben 8 KB Stack (`App.h`). Ein Überlauf läuft in die Thread-Struktur direkt unter dem Stack und zeigt sich als Hänger im VBlank-Interrupt (`rtos_wakeupQueue`), nicht als sauberer Absturz. Große Puffer gehören deshalb nicht auf den Stack von IO-Aufgaben. Nachmessen: Das RTOS füllt Stacks mit `0xA5`, mit dem DeSmuME-GDB-Stub (`--arm9gdb=PORT`, `gdb-multiarch`) lässt sich der unberührte Rest zählen.
- **Palettenzeilen:** Es gibt 16 OBJ-Paletten pro Bildschirmzeile. Zeilen mit gleichen Farben werden zusammengelegt, ist keine frei, bleibt der Launcher mit einer Log-Meldung stehen. Neue Elemente mit eigener Palette immer mit möglichst kleinem Y-Bereich anmelden.
- **Affine OBJ-Matrizen:** Es gibt 32 pro Bildschirm, `OamManager::AllocMatrices` prüft das nicht. Die vergrößerten Icons im 3×2-Raster holen sich eine Matrix pro Kachel und lassen das Icon unvergrößert, wenn keine mehr frei ist.
- **DMA-Kanäle:** Kanal 3 kopiert (auch im IO-Thread), Kanal 1 ist die HBlank-DMA für das Abdunkeln des Artworks (`DsRunnerTopArt`). Kanal 0 wird in jedem VBlank gestoppt.
- **Statusleiste, RTC:** `rtc_readDateTime` blockiert bis der ARM7 antwortet. Nicht jedes Bild aufrufen.
