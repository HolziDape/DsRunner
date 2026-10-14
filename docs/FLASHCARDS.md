# Flashkarten: was wo funktioniert

DsRunner wird auf einem **DS Lite mit der M3 DS Real** entwickelt und getestet. Auf anderen Karten ist bisher nichts getestet. Diese Seite sagt, was auf welcher Karte gehen **sollte**, warum, und wie man DsRunner dort einrichtet. Die Kurzfassung steht im [README](../README.md#kompatible-flashkarten).

Stand: 14.10.2026 · Pico Loader `develop` @ `b9f923e` (v1.7.1 + Fixes, siehe [loader/UPSTREAM](../loader/UPSTREAM)) · TWiLight Menu++ v27.25.1

> [!CAUTION]
> **Die falsche Pico-Loader-Plattform kann die Flashkarte beschädigen** (Warnung aus dem Pico-Loader-README). Nimm nur eine Plattform, unter der deine Karte unten oder im [Pico-Loader-README](https://github.com/LNH-team/pico-loader#supported-platforms) **mit Namen** steht. Im Zweifel nicht raten.

## Was von der Karte abhängt und was nicht

DsRunner besteht aus mehreren Teilen. Wirklich von der Flashkarte abhängig sind nur zwei davon:

| Teil | Hängt von der Karte ab? | Warum |
|---|---|---|
| **DsRunner selbst** (das Menü) | nein | Ein normales DS-Homebrew-Programm (BlocksDS) mit DLDI. Es läuft auf jeder Karte, die Homebrew starten kann, auch auf DSi und 3DS im DS-Modus. |
| **Spiele starten** | **ja** | DsRunner startet alles (DS-Spiele, Homebrew, Emulatoren) über [Pico Loader](https://github.com/LNH-team/pico-loader). Pico Loader gibt es nur für die Karten in der [Tabelle unten](#kompatible-karten), jeweils als eigene Datei `picoLoader9.bin` („Plattform“). Ohne passende Plattform startet DsRunner zwar, aber kein Spiel. |
| **Autostart** beim Einschalten | **ja** | Jede Karte startet beim Einschalten ihre eigene Menü-Datei. TWiLight Menu++ liefert für viele Karten Ersatzdateien, die stattdessen `/_nds/TWiLightMenu/main.srldr` starten. Dort liegt dann DsRunner. Siehe [Autostart](#autostart). |
| **Zurück zu DsRunner aus dem Spiel** | kaum | Unser angepasster Pico Loader ([loader/](../loader/README.md)) baut den Hotkey für alle Plattformen gleich ein. Er braucht nur den SD-Lesecode, den Pico ohnehin für die Plattform hat. Ausnahme: die Plattform `R4`, siehe [unten](#zurück-aus-dem-spiel). Getestet nur auf der M3 DS Real. |
| **Emulatoren** (GBA, NES, Game Boy, SNES) | nein, fast | Normale Homebrew-Programme, laufen auf jeder Karte. Nur bei GBARunner2 gibt es je nach Karte eine passendere Variante, siehe [Emulatoren](#emulatoren). |
| **DsRunner Art** (PC-Programm) | nein | Läuft auf dem PC und schreibt nur Bilder auf die SD-Karte. |
| **DsRunner Stream** | nein | DS-Homebrew mit WLAN (dswifi). Läuft auf DS, DS Lite, DSi und 3DS im DS-Modus. Im DS-Modus kann das WLAN nur offene Netze und WEP, kein WPA. |

## Kompatible Karten

Alle Karten, für die es eine Pico-Loader-Plattform gibt. Namen und Klone stehen so im [Pico-Loader-README](https://github.com/LNH-team/pico-loader#supported-platforms); die Autostart-Ordner stammen aus `TWiLightMenu-Flashcard.7z` (v27.25.1, Ordner `Autoboot`).

| Karte (bekannte Namen und Klone) | Plattform | Spiele starten | Autostart | Zurück aus dem Spiel | Getestet |
|---|---|---|---|---|---|
| **M3 DS Real** | `M3DS` | ja | ja, fertiges Paket `DsRunner_M3DSReal.zip` | ja | **ja** (DS Lite) |
| M3i Zero (nicht GMP-Z003), iTouchDS, r4rts.com, r4isdhc.com RTS (schwarz) | `M3DS` | ja | ja, dasselbe Paket (TWiLight-Ordner „M3 DS Real, M3i Zero (non-GMP-Z003), iTouchDS, R4(i) SDHC (r4rts.com)“; r4isdhc.com RTS steht dort nicht) | ja | ungetestet |
| M3i Zero (GMP-Z003) | `G003` | ja | ja, Ordner „M3i Zero (GMP-Z003)“, startet `/BOOT.NDS` | ja | ungetestet |
| Acekard 2, 2.1, 2i, r4ids.cn und Ableger | `AK2` | ja | ja, Ordner „Acekard 2i & Acekard 2.1“ (`akmenu4.nds`); für r4ids.cn-Karten nennt TWiLight den Ordner „R4 Ultra“ | ja | ungetestet |
| Acekard RPG (SD) | `AKRPG` | ja | nein, kein TWiLight-Ordner | ja | ungetestet |
| Ace3DS+, Gateway 3DS (blau), r4isdhc.com.cn, r4isdhc.hk ab 2020 und Ableger | `ACE3DS` | ja | ja, Ordner „Ace3DS+“ bzw. „Gateway Blue“ | ja | ungetestet |
| DSTT, SuperCard DSONE SDHC, r4isdhc.com ab 2014, r4i-sdhc.com und Ableger | `DSTT` | ja | ja, Ordner „DSTT“, „SuperCard DSONE SDHC & SuperCard DSONEi“ oder „R4i-SDHC“ (siehe `Autoboot/README.txt`) | ja | ungetestet |
| r4idsn.com | `R4iDSN` | ja | ja, Ordner „R4iDSN“ | ja | ungetestet |
| Original R4DS (ohne SDHC), M3 DS Simply | `R4` | ja, ohne DMA | ja, Ordner „Original R4 & M3 Simply“ | nur in Spielen mit eigenem Neustart, siehe [unten](#zurück-aus-dem-spiel) | ungetestet |
| Stargate 3DS (DS-Modus) | `STARGATE` | ja | ja, Ordner „Stargate 3DS“, startet `/BOOT.NDS` | ja | ungetestet |
| DSpico | `DSPICO` | ja | ja, ohne TWiLight: `DsRunner.nds` als `/_picoboot.nds` | ja | ungetestet |
| EZ-Flash Parallel | `EZP` | ja, ohne DMA | ja, Ordner „EZ Flash Parallel“ | ja | ungetestet |
| DATEL Games n' Music, Action Replay DS(i) Media Edition | `DATEL` | ja, ohne DMA | Games n' Music: Ordner „Games N Music“; Action Replay: nein | ja | ungetestet |
| SuperCard SD, Lite, Rumble, SuperChis (Slot 2) | `SUPERCARD` | ja, ohne DMA | nein | ja | ungetestet |
| SuperCard CF (Slot 2) | `SUPERCARDCF` | ja, ohne DMA | nein | ja | ungetestet |
| M3 Compact Flash (Slot 2) | `M3CF` | ja, ohne DMA | nein | ja | ungetestet |
| DATEL Max Media Dock CF (Slot 2) | `MMCF` | ja, ohne DMA | nein | ja | ungetestet |
| GBA Movie Player CF (Slot 2) | `MPCF` | ja, ohne DMA | mit PassMe/FlashMe: Ordner „GBAMP + PassMe, FlashMe or WifiMe“ | ja | ungetestet |

**„ungetestet“ heißt: sollte funktionieren, hat aber noch niemand ausprobiert.** Rückmeldungen (auch „geht“) gerne als [Issue](https://github.com/HolziDape/DsRunner/issues).

- **„ohne DMA“:** Pico Loader liest auf dieser Plattform Spieldaten ohne DMA. Laut Pico-Loader-README können dann manche Spiele Cache-Probleme haben. Spiele laufen also eventuell schlechter als auf Karten mit DMA.
- **Slot-2-Karten** (GBA-Steckplatz) brauchen am DS wie immer einen Weg, Slot 2 zu starten (z. B. FlashMe oder PassMe). Das hat mit DsRunner nichts zu tun.
- **Nicht für Spieler:** `ISNITRO` (Nintendo-Entwicklerhardware IS-NITRO-EMULATOR) und `MELONDS` (nur zum Testen im Emulator melonDS). Beide baut Pico Loader, wir aber nicht für Releases.
- Die Herstellerseite auf dem Kartenetikett (z. B. r4isdhc.com) unterscheidet oft Karten, die gleich heißen, aber verschiedene Technik haben. Welche „R4i“ welche ist, steht im Pico-Loader-README und in `Autoboot/README.txt` von TWiLight Menu++.

### Karten ohne Pico-Loader-Plattform

DsRunner würde auf diesen Karten starten, könnte aber **kein Spiel starten**, weil es keine passende Pico-Loader-Plattform gibt. Für sie ist [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu) die bessere Wahl.

- **Keine Plattform** (laut TWiLight-Build-Skript jeweils ein eigener DLDI-Treiber, den keine Pico-Plattform abdeckt): SuperCard DSTWO, SuperCard DSONE (ohne SDHC), EZ-Flash V und Vi, N-Card, iSmart Premium, r4i.cn.
- **Nicht namentlich genannt:** N5 / R4-PLUS, R4DS Pro (r4dspro.com), Wood R4igold.cc Upgrade, Galaxy Eagle, EDGEi und viele weitere R4-Klone. Laut dem Build-Skript von TWiLight Menu++ nutzen ihre Startdateien den DLDI-Treiber des Original-R4 (N5 / R4-PLUS) bzw. des Acekard 2 (die anderen). Ebenso nutzen die Klone im TWiLight-Ordner „R4 Ultra“ den Acekard-2-Treiber und die im Ordner „DSTT“ den DSTT-Treiber. Das ist ein **Hinweis, keine Freigabe**: Pico Loader braucht mehr als den DLDI-Treiber, und eine falsche Plattform kann die Karte beschädigen. Wer es trotzdem versucht, tut es auf eigenes Risiko.

### DSi und 3DS

- **Mit Flashkarte:** Läuft die Karte auf DSi/3DS (z. B. Ace3DS+, DSTTi, Stargate 3DS), startet sie im DS-Modus. Dann gilt alles wie auf dem DS: dieselbe Plattform, dieselben Dateien. Der Autostart startet DsRunner, sobald die Karte im Systemmenü angetippt wird.
- **Von der SD-Karte der Konsole** (DSi mit Unlaunch/hiyaCFW, 3DS mit TWiLight Menu++): DsRunner bringt von Pico Launcher den Zugriff auf die DSi-SD-Karte mit. Pico Loader kann von dort aber laut seinem README **keine Retail-Spiele** starten, nur Homebrew. Dafür ist DsRunner nicht gedacht und das ist ungetestet. Auf DSi und 3DS ohne Flashkarte ist TWiLight Menu++ (mit nds-bootstrap) die bessere Wahl.

## Einrichten auf einer anderen Karte

Du brauchst aus dem [Release](https://github.com/HolziDape/DsRunner/releases/latest):

- `DsRunner.zip`: DsRunner, die Themes in `_pico` und die Emulatoren
- `PicoLoader-DsRunner_<PLATTFORM>.zip` für deine Plattform aus der Tabelle, z. B. `PicoLoader-DsRunner_DSTT.zip`: Pico Loader mit „zurück zu DsRunner aus dem Spiel“ (ab dem nächsten Release, siehe [CHANGELOG](../CHANGELOG.md))

Schritte:

1. **SD-Karte sichern**, vor allem alle Spielstände (`.sav`) und die Menü-Dateien deiner Karte.
2. `DsRunner.zip` entpacken und den **Inhalt** ins Hauptverzeichnis der SD-Karte kopieren.
3. `PicoLoader-DsRunner_<PLATTFORM>.zip` entpacken und den Inhalt ebenfalls ins Hauptverzeichnis kopieren. Danach liegen in `/_pico` die fünf Dateien `picoLoader7.bin`, `picoLoader9.bin`, `aplist.bin`, `patchlist.bin` und `savelist.bin`.
   - Gibt es für deine Plattform (noch) keine solche ZIP-Datei: die Datei `Pico_Loader_<PLATTFORM>.zip` aus den [Pico-Loader-Releases](https://github.com/LNH-team/pico-loader/releases) nehmen und dieselben fünf Dateien nach `/_pico` kopieren. Alles geht dann, nur „zurück zu DsRunner aus dem Spiel“ nicht.
4. Spiele nach `/Games/NDS`, `/Games/GBA` usw. legen, wie im [README](../README.md#ordner-für-die-spiele).
5. `DsRunner.nds` im Menü deiner Karte starten. Oder den Autostart einrichten (nächster Abschnitt).

### Autostart

Fertig eingerichtet ist der Autostart nur im Paket für die M3 DS Real. Für andere Karten geht es von Hand, mit den Autostart-Dateien von TWiLight Menu++:

1. Von [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu/releases) die Datei `TWiLightMenu-Flashcard.7z` herunterladen (DsRunner nutzt Version v27.25.1) und mit 7-Zip öffnen.
2. Im Ordner `Autoboot` den Ordner deiner Karte suchen. Welche Klone zu welchem Ordner gehören, steht in `Autoboot/README.txt`.
3. **Die Menü-Dateien deiner Karte sichern**, die gleich überschrieben werden (z. B. `TTMenu.dat`, `_DS_MENU.DAT`, `akmenu4.nds`). Mit ihnen kommst du zum alten Menü zurück.
4. Den **Inhalt** dieses Ordners ins Hauptverzeichnis der SD-Karte kopieren und ersetzen.
5. `DsRunner.nds` kopieren nach **`/_nds/TWiLightMenu/main.srldr`** (Ordner anlegen, Datei umbenennen). Die TWiLight-Startdateien starten genau diese Datei.
   - **Ausnahmen:** Die Ordner „Stargate 3DS“, „M3i Zero (GMP-Z003)“ und „Original R4SDHC“ starten stattdessen **`/BOOT.NDS`**. Dann `DsRunner.nds` als `/BOOT.NDS` ablegen.
   - **DSpico:** Keine TWiLight-Dateien nötig. Die DSpico startet beim Einschalten `/_picoboot.nds`, also `DsRunner.nds` dorthin kopieren und so umbenennen.
6. Einschalten. DsRunner startet von selbst.

**Hast du TWiLight Menu++ installiert?** Dann gehört `/_nds/TWiLightMenu/main.srldr` zu TWiLight Menu++. Wer sie durch DsRunner ersetzt, startet TWiLight Menu++ nicht mehr. Vorher sichern; zum Zurückwechseln die gesicherte Datei zurückkopieren.

**Update:** neue `DsRunner.nds` wieder an dieselbe Stelle kopieren (`main.srldr`, `BOOT.NDS` oder `_picoboot.nds`). Steht im Changelog „Pico Loader geändert“, auch die fünf Dateien aus der neuen `PicoLoader-DsRunner_<PLATTFORM>.zip`.

**Zurück zum alten Menü:** die gesicherten Menü-Dateien zurückkopieren.

Ungetestet ist hier alles außer der M3 DS Real. Die Kette ist aber dieselbe wie bei TWiLight Menu++: Startdatei der Karte → TWiLight-Startprogramm → `/_nds/TWiLightMenu/main.srldr`. Geprüft haben wir das an den Dateien selbst: Alle Startdateien im Ordner `Autoboot` außer den drei Ausnahmen enthalten den Pfad `fat:/_nds/TWiLightMenu/main.srldr` oder werden laut TWiLight-Build-Skript aus demselben Startprogramm gebaut (die verschlüsselten Dateien für R4, Ace3DS+ und Gateway).

## Zurück aus dem Spiel

„Zurück zu DsRunner“ (L + R + Unten + B, zwei Sekunden) steckt in unserem angepassten Pico Loader. Der Code ist für alle Plattformen derselbe (Patches 0001 bis 0016, [loader/README.md](../loader/README.md)). Er hängt sich in den Interrupt-Code des Spiels und lädt beim Hotkey Pico Loader mit dem SD-Lesecode der Plattform neu, der dann DsRunner startet.

- **Alle Plattformen außer `R4`:** Der Hotkey wird eingebaut, wenn das Spiel passt (wie auf der M3, siehe `/_pico/igr-log.txt`).
- **`R4` (Original R4, M3 DS Simply):** Diese Plattform liest Spiele direkt von der Karte statt per SD-Lesecode. Den braucht der Hotkey aber für den Neustart. Er geht deshalb nur in Spielen, die selbst einen Neustart eingebaut haben (`OS_ResetSystem`), denn dafür legt Pico den SD-Lesecode ohnehin an. In allen anderen steht im Log „no sd read code for the reboot“.
- Nach dem Hotkey sucht Pico DsRunner unter dem Pfad, mit dem DsRunner das Spiel gestartet hat (`/_pico/launcher.txt`), sonst unter `/_nds/TWiLightMenu/main.srldr`, `/BOOT.NDS` und `/_picoboot.nds`. Das passt zu allen Autostart-Varianten oben.
- Getestet ist das nur auf der M3 DS Real. Auf anderen Karten bitte zuerst mit einem Spiel ohne wichtigen Spielstand ausprobieren.

## Emulatoren

Die mitgelieferten Emulatoren (GBARunner2, nesDS, GameYob, SNEmulDS) sind normale DS-Homebrew-Programme mit DLDI und laufen auf jeder Karte aus der Tabelle.

**GBARunner2** gibt es in Varianten, die den DLDI-Treiber der Karte auf verschiedenen Prozessoren ausführen. Mitgeliefert ist `GBARunner2_arm7dldi_ds.nds` (ARM7). TWiLight Menu++ beschreibt die Wahl so: „Select ARM7 if ARM9 causes a white screen crash“. Je nach Karte läuft also eine der beiden Varianten besser. Bleibt ein GBA-Spiel bei dir weiß oder startet nicht:

1. Aus dem [GBARunner2-Release](https://github.com/Gericom/GBARunner2/releases) die Datei `GBARunner2_arm9dldi_ds.nds` holen.
2. In `/_dsrunner/emulators/` die Datei `GBARunner2_arm7dldi_ds.nds` wegnehmen (DsRunner nimmt sonst weiter sie) und `GBARunner2_arm9dldi_ds.nds` hineinlegen.

Welche Karte welche Variante braucht, steht in der [GBARunner2-Kompatibilitätsliste](https://wiki.gbatemp.net/wiki/GBARunner2) (konnten wir nicht abrufen, unverifiziert). Für DSi/3DS im DSi-Modus gibt es eigene Varianten (`_dsi`, `_3ds`); die betreffen DsRunner auf der Flashkarte nicht.

## Pico Loader selbst bauen

Für Entwickler: `PICO_PLATFORM=<PLATTFORM> loader/build.sh <Ausgabeordner>` baut unseren Pico Loader für jede Plattform aus der Tabelle (Docker reicht, siehe [loader/README.md](../loader/README.md)). Am 14.10.2026 haben alle 19 Plattformen von Pico Loader damit gebaut, auch `ISNITRO` und `MELONDS`. Der Release-Workflow baut die 17 Plattformen aus dem Pico-Loader-Release und hängt je eine `PicoLoader-DsRunner_<PLATTFORM>.zip` an.

## Quellen

- Pico Loader, Commit `b9f923e`: `README.md` (Plattformen, Klone, DMA, DSi-SD-Hinweis), `arm9/source/patches/platform/LoaderPlatformFactory.cpp` (alle 19 Plattformen), `.github/workflows/release.yml` (17 Release-Plattformen), `R4LoaderPlatform.h` (`HasRomReads() == true` nur bei R4)
- Unsere Patches: `loader/patches/0005-…` (Hotkey nur mit SD-Lesecode der Plattform oder vorhandenem `OS_ResetSystem`), `0013-…` (Rückkehrpfade)
- TWiLight Menu++ v27.25.1, `TWiLightMenu-Flashcard.7z`: Ordner `Autoboot` und `Autoboot/README.txt`; Quellcode `booter_fc/Makefile` (welche Startdatei mit welchem DLDI-Treiber gebaut wird) und `booter_fc/arm9/source/main.cpp` (startet `/_nds/TWiLightMenu/main.srldr`)
- Pico Launcher (Original-README): DSpico startet `/_picoboot.nds`
- GBARunner2 `readme.md`; TWiLight Menu++ `settings` (Text zur ARM7/ARM9-Einstellung)
- Nicht erreichbar und deshalb nicht geprüft: gbatemp.net (GBARunner2-Kompatibilitätsliste), wiki.ds-homebrew.com (Flashkarten-Liste von TWiLight Menu++)
