# Pico Loader für DsRunner

Dieser Ordner baut den **Pico Loader** ([LNH-team/pico-loader](https://github.com/LNH-team/pico-loader), Zlib-Lizenz) mit ein paar Zusatz-Patches. Damit kommt man aus **Retail-Spielen zurück zu DsRunner**: im Spiel **L+R+Unten+B 2 Sekunden halten**.

Hier liegt kein kompletter Quellcode, nur:

| Datei | Inhalt |
|---|---|
| `UPSTREAM` | Upstream-Repo, festgelegter Commit und Toolchain (BlocksDS-Image, .NET-Version) |
| `patches/*.patch` | Patch-Serie, wird der Reihe nach mit `git am` angewendet |
| `build.sh` | klont, patcht und baut alles |
| `work/` | (nicht im Git) Klon von pico-loader, wird beim nächsten Bau wiederverwendet |
| `out/` | (nicht im Git) fertige Dateien: `picoLoader7.bin`, `picoLoader9.bin`, `aplist.bin`, `patchlist.bin`, `savelist.bin`, `BUILDINFO.txt` |

Hintergrund und Recherche: [docs/LOADER.md](../docs/LOADER.md).

## Basis

- **Upstream:** `LNH-team/pico-loader` Branch `develop` @ `b9f923e` (18.09.2026, „Update changelog“). Das ist v1.7.1 plus drei Fixes (Banner-Saves, BG2/BG3-Register, argv auf DSi). Die Listen `aplist`/`savelist`/`patchlist` sind seit v1.7.1 unverändert, die gebauten `.bin` sind byte-gleich mit dem Release.
- **Plattform:** `M3DS` (M3 DS Real). Andere Plattformen: `PICO_PLATFORM=… loader/build.sh`.

## Patches

| Nr. | Patch | Herkunft |
|---|---|---|
| 0001 | DLDI-Treiber auf der SD-Karte zwischenspeichern (`/_pico/dldi.bin`). Nach einem Reset aus dem Spiel bekommt Pico keinen DLDI-Treiber übergeben, DsRunner braucht aber einen. | danyocom, Branch `dldi-cache` (= Upstream-PR [#220](https://github.com/LNH-team/pico-loader/pull/220)), Commit `8f9d8db` |
| 0002 | DLDI-Cache: Treiber mit BSS hinter dem Datenende auch speichern | danyocom `dldi-cache` `84962d8` |
| 0003 | Reboot-Code von `OS_ResetSystem` als gemeinsamer Patchcode, auch für Spiele ohne `OS_ResetSystem` | danyocom `igr-hooks` `e7d4564` |
| 0004 | Optionale Patches dürfen vorher prüfen, ob noch Patchplatz frei ist | danyocom `igr-hooks` `43c4a80` |
| 0005 | **In-Game-Reset:** Hook im ARM9-IRQ-Dispatcher des SDK zählt bei VBlank die gehaltenen Tasten. Vor dem Reset wird der **Slot-1-Lock** genommen (kein ROM-Lesen und kein Speichern wird unterbrochen), dann `resetParam = 'PIGR'` und Sprung in den Reboot-Code. Der neu geladene Pico-ARM7 bootet dann den Launcher. | danyocom `igr-hooks` `a7def04` |
| 0006 | Launcher bekommt nach dem Reset `argv[1] = "igr"`; Combo dort auf L+R+Unten+B+A, 1 s geändert | danyocom `igr-dispatcher-variants` `44cd832` |
| 0007–0011 | Dispatcher-Varianten: gemeinsamer Tastencheck, eigene Hooks für Golden Sun: Dark Dawn, Black Sigil, Diddy Kong Racing DS | danyocom `igr-dispatcher-variants` `8b4fe8f`, `4d520b5`, `b8698f3`, `dc992c7`, `ca06bd5` |
| 0012 | **DsRunner:** Combo zurück auf **L+R+Unten+B, 2 s** (wie in der DsRunner-Bedienung) | eigener Patch |
| 0013 | **DsRunner:** Launcher-Pfad überlebt den Reset: Bei jedem normalen Start speichert Pico den von DsRunner übergebenen `launcherPath` in `/_pico/launcher.txt` (nur wenn er sich geändert hat, über `.tmp` + Umbenennen). Nach dem Reset wird dieser Pfad gebootet. Fehlt die Datei, nimmt Pico die erste vorhandene von `/_nds/TWiLightMenu/main.srldr`, `/BOOT.NDS`, `/_picoboot.nds`. | eigener Patch |
| 0014 | **DsRunner:** Abschalten ohne neuen Build: Gibt es `/_pico/igr.off`, wird der In-Game-Reset-Patch weggelassen. DsRunner legt die Datei passend zur Einstellung `inGameReset` an oder löscht sie. | eigener Patch |

Die Patches 0001–0011 sind Cherry-Picks aus dem Fork von **danyocom** (Dan Yocom), ohne Änderung und ohne Konflikte auf `b9f923e` angewendet. Sie sind upstream (noch) nicht gemergt.

## Wie „zurück zum Launcher“ abläuft

1. DsRunner startet ein Spiel und übergibt im Pico-Header `v2.launcherPath` (aus `argv[0]`, sonst Fallback, siehe `arm9/source/romBrowser/Theme/dsrunner/DsRunnerInGameReset.cpp`). Pico speichert ihn in `/_pico/launcher.txt` und den DLDI-Treiber in `/_pico/dldi.bin`.
2. Pico patcht das Spiel. Der IRQ-Hook kommt **zuletzt** dazu und nur, wenn noch Patchplatz frei ist und der Dispatcher erkannt wird. Sonst läuft das Spiel normal, nur ohne Hotkey.
3. Im Spiel: L+R+Unten+B 2 s halten. Der Hook wartet auf den Slot-1-Lock, setzt `PIGR` und lädt Pico per SD-Lesecode der Plattform neu.
4. Der neue Pico-ARM7 sieht `PIGR`, liest `/_pico/launcher.txt`, bootet DsRunner mit dem DLDI aus `/_pico/dldi.bin` und `argv = { Pfad, "igr" }`.

**Nicht gespeicherter Spielfortschritt geht verloren**, wie beim Ausschalten. Spielstände schreibt Pico direkt auf die SD-Karte. Ein Reset mitten in einem Schreibvorgang ist durch den Slot-1-Lock ausgeschlossen.

## Bauen

```sh
loader/build.sh            # Ergebnis in loader/out/
loader/build.sh /pfad/out  # anderer Zielordner
loader/build.sh --id       # Kennung der aktuellen Quellen (Commit + Hash der Patches)
```

Braucht `git` und entweder **Docker** (Images `skylyrac/blocksds:slim-v1.20.0` und `mcr.microsoft.com/dotnet/sdk:9.0` werden geladen) oder eine lokale BlocksDS-Installation (`$BLOCKSDS` gesetzt) plus .NET 9 (`dotnet`). Das ist dieselbe Toolchain wie in der Upstream-CI (`.github/workflows/nightly.yml` von pico-loader). DsRunner selbst baut weiter mit BlocksDS 1.16.

Ein Bau dauert etwa 30 Sekunden. Gleiche Quellen ergeben byte-gleiche Dateien (geprüft mit zwei Läufen).

`tools/make-sd-package.sh` (und `./build.sh package`) nimmt die Dateien aus `loader/out/` und baut sie vorher selbst, wenn sie fehlen oder nicht zu den aktuellen Quellen passen. Mit `DSRUNNER_LOADER=upstream` kommt stattdessen das unveränderte Pico-Loader-Release v1.7.1 ins Paket (ohne Rückkehr aus Spielen).

## Aktualisieren

Auf einen neueren Upstream-Stand oder neue Patches von danyocom:

```sh
cd loader/work/pico-loader              # Klon aus dem letzten Bau
git fetch origin
git checkout -b update <neuer-commit>   # z. B. origin/develop oder ein Tag
git am ../../patches/*.patch            # Konflikte lösen, dann git am --continue
# optional neue Commits dazunehmen:
#   git remote add danyo https://github.com/danyocom/pico-loader.git && git fetch danyo
#   git cherry-pick <commit>
git format-patch --zero-commit --no-signature -o ../../patches.neu <neuer-commit>..HEAD
```

Dann `loader/patches/` durch `patches.neu/` ersetzen, in `UPSTREAM` den Commit (und falls nötig das BlocksDS-Image aus der Upstream-CI) eintragen, `loader/build.sh` laufen lassen und auf der Hardware testen (Testplan in [docs/compat/pico-vs-ysmenu.md](../docs/compat/pico-vs-ysmenu.md)). Die Patch-Tabelle oben anpassen.

Wenn LNH den In-Game-Reset selbst einbaut, fallen 0003–0011 weg. Dann reicht womöglich wieder das Upstream-Release mit kleinen DsRunner-Patches oder ganz ohne.

## Lizenz und Dank

Pico Loader: Copyright (c) LNH team, **Zlib-Lizenz** (siehe `LICENSE.txt` im Klon unter `work/pico-loader/` bzw. [upstream](https://github.com/LNH-team/pico-loader/blob/develop/LICENSE.txt)). Die Patches 0001–0011 stammen von **Dan Yocom ([danyocom](https://github.com/danyocom/pico-loader))**, Autor und Datum stehen in jedem Patch. Die Patches 0012–0014 gehören zu DsRunner. Alle Patches stehen unter derselben Zlib-Lizenz wie Pico Loader.
