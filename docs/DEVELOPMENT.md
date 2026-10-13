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
| `BOOT.NDS` | DsRunner. Startet automatisch beim Einschalten. |
| `_pico/` | Themes aus diesem Repo + Pico Loader `M3DS` (Release-Version im Skript festgelegt) |
| `SYSTEM/g6dsload.*`, `boot.*`, `loader.eng` | Autostart-Dateien der M3 DS Real aus TWiLight Menu++ (Commit im Skript festgelegt) |

Bei jedem GitHub-Release baut die CI das Paket automatisch als `DsRunner_M3DSReal.zip`.

## Testen

**melonDS** (`H:\dev\ds\DsRunner-emu`): Der Launcher selbst läuft dort, die SD-Karte ist der Ordner `sd\` (DLDI-Ordner-Sync). Gut geeignet für alle UI-Arbeiten. Spiele starten dort mit dem M3DS-Loader nicht. Dafür bräuchte man einen Loader für die Plattform `MELONDS`, den man selbst aus Pico Loader bauen müsste.

```powershell
Copy-Item DsRunner.nds H:\dev\ds\DsRunner-emu\sd\
H:\dev\ds\DsRunner-emu\melonDS\melonDS.exe H:\dev\ds\DsRunner-emu\sd\DsRunner.nds
```

Tastatur: X = A, Z = B, S = X, A = Y, Q/W = L/R, Enter = Start, Backspace = Select.

**Echte Hardware:** Spiele, Speicherstände und Autostart immer auf dem DS Lite prüfen.

## Icon

`icon.bmp` wird von `tools/icon/make_icon.py` erzeugt (32×32, 16 Farben, Farbe 0 transparent).

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
