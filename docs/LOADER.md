# Loader: Pico Loader vs. YSMenu, zurück zum Launcher aus Spielen

Recherche zu Phase 8 ([PLAN.md](PLAN.md#phase-8-loader-bei-bedarf)). Noch nichts davon ist umgesetzt.

Stand: 13.10.2026 · Basis: `LNH-team/pico-loader` develop @ `b9f923e` (18.09.2026, nach v1.7.1), DsRunner @ `36184e8`.

**Netzwerk:** github.com (git und Webseiten) war erreichbar. Die GitHub-REST-API für fremde Repos, **gbatemp.net**, **gamebrew.org**, **wiki.ds-homebrew.com** und **git.randommeaninglesscharacters.com** waren per Proxy-Policy gesperrt. Angaben zu YSMenu und AKAIO stammen daher nur aus Suchergebnis-Auszügen und sind als *unverifiziert* markiert.

---

## TL;DR

1. **Upstream kann es noch nicht:** In der README steht ausdrücklich: *"Return to loader is also currently not supported in retail games yet."* Für Homebrew gibt es einen **Bootstub** (libnds-Konvention bei `0x02FF4000`). Für Retail-Spiele gibt es nur den Patch für **`OS_ResetSystem`**. Er lädt Pico Loader neu von der SD, startet aber **dasselbe Spiel** wieder.
2. **Es gibt fast fertige Lösungen von Dritten**, beide ohne Review der Maintainer:
   - **danyocom, Branch `igr-hooks`** (Commit `a7def04`): Wer **L+R+Unten+B 2 s hält**, landet im Launcher. Der Hook sitzt im **ARM9-SDK-IRQ-Dispatcher**. Vor dem Reset nimmt der Code den **Slot-1-Lock**, damit kein Speichervorgang abgebrochen wird. Danach springt er in den vorhandenen Reboot-Code von `OS_ResetSystem`. Der Code ist **plattformunabhängig**, nutzt also den SD-Lesecode der jeweiligen Plattform und damit auch M3DS. Dazu gehören PR #216 (DLDI-Fallback-Treiber) und PR #220 (DLDI-Cache in `/_pico/dldi.bin`).
   - **PR #228** (VideoGameMore, Draft): ein ARM7-VBlank-Hook. Er enthält aber **fest eingebaute DSpico-Kartenbefehle (E3/E4/E5)**, läuft also **nur auf dem DSpico**. Er ist sehr groß (+2300 Zeilen) und wird abgeschaltet, sobald Cheats aktiv sind.
3. **Für die M3 DS Real** ist der Weg von danyocom am besten geeignet. Er setzt nur voraus, dass die Plattform ROMs über SD-Lesepatches liest. Das gilt für M3DS (`HasRomReads()` ist false, SD-Lese- und DMA-Patchcode sind vorhanden).
4. **Empfehlung:** Den Fork `HolziDape/pico-loader` von develop anlegen und `dldi-cache` (PR #220) plus `igr-hooks` übernehmen. Den Launcher-Pfad für DsRunner anpassen, M3DS bauen und mit einer Testliste auf der Hardware prüfen. Der Bau dauert etwa **1–2 Tage**, Hardwaretests **mehrere Abende**. Upstream im Auge behalten: Gericom hat eine Richtung genannt, die Funktion kommt dort voraussichtlich irgendwann.

---

## 1. Wie Pico Loader heute zum Launcher zurückkehrt

### 1.1 Wie DsRunner Pico startet
- `arm9/source/main.cpp:170`: `if (argc >= 1) pload_setLauncherPath(argv[0]);`. Der Launcher-Pfad kommt also **nur aus `argv[0]`**.
- `arm9/source/picoLoaderBootstrap.cpp`: Lädt `picoLoader9.bin` nach `0x06800000` und `picoLoader7.bin` nach `0x06840000`. Danach füllt es `pload_header7_t` aus (`loadParams.romPath/savePath/arguments`, `v2.launcherPath`, `v3.cheats`, `dldiDriver`, `bootDrive`).
- `RomBrowserController::SetPicoLoaderParams()` setzt ROM- und Save-Pfad sowie die Argumente. Cheats kommen über `pload_setCheatData`.
- **Offene Frage (Hardware):** Wird `argv[0]` überhaupt gesetzt, wenn DsRunner per Autostart läuft (`g6dsload.eng` → `r4i.sys` → `/_nds/TWiLightMenu/main.srldr`)? Wenn nicht, ist `launcherPath` leer, und Pico baut **auch bei Homebrew keinen Bootstub** ein (`pload_header7_v2_t`: *"When this path is not set, no bootstub will be patched"*). Das lässt sich mit einer Logzeile prüfen. Notfalls in DsRunner einen festen Pfad als Fallback setzen.

### 1.2 Homebrew: Bootstub
- ARM7 `NdsLoader::HandleHomebrewPatching()` (`arm7/source/loader/NdsLoader.cpp:545`) schickt `IPC_COMMAND_ARM9_SETUP_HOMEBREW_BOOTSTUB`. Wenn ein `launcherPath` gesetzt ist, legt die ARM9 (`arm9/source/main.cpp:263`) bei `HOMEBREW_BOOTSTUB` = `0x02FF4000` ab: 32 KB, im DS-Modus gespiegelt nach `0x023F4000`. Das ist die libnds-Struktur `bootSig`, `arm9Reboot`, `arm7Reboot`. Dazu kommen 16 KB DLDI-Kopie und der Launcher-Pfad.
- `BootstubPatchCode.s`: Synchronisiert ARM9 und ARM7 über `IPCSYNC` und den libnds-Reset-Code `0x0C04000C`, mappt VRAM A–D auf LCDC und liest `picoLoader9/7.bin` **über den SD-Lesecode der Plattform** mit Cluster-Maps aus `loader_info_t` (`/_pico/*.bin`). Danach setzt er `romPath` = Launcher-Pfad und startet Pico neu. **Pico bootet also den Launcher.**
- Das funktioniert nur, weil Homebrew `exit()` oder `bootstub->arm9Reboot` **selbst aufruft**. Retail-Spiele tun das nie.

### 1.3 Retail: nur `OS_ResetSystem`, und zwar zurück ins selbe Spiel
- `arm9/source/patches/arm9/OSResetSystemPatch.cpp` sucht das Ende von `OS_ResetSystem` (Signaturen für SDK 2–5 und Hybrid) und biegt es auf `OSResetSystemPatchCode.s` um. Dieser Code schickt dem Spiel-ARM7 per FIFO das SDK-Reset-Kommando (`0x4000C`), lädt Pico per SD neu nach VRAM und setzt `arm7EntryAddress` im Header auf den Pico-Einstieg.
- `arm7/source/main.cpp:224`: Wenn `romHeader->arm7EntryAddress == gLoaderHeader.entryPoint` gilt, folgt *"Retail soft reset detected"* und `BootMode::SdkResetSystem`. **Das gleiche Spiel** wird neu gestartet, `resetParam` und `romOffset` bleiben erhalten.

### 1.4 Was für Retail fehlt
1. **Auslöser:** Viele Spiele rufen `OS_ResetSystem` nie auf, etwa Mario Kart DS mit eigener Reset-Logik, oder Metal Slug 7 und The Simpsons. Nötig ist ein **Hotkey-Hook**.
2. **Ziel:** Der neu geladene Pico kennt den `launcherPath` nicht, denn `picoLoader7.bin` kommt frisch von der SD und hat ein leeres `v2`. Er muss den Launcher statt des Spiels booten. Dafür braucht es eine Markierung (z. B. `resetParam`) und einen Pfad, der den Reset übersteht.
3. **DLDI für den Launcher:** DsRunner ist Homebrew und braucht einen DLDI-Treiber. Nach einem Reboot aus dem Spiel bekommt Pico **keinen DLDI übergeben**. Lösung: PR #220 cacht ihn in `/_pico/dldi.bin`. Gericom hat diesen Weg in PR #216 selbst vorgeschlagen.

---

## 2. Wie andere Loader den In-Game-Reset lösen

| Loader | Hotkey (Standard) | Technik | Quelle |
|---|---|---|---|
| **nds-bootstrap** (TWiLight, auch B4DS-Modus auf DS Lite) | IGM: **L+Unten+Select** (`KEY_L\|KEY_DOWN\|KEY_SELECT` = `0x284`) | Der ARM7-Cardengine-Hook `myIrqHandlerVBlank` prüft `REG_KEYINPUT` und `REG_EXTKEYINPUT` (X/Y gibt es nur auf ARM7). Das In-Game-Menü öffnet sich, sofern ein ARM9-IRQ-Hook besteht und kein WLAN-IRQ aktiv ist. „Zurück zum Menü“: ARM9 `reset(0xFFFFFFFF)` liest die Boot-Datei per Cluster von der SD (`quitFileCluster`), patcht DLDI hinein und gibt ARM7 das Signal `'BOOT'`. Für das IGM wird Spiel-RAM in eine Pagefile auf der SD ausgelagert. | `retail/cardengine/arm7/source/cardengine.c:216,356`, `retail/cardengine/arm9/source/cardengine.c:208–380`, TWiLight `universal/source/common/bootstrapsettings.cpp:21` |
| **YSMenu** (RetroGameFan, Closed Source, DSTT-Kernel) | laut GameBrew/RGF-Paket **L+R+X+Y+B+A** *(unverifiziert)* | Kernel-Datei `ttpatch.dat` aus dem DSTT-Firmware-Paket. Reset lädt die Menü-Datei (`TTMENU.DAT`, bzw. `SRESET.DAT` im M3-Paket) neu. Laut Foren-Auszügen klappt der Soft-Reset nicht auf jeder Karte zuverlässig (R4 problematisch, DSTT gut). | Suchergebnis-Auszüge: GameBrew YSMenu, gbatemp-Threads (Host gesperrt) |
| **AKAIO / Wood R4** | **L+R+A+B+Unten** *(Foren, unverifiziert)*, Option „soft reset in game“, standardmäßig aus | Closed Source. Kernel-Hook in der Kartenfirmware. | gbatemp-Thread „soft reset AKAIO 1.5.1“ (gesperrt) |
| **Pico Loader PR #159** (abgelehnt) | L+R+Start+Select | Hängt an `OS_ResetSystem`, `_picoboot.nds` fest eingetragen, DLDI musste manuell gepatcht werden. **Gericom:** Nicht alle Spiele nutzen `OS_ResetSystem`. Er will stattdessen den **ARM7-VBlank-Hook der Cheats** und einen **DLDI, der aus dem Plattformcode erzeugt** wird. | https://github.com/LNH-team/pico-loader/pull/159 |
| **Pico PR #228** (Draft) | L+R+Unten+Select, 30 Frames | ARM7-Hook `OSi_IrqVBlank` an derselben Stelle wie der Cheat-Hook, daher **nicht zusammen mit Cheats nutzbar**. Übergabe per IPCSYNC, ARM9 bestätigt in Lese- und Warte-Hooks. ARM7 liest Pico **mit fest eingebauten DSpico-Befehlen** und reserviert 16 KB Arena für DLDI. Test auf DS Lite: 299 von 360 bestanden, 54 Spiele ignorieren den Hotkey. | https://github.com/LNH-team/pico-loader/pull/228 |
| **danyocom `igr-hooks`** (Fork, kein PR) | **L+R+Unten+B, 120 Frames (2 s)** | **ARM9**: Die letzten 4 Instruktionen von `OS_IrqHandler` (SDK 2–5 identisch) werden durch eigenen Code ersetzt, der bei VBlank `REG_KEYINPUT` zählt. Beim Reset: **Slot-1-Lock per `swp` nehmen** (kein laufender ROM- oder Save-Zugriff), `EXMEMCNT` auf ARM9 stellen, DMA stoppen, `resetParam = 'PIGR'` setzen und den gemeinsamen Reboot-Code von `OS_ResetSystem` anspringen. Der neue Pico-ARM7 erkennt `PIGR` und bootet `launcherPath` oder als Fallback `/_picoboot.nds`. Der Patch ist **optional**: Fehlt die Signatur oder der Platz, wird er übersprungen. | https://github.com/danyocom/pico-loader/tree/igr-hooks |

**Welche Hooks man braucht:**
- **Tasten abfragen:** Entweder im ARM7-VBlank-IRQ (dann auch X/Y über `0x04000136`) oder im ARM9-IRQ-Dispatcher (nur `0x04000130`, also ohne X/Y). Pico hat schon die ARM7-Signaturen `OSi_IrqVBlank` für Cheats (`CheatEnginePatch.cpp`).
- **Übergabe ARM7 → ARM9:** Kommt der Auslöser vom ARM7, muss die ARM9 im Spielkontext reagieren (IPCSYNC-IRQ oder Hook in Lese- und Wartefunktionen). Kommt er vom ARM9, setzt die ARM9 den ARM7 über das **SDK-PXI-Reset-Kommando** zurück. Das braucht den OS-Reset-Handler im Spiel-ARM7, den der vorhandene Pico-Reboot ohnehin voraussetzt.
- **Launcher von SD laden:** Pico liest sich selbst über Cluster-Maps und Plattform-SD-Code, ohne FAT-Treiber im Spiel. Den eigentlichen Launcher lädt dann der frisch gestartete Pico mit FatFs. DLDI für den Launcher kommt aus dem Cache.
- **Speicher auf dem DS Lite:** 4 MB Haupt-RAM, keine DSi-Erweiterung. Pico legt Patchcode in Lücken der Secure-Area-Syscall-Thunks (oft nur etwa 0x70 Byte), deshalb ist danyocoms Code zweigeteilt. Dazu kommen die Arena und 2 KB ARM7-WRAM. Der Reboot braucht nur VRAM A–D, und die wird beim Abbruch des Spiels ohnehin frei. Ein **echtes In-Game-Menü** wie bei nds-bootstrap wäre auf dem DS Lite nur mit RAM-Auslagerung auf die SD möglich. Für einen reinen Reset ist das nicht nötig.

---

## 3. Machbarkeit für Pico Loader auf der M3 DS Real

### Betroffene Komponenten (Weg von danyocom)
| Datei | Änderung |
|---|---|
| `arm9/source/patches/arm9/InGameResetPatch{.cpp,.h,Code.s,Code.h}` | neu, ca. 490 Zeilen |
| `arm9/source/patches/arm9/OSResetSystemPatch*` | Reboot als gemeinsam nutzbarer Patchcode |
| `arm9/source/patches/PatchHeap/PatchContext` | Prüfung des Platzes für optionale Patches |
| `arm9/source/Arm9Patcher.cpp` | Patch zuletzt einhängen, nur wenn `!HasRomReads()`. Für M3DS gilt das. |
| `arm7/source/main.cpp` | `resetParam == 'PIGR'` → Launcher booten |
| `arm7/source/fat/dldi.cpp`, `NdsLoader.cpp` | DLDI-Cache `/_pico/dldi.bin` (PR #220) |
| `common/inGameReset.h` | Marker-Konstante |
| **neu für DsRunner** | Fallback-Pfad statt `/_picoboot.nds`. Besser wäre, den letzten `launcherPath` analog zum DLDI-Cache in z. B. `/_pico/launcher.txt` zu speichern. |

Insgesamt ca. **1000 Zeilen Diff** auf develop (`git diff 7a13a2e danyo/igr-hooks`: 19 Dateien, +1045/−40). Davon ist das meiste fertig. Selbst zu schreiben sind nur etwa 20–50 Zeilen für Pfad und Konfiguration.

**Warum M3DS passen sollte:** `M3DSLoaderPlatform` liefert `CreateSdReadPatchCode` (Thumb, ARMv4T, also auf beiden CPUs nutzbar) und hat `HasRomReads() == false`. Der vorhandene Reboot von `OS_ResetSystem` läuft also schon heute auf M3DS. danyocom hat nur auf DS Lite mit DSpico getestet, auf AK2/2i war ein Test geplant. **Auf M3DS ist der Weg noch nicht getestet.**

### Risiken
- **Spielkompatibilität:**
  - Spiele mit eigenem IRQ-Dispatcher oder untypischer SDK-Version: Die Signatur fehlt, der Hotkey wird stillschweigend nicht eingebaut. Die Spiele selbst laufen weiter.
  - Wenig Patchplatz: Bei großen Patchsätzen (DMA-Patches für M3DS!) wird der Hotkey eventuell übersprungen.
  - Spiele, deren ARM7 auf das PXI-Reset-Kommando nicht reagiert: In #21 berichtet rasalopa, dass ein Reset ohne `OS_ResetSystem` in MKDS und Metal Slug 7 nichts tat. Das war allerdings ein anderer Ansatz. Laut danyocom funktioniert seine Variante unabhängig davon.
  - Spiele mit WLAN oder Download Play: nds-bootstrap blockt das IGM bei WLAN-IRQ.
  - Fehlausgelöste Combos: L+R+Unten+B 2 s lang ist selten. Trotzdem sollte die Combo konfigurierbar sein. **Achtung:** Ohne X/Y auf der ARM9 ist **L+R+X+Y+A+B wie bei YSMenu im ARM9-Hook nicht möglich.**
- **Speicherstände:** Pico schreibt Saves **direkt durch** auf die vorab angelegte Save-Datei (`WriteSavePatchCode.s`: Sektor lesen, ändern, schreiben, über den Plattform-SD-Code). Die FAT wird dabei nicht verändert, und es gibt keinen Cache, der geleert werden müsste. Gefährlich ist **nur ein Reset mitten in einem Sektor-Schreibvorgang**. Dagegen hilft das Warten auf den Slot-1-Lock. **Nicht gespeicherter Fortschritt im Spiel geht verloren**, genau wie beim Ausschalten. Darauf muss die Doku hinweisen.
- **DLDI-Cache:** Schreibt beim normalen Boot `/_pico/dldi.bin` (über `.tmp` mit anschließendem Umbenennen). Das ist unkritisch, solange immer derselbe M3-DLDI übergeben wird.
- **Upstream-Drift:** Die Arbeiten von danyocom und VideoGameMore sind nicht gemergt und haben kein Review. Ein eigener Fork muss bei jedem Upstream-Release neu gerebased werden. Konflikte sind zu erwarten in `Arm9Patcher.cpp`, `OSResetSystemPatch*`, `main.cpp` und `PatchHeap`.

### Upstream-Stand (Issues und PRs)
- **#21** „Add in-game hotkey and overlay“: offen, Label enhancement, keine Zusage der Maintainer. https://github.com/LNH-team/pico-loader/issues/21
- **#159** IGR/Soft-Reset: abgelehnt, Gericom hat dabei die Richtung vorgegeben (siehe oben). https://github.com/LNH-team/pico-loader/pull/159
- **#216** DLDI-Fallback-Treiber und **#220** DLDI-Cache (danyocom): offen. Gericom schlägt in #216 ausdrücklich vor, DLDI in `_pico` zu speichern, *"for example when returning to the loader from retail games"*. https://github.com/LNH-team/pico-loader/pull/216, https://github.com/LNH-team/pico-loader/pull/220
- **#228** DSpico-Hotkey: Draft, ohne Review. https://github.com/LNH-team/pico-loader/pull/228
- **#213** Rückkehr bei Homebrew auf dem DSi kaputt: offen, für DS Lite nicht relevant. https://github.com/LNH-team/pico-loader/issues/213
- Fork-Branches: https://github.com/danyocom/pico-loader/tree/igr-hooks (`a7def04`, Rebase auf `b9f923e` am 20.09.), `dldi-cache` (`84962d8`), `dldi-cache-igr-test` (`a89f108`, Reset über das SDK zurück zum Launcher). Fork von VideoGameMore: https://github.com/VideoGameMore/pico-loader (`upstream/retail-return-hotkey` = `de30578`).

---

## 4. Kompatibilitätsliste und Sammlung schlechterer Spiele

- **Eine offizielle Liste gibt es nicht.** Das DSpico-FAQ sagt sinngemäß: *"We do not have an official compatibility list"*. Die README warnt: *"there can be some game compatibility differences between different platforms"*, und ohne DMA-Kartenlesen gebe es Cache-Probleme. M3DS hat DMA-Lesen seit v1.7.0 (PR #167).
- Spielbezogene Fixes liegen in `data/aplist.csv` (Anti-Piracy), `data/savelist.csv` und `data/patchlist.json` und sind im **Upstream-Issue-Tracker** dokumentiert. M3-relevante offene Fälle sind z. B. #5 und #209 (Pokémon Dash), #101, #102 (schwarzer Bildschirm), #200 (Audioverzögerung auf DS Lite).
- Eine **YSMenu-Vergleichsliste speziell für M3/R4** wurde nicht gefunden. gbatemp war gesperrt, dort gibt es eventuell Foren-Threads.

### Vorlage: [compat/pico-vs-ysmenu.md](compat/pico-vs-ysmenu.md)

```markdown
# Kompatibilität: Pico Loader (M3DS) vs. YSMenu – DS Lite + M3 DS Real

Pico-Version: v1.7.1 (M3DS) · YSMenu-Version: 7.06 · DsRunner: v0.x
Status: ✅ läuft · ⚠️ läuft mit Fehlern · ❌ startet nicht/hängt · – nicht getestet

| Spiel (No-Intro-Name) | Gamecode | Region | Pico | YSMenu | Symptom bei Pico (wo genau?) | Reproduzierbar? | Save-Typ/Größe | Cheats an? | Upstream-Issue | Notiz |
|---|---|---|---|---|---|---|---|---|---|---|
| Mario Kart DS | AMCP | EUR | ✅ | ✅ | – | – | EEPROM 64K | nein | – | |
| Pokémon Dash | APDP | EUR | ❌ | ✅ | Weißer Bildschirm nach Save-Erstellung | immer | | nein | #5/#209 | |
```

Regeln für Einträge:
1. **Gamecode** aus dem ROM-Header (Offset `0x0C`, 4 Zeichen). Pico-Fixes werden nach Gamecode zugeordnet. Optional die CRC32, um eine schlechte Dump-Datei auszuschließen.
2. **Gegenprobe:** Dieselbe ROM-Datei mit YSMenu **und** einem frischen Save testen, damit ein alter oder fremder Save nicht das Ergebnis verfälscht (siehe #199, AK2-Saves).
3. **Symptom genau beschreiben:** Boot, Intro, Menü, Speichern, bestimmte Stelle; Grafik, Ton, Freeze, Geschwindigkeit.
4. **Vor einem Upstream-Issue** prüfen, ob das Spiel schon gemeldet ist (Suche nach Titel oder Gamecode). Plattform `M3DS` und Konsole `DS Lite` angeben.
5. Später eine dritte Spalte **nds-bootstrap (B4DS)** über TWiLight, um zu sehen, ob ein Fehler Pico-spezifisch ist.
6. Komfort-Idee: Die `playHistory` von DsRunner liefert die Liste der getesteten Spiele. Ein Export als CSV wäre eine kleine Hilfe.

Für den Soft-Reset zusätzlich eine Spalte **„Hotkey → Launcher“** (✅ / ❌ ignoriert / ⚠️ hängt / 💾 Save-Problem).

---

## 5. Empfehlung und nächste Schritte

### Ohne Hardware machbar
1. **Fork** `HolziDape/pico-loader` von `LNH-team/pico-loader@develop` anlegen. Branch `dsrunner/igr`: `danyocom/dldi-cache` (PR #220) und `danyocom/igr-hooks` per Cherry-Pick oder Merge übernehmen und Konflikte lösen.
2. **Launcher-Pfad:** Statt des Fallbacks `/_picoboot.nds` entweder
   a) beim normalen Boot `gLoaderHeader.v2.launcherPath` in `/_pico/launcher.txt` sichern (wie den DLDI-Cache) und nach `PIGR` wieder einlesen. Das ist sauber und auch für Upstream geeignet. Oder
   b) als Übergangslösung einen festen Pfad per Makefile-Variable, z. B. `/_nds/TWiLightMenu/main.srldr`, wie im DsRunner-Autostart.
3. **DsRunner:** Fehlt `argv[0]`, einen festen Pfad als Fallback in `main.cpp` setzen und den Launcher-Pfad loggen. Optional ein Argument wie `-igr` übergeben, damit DsRunner nach dem Reset z. B. direkt das zuletzt gespielte Spiel markiert. Diese Idee stammt aus #159.
4. **CI:** `make PICO_PLATFORM=M3DS` mit BlocksDS und .NET 9 im Release-Paket, `_pico/` mit den neuen Binaries. Prüfen, dass `patchlist.bin` und die anderen Listen weiter gebaut werden.
5. **melonDS-Test:** Die Plattform `MELONDS` mit demselben Branch bauen. So lassen sich der Hotkey und die Rückkehr ohne M3 testen. Laut PLAN laufen Spiele mit dem M3DS-Loader in melonDS nicht, wohl aber mit der Plattform MELONDS.
6. **Kompatibilitätsvorlage** anlegen (siehe Abschnitt 4).

### Nur mit Hardware (DS Lite + M3 DS Real)
1. Smoke-Test: Pico normal starten, danach `/_pico/dldi.bin` vorhanden? Ein Spiel starten, L+R+Unten+B 2 s halten: Erscheint DsRunner, und hat er SD-Zugriff?
2. Testmatrix aus etwa 20–30 Spielen über SDK 2, 3, 4, 5 und Hybrid, mit und ohne eigenes `OS_ResetSystem` (MKDS, NSMB, Pokémon DP/HGSS/SW, Metal Slug 7, Animal Crossing, Spiele mit Download Play). Spiele mit **DMA-Patches** sind wegen des Patchplatzes besonders wichtig.
3. **Save-Test:** Im Spiel speichern, *während* des Speicherns den Hotkey halten. Der Reset muss bis nach dem Schreiben warten. Danach den Save mit Pico **und** YSMenu laden.
4. Mit aktiven Cheats testen (verträgt sich der ARM9-Hook mit dem ARM7-Cheat-Hook?).
5. Ergebnisse in die Kompatibilitätsliste eintragen und Fehler an danyocom bzw. im Upstream (Kommentar in #21 oder #220) melden.

### Aufwand (ehrliche Schätzung)
| Schritt | Aufwand |
|---|---|
| Fork, Merge der Branches von danyocom, Launcher-Pfad, M3DS-Build und CI | **1–2 Tage** (Konfliktlösung und Toolchain-Einrichtung sind der unsichere Teil) |
| Hardwaretests (Smoke-Test plus 20–30 Spiele) | **3–5 Abende** |
| Fehlerbehebung bei Spielen ohne Hotkey (Signaturen oder Platz) | offen. Pro Spiel Stunden bis Tage, Debugging nur über no$gba/melonDS oder Logs. Eher upstream melden. |
| PR #228 auf M3DS portieren (DSpico-Befehle durch M3DS-Befehle ersetzen) | **1–2 Wochen**, nicht empfohlen: großer Diff mit Diagnose-Code, kollidiert mit Cheats |
| Eigenes ARM7-IGR mit Menü wie nds-bootstrap | **mehrere Wochen**, auf dem DS Lite wegen RAM kaum sinnvoll |

**Fazit:** Die Funktion lässt sich gut umsetzen, wenn man auf den Branches von danyocom aufbaut. Diese passen zu Gericoms Vorgaben aus #159 und #216: DLDI-Cache und `launcherPath` statt festem Pfad. Eine Abweichung gibt es: Der Hook sitzt auf der ARM9, Gericom wollte ihn auf der ARM7. Mit dem Fork kein „Pico-Neuschreiben“ anfangen, sondern nah an Upstream bleiben. Wenn LNH das Feature selbst mergt, wird der Fork wieder überflüssig.

---

## Quellen
- Pico Loader README und Code: https://github.com/LNH-team/pico-loader (u. a. `include/picoLoader7.h`, `arm9/source/patches/homebrew/BootstubPatchCode.s`, `arm9/source/patches/arm9/OSResetSystemPatch*.{cpp,s}`, `arm7/source/main.cpp`, `arm9/source/patches/arm7/cheats/CheatEnginePatch.cpp`, `arm9/source/patches/arm7/WriteSavePatchCode.s`, `arm9/source/patches/platform/m3ds/`)
- Issues und PRs: #21, #159, #213, #216, #220, #228, #167 (siehe Links oben)
- danyocom-Fork: https://github.com/danyocom/pico-loader (Branches `igr-hooks`, `dldi-cache`, `dldi-cache-igr-test`)
- VideoGameMore-Fork: https://github.com/VideoGameMore/pico-loader
- nds-bootstrap: https://github.com/DS-Homebrew/nds-bootstrap (`retail/cardengine/arm7/source/cardengine.c`, `retail/cardengine/arm9/source/cardengine.c`)
- TWiLight Menu++: https://github.com/DS-Homebrew/TWiLightMenu (`universal/source/common/bootstrapsettings.cpp`, IGM-Hotkey Standard `0x284`)
- YSMenu und AKAIO (nur Suchergebnis-Auszüge, Hosts gesperrt): https://www.gamebrew.org/wiki/YSMenu, https://gbatemp.net/download/retrogamefan-multi-cart-update.35737/, https://gbatemp.net/threads/software-reset-on-ysmenu.215702/, https://gbatemp.net/threads/soft-reset-akaio-1-5-1.202212/
- Keine offizielle Kompatibilitätsliste: DSpico-FAQ (laut Suchergebnis), https://blog.omgmog.net/post/ds-pico-flashcart/
- DsRunner: `arm9/source/main.cpp:170`, `arm9/source/picoLoaderBootstrap.cpp`, `arm9/source/romBrowser/RomBrowserController.cpp:508`, `docs/PLAN.md:58,172`
