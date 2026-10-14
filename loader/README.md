# Pico Loader für DsRunner

Dieser Ordner baut den **Pico Loader** ([LNH-team/pico-loader](https://github.com/LNH-team/pico-loader), Zlib-Lizenz) mit ein paar Zusatz-Patches. Damit kommt man aus **Retail-Spielen zurück zu DsRunner**: im Spiel **L+R+Unten+B 2 Sekunden halten**.

Hier liegt kein kompletter Quellcode, nur:

| Datei | Inhalt |
|---|---|
| `UPSTREAM` | Upstream-Repo, festgelegter Commit und Toolchain (BlocksDS-Image, .NET-Version) |
| `patches/*.patch` | Patch-Serie, wird der Reihe nach mit `git am` angewendet |
| `build.sh` | klont, patcht und baut alles |
| `test/` | Prüfungen ohne DS (`test/run.sh`, siehe [Testen ohne DS](#testen-ohne-ds)) |
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
| 0015 | **DsRunner: Diagnose.** Schreibt bei jedem Start `/_pico/igr-log.txt` (siehe [unten](#diagnose-_picoigr-logtxt)) und hellt den Hauptbildschirm halb auf, sobald der Hotkey erkannt ist. Die Build-Kennung kommt von `build.sh` (`DSRUNNER_BUILD_ID`). | eigener Patch |

Die Patches 0001–0011 sind Cherry-Picks aus dem Fork von **danyocom** (Dan Yocom), ohne Änderung und ohne Konflikte auf `b9f923e` angewendet. Sie sind upstream (noch) nicht gemergt.

## Wie „zurück zum Launcher“ abläuft

1. DsRunner startet ein Spiel und übergibt im Pico-Header `v2.launcherPath` (aus `argv[0]`, sonst Fallback, siehe `arm9/source/romBrowser/Theme/dsrunner/DsRunnerInGameReset.cpp`). Pico speichert ihn in `/_pico/launcher.txt` und den DLDI-Treiber in `/_pico/dldi.bin`.
2. Pico patcht das Spiel. Der IRQ-Hook kommt **zuletzt** dazu und nur, wenn noch Patchplatz frei ist und der Dispatcher erkannt wird. Sonst läuft das Spiel normal, nur ohne Hotkey.
3. Im Spiel: L+R+Unten+B 2 s halten. Der Hook wartet auf den Slot-1-Lock, setzt `PIGR` und lädt Pico per SD-Lesecode der Plattform neu.
4. Der neue Pico-ARM7 sieht `PIGR`, liest `/_pico/launcher.txt`, bootet DsRunner mit dem DLDI aus `/_pico/dldi.bin` und `argv = { Pfad, "igr" }`.

**Nicht gespeicherter Spielfortschritt geht verloren**, wie beim Ausschalten. Spielstände schreibt Pico direkt auf die SD-Karte. Ein Reset mitten in einem Schreibvorgang ist durch den Slot-1-Lock ausgeschlossen.

## Diagnose: `/_pico/igr-log.txt`

Pico Loader schreibt die Datei bei **jedem Start eines Spiels oder Programms neu**, kurz bevor es startet. Nach einem Start über den Hotkey (oder über den eigenen Soft-Reset des Spiels) hängt er einen Abschnitt **an**. So steht nach einem Versuch beides drin: das Spiel und was danach passiert ist. Also **direkt nach dem Versuch** die Karte in den PC stecken, bevor ein anderes Spiel gestartet wird.

Beispiel (Spiel mit Hotkey, danach erfolgreiche Rückkehr):

```
DsRunner Pico Loader: in-game reset log (rewritten on every start)
build: b9f923e59ddced0528e3bd4e2c4f7e00f69b4d60-3528b875307f2e8e

[normal start]
rom: fat:/Games/NDS/Mario Kart DS.nds
game: AMCP "MARIOKARTDS" (retail)
DLDI: handed down by the launcher, saved to /_pico/dldi.bin
launcher: fat:/_nds/TWiLightMenu/main.srldr [/_pico/launcher.txt is up to date]
/_pico/igr.off: absent
platform: M3DS
sdk version: 0x4017530
OS_ResetSystem: not found (0x0)
in-game reset: applied
  dispatcher sdk: MATCH (pattern found 1 times)
  dispatcher blx: not tried
  ...
  hook at 0x200A1C0, continues at 0x1FF81D8
  dispatch code at 0x2000F01, reboot created for the hotkey

[start after the in-game reset (PIGR): booting the launcher]
rom: fat:/_nds/TWiLightMenu/main.srldr
game: #### "DSRUNNER" (homebrew)
DLDI: none handed down, loaded /_pico/dldi.bin
DLDI for homebrew: available
launcher: fat:/_nds/TWiLightMenu/main.srldr [from /_pico/launcher.txt]
```

| Zeile | Bedeutung, und was zu tun ist |
|---|---|
| *(Datei fehlt)* | Auf der Karte liegt **nicht** dieser Loader (z. B. nur `main.srldr` ersetzt, `_pico/*.bin` noch vom offiziellen v1.7.1). Alle fünf `.bin` aus dem Paket nach `/_pico` kopieren. |
| `build:` | Kennung des Builds (Upstream-Commit + Hash der Patches, wie `loader/build.sh --id`) |
| `in-game reset: applied` | Der Hotkey ist eingebaut. |
| `NOT applied: switched off by /_pico/igr.off` | In `settings.json` steht `"inGameReset": false`. |
| `NOT applied: no known irq dispatcher found` | Das Spiel hat einen unbekannten IRQ-Dispatcher. Die Zeilen `dispatcher …` darunter sagen, woran es lag: `no match` (Muster fehlt), `ambiguous` (mehrfach), `dispatcher start not found`, `return address mismatch`, `invalid irq table`, `pattern found, rejected by a check`. Game-Code melden. |
| `NOT applied: not enough patch space` (+ `needed … bytes`) | Nach den Patches, die das Spiel zum Laufen braucht, war kein Platz mehr. Game-Code melden. |
| `NOT applied: … no sd read code` | Spiel ohne `OS_ResetSystem`, und die ROM-Lesepatches haben keinen SD-Lesecode angelegt. |
| `DLDI: … not cacheable` / `FAILED` | `/_pico/dldi.bin` fehlt, nach dem Hotkey käme dann die Fehlermeldung „DLDI driver not found“. |
| `launcher: (none) […handed down no path…]` | DsRunner hat keinen Rückkehrpfad übergeben, der Hotkey könnte nicht zu DsRunner zurück. |
| Abschnitt `[start after the in-game reset (PIGR)…]` | Der Hotkey hat ausgelöst und Pico neu gestartet. Steht darunter `DLDI for homebrew: MISSING`, fehlte der DLDI-Cache. |
| Abschnitt `[start after the game's own soft reset…]` statt PIGR | Der Reset kam an, aber die Markierung `PIGR` nicht: Pico hat das Spiel neu gestartet statt DsRunner. |

**Am Bildschirm:** Nach 2 s Halten wird der Hauptbildschirm (meist der obere) **halb weiß**, dann startet DsRunner. Wird es hell, und das Spiel läuft hell weiter, wartet der Reset auf den Slot-1-Lock (eine Karten- oder Speicheroperation hört nicht auf). Wird es gar nicht hell, ist der Hotkey nicht eingebaut (siehe Log) oder die Tasten kommen nicht an. Friert das Bild nach dem Hellwerden ein oder wird schwarz, hängt der Neustart von Pico.

Wichtig: `picoLoader7.bin` und `picoLoader9.bin` immer **zusammen** aus demselben Build kopieren. Ab 0015 sprechen beide über einen zusätzlichen Befehl miteinander, gemischte Versionen bleiben beim Start hängen.

## Testen ohne DS

`loader/test/run.sh` (nach `loader/build.sh`) prüft ohne Hardware:

1. `irq_dispatcher_search_test.cpp`: die echte Dispatcher-Suche aus dem gepatchten Quellcode, für den PC übersetzt, auf künstlichen ARM9-Abbildern. Der Dispatcher darin folgt `OS_IrqHandler` aus dem NitroSDK mit denselben Anfangs- und End-Signaturen, die nds-bootstrap zum Einhängen benutzt. Geprüft werden Fundstelle, Tabelle, Rücksprungadresse, Mehrdeutigkeit, die Suchgrenze von 32 Wörtern und Kopien außerhalb des ITCM-Blocks.
2. `igr_log_test.cpp`: der Text von `igr-log.txt` (Neuschreiben und Anhängen) auf einem PC-Dateisystem.
3. `igr_patchcode_test.py`: der **assemblierte** Patchcode aus `picoLoader9_M3DS.elf` in einem ARM-Emulator (unicorn, ARMv5): Tastenkombination, 120 VBlanks, Handler bekommt weiter die richtige Rücksprungadresse, Warten auf den Slot-1-Lock, EXMEMCNT/DMA/`PIGR` beim Reset, Aufhellen.

Braucht `g++`, `python3` und `pip install unicorn pyelftools`. Was das nicht zeigen kann: ob ein bestimmtes Spiel genau diesen Dispatcher enthält und genug Patchplatz hat. Das sagt `igr-log.txt` auf dem DS.

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

Pico Loader: Copyright (c) LNH team, **Zlib-Lizenz** (siehe `LICENSE.txt` im Klon unter `work/pico-loader/` bzw. [upstream](https://github.com/LNH-team/pico-loader/blob/develop/LICENSE.txt)). Die Patches 0001–0011 stammen von **Dan Yocom ([danyocom](https://github.com/danyocom/pico-loader))**, Autor und Datum stehen in jedem Patch. Die Patches 0012–0015 gehören zu DsRunner. Alle Patches stehen unter derselben Zlib-Lizenz wie Pico Loader.
