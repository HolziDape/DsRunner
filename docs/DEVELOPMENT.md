# DsRunner entwickeln

## Bauen

Voraussetzung: Docker (unter Windows in WSL2). Die Toolchain ist auf `skylyrac/blocksds:slim-v1.16.0` festgelegt, dieselbe Version wie in der CI.

```sh
./build.sh            # → DsRunner.nds
./build.sh clean
./build.sh package    # Build + fertiges SD-Paket für die M3 DS Real in sd-package/
```

Das SD-Paket (`tools/make-sd-package.sh`) enthält:

| Datei | Herkunft |
|---|---|
| `_nds/TWiLightMenu/main.srldr` | DsRunner. Diese Datei startet die Autostart-Kette. |
| `BOOT.NDS` | DsRunner, zum manuellen Starten aus anderen Menüs |
| `_pico/` | Themes aus diesem Repo + Pico Loader `M3DS` (Release-Version im Skript festgelegt) |
| `SYSTEM/g6dsload.*`, `boot.*`, `loader.eng`, `_system_/_sys_data/r4i.sys`, `SRESET.dat` | Autostart-Dateien der M3 DS Real aus dem TWiLight-Menu++-Release (Version und SHA-256 im Skript festgelegt) |

Autostart-Kette der M3 DS Real: Firmware → `SYSTEM/g6dsload.eng` → `_system_/_sys_data/r4i.sys` (Startprogramm von TWiLight mit M3-DLDI-Treiber) → `/_nds/TWiLightMenu/main.srldr`. Der Pfad ist im TWiLight-Startprogramm fest eingebaut. Später können wir ein eigenes Startprogramm bauen, das direkt `/DsRunner.nds` lädt.

Bei jedem GitHub-Release baut die CI das Paket automatisch als `DsRunner_M3DSReal.zip`.

## Testen

**melonDS** (`H:\dev\ds\DsRunner-emu`): Der Launcher selbst läuft dort, die SD-Karte ist der Ordner `sd\` (DLDI-Ordner-Sync). Gut geeignet für alle UI-Arbeiten. Spiele starten dort mit dem M3DS-Loader nicht. Dafür bräuchte man einen Loader für die Plattform `MELONDS`, den man selbst aus Pico Loader bauen müsste.

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

## Pico-Updates übernehmen

Remote `upstream` = [LNH-team/pico-launcher](https://github.com/LNH-team/pico-launcher), Branch `develop`.

```sh
git fetch upstream
git checkout -b chore/upstream-sync main
git merge upstream/develop      # Konflikte lösen, bauen, in melonDS testen
git push -u origin chore/upstream-sync   # → PR nach main
```

Damit das einfach bleibt: Eigene Funktionen kommen in **eigene Dateien** (eigener Theme-Typ, eigene DisplayModes und Views). Pico-Dateien ändern wir nur an kleinen, klar kommentierten Einhängepunkten.

Die Version des Pico Loaders steht in `tools/make-sd-package.sh` (`PICO_LOADER_VERSION`). Neue Loader-Releases bringen oft Spiel-Fixes, also regelmäßig prüfen.

## Branches

- `main`: aktueller Stand von DsRunner (Default)
- `develop`: Spiegel von Pico `develop` zum Vergleich
- `feature/*`, `fix/*`, `docs/*`: Arbeitszweige → PR nach `main`

## Stolpersteine

- **Data Cache und DMA:** melonDS emuliert den ARM9-Datencache nicht, die Hardware schon. Puffer, die die CPU beschrieben hat, müssen vor einem DMA-Transfer mit `DC_FlushRange` aus dem Cache geschrieben werden, sonst liest der DMA alte Daten (zu sehen als fehlende Grafik nur auf dem DS). Alternativ mit `memcpy` direkt nach VRAM schreiben.
- **VRAM:** OBJ-/BG-VRAM nimmt keine Byte-Schreibzugriffe an, nur 16/32 Bit.
- **Palettenzeilen:** Es gibt 16 OBJ-Paletten pro Bildschirmzeile. Zeilen mit gleichen Farben werden zusammengelegt, ist keine frei, bleibt der Launcher mit einer Log-Meldung stehen. Neue Elemente mit eigener Palette immer mit möglichst kleinem Y-Bereich anmelden.
- **Affine OBJ-Matrizen:** Es gibt 32 pro Bildschirm, `OamManager::AllocMatrices` prüft das nicht. Die vergrößerten Icons im 3×2-Raster holen sich eine Matrix pro Kachel und lassen das Icon unvergrößert, wenn keine mehr frei ist.
- **DMA-Kanäle:** Kanal 3 kopiert (auch im IO-Thread), Kanal 1 ist die HBlank-DMA für das Abdunkeln des Artworks (`DsRunnerTopArt`). Kanal 0 wird in jedem VBlank gestoppt.
- **Statusleiste, RTC:** `rtc_readDateTime` blockiert bis der ARM7 antwortet. Nicht jedes Bild aufrufen.
