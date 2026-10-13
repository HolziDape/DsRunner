# Architektur von GBARunner2

Ein Lern-Dokument: Wie bringt GBARunner2 GBA-Spiele auf dem DS zum Laufen, und wo liegt im Code was?
Die Angaben beziehen sich auf den **DS-Build** (4 MB RAM). DSi/3DS nutzen den größeren RAM ab `0x0C000000`.

## Grundidee: Hypervisor statt Emulator

Der DS enthält mit dem **ARM9** (ARM946E-S, 67 MHz) eine CPU, die GBA-Code (ARMv4T) **nativ** ausführen kann. Außerdem liegen viele GBA-Speicherbereiche auf dem DS an **denselben Adressen**:

| GBA-Bereich | Adresse | Auf dem DS |
|---|---|---|
| EWRAM 256 KB | `0x02000000` | Anfang des DS-Haupt-RAM, deshalb beginnen die ROM-Daten erst bei `0x02040000` |
| IWRAM 32 KB | `0x03000000` | Shared WRAM |
| Palette | `0x05000000` | Palette (gleiches Format) |
| VRAM | `0x06000000` | DS-VRAM-Bänke |
| OAM | `0x07000000` | OAM |
| IO | `0x04000000` | **anders**, muss emuliert werden |
| ROM | `0x08000000…` | **existiert nicht**, muss umgeleitet werden |

GBARunner2 lässt das Spiel also direkt auf der CPU laufen und fängt nur die Zugriffe ab, die auf dem DS anders funktionieren. Dafür nutzt es die **MPU** (Memory Protection Unit, `cp15`). Sie ist in `arm9/source/setup.s` (ab ca. Zeile 190) konfiguriert:

- **Region 0** deckt nur `0x00000000–0x07FFFFFF` ab. Jeder Zugriff auf den GBA-ROM-Bereich `0x08000000+` löst deshalb einen **Abort** aus.
- **Region 1** sperrt den **IO-Bereich** `0x04000000`. Jeder IO-Zugriff des Spiels löst einen **Data Abort** aus.
- Einige VRAM- und OAM-Regionen sind schreibgeschützt, weil der DS dort keine Byte-Schreibzugriffe kann, der GBA aber schon.

Ablauf eines abgefangenen Zugriffs:
1. Das Spiel liest z.B. `REG_DISPCNT` oder ein ROM-Byte → Abort.
2. Der Abort-Handler (`arm9/source/aborthandler_asm.s`) dekodiert den Befehl (`emu/cpuArm*.s`, `emu/cpuThumb.s`) und bestimmt Register, Adresse und Breite.
3. Über Sprungtabellen (`emu/handle_address_read_table.s` / `…_write_table.s`, ins DTCM kopiert) landet er im passenden Emulationscode: `gfx.s`, `snd.s`, `dma.s`, `timer.s`, `irq.s`, `input.s`.
4. Das Ergebnis kommt ins Zielregister, danach springt der Handler hinter den Befehl zurück.

**Code-Ausführung aus dem ROM:** Springt das Spiel in den ROM-Bereich, löst das einen *Instruction Abort* aus. `instruction_abort_handler` in `setup.s` rechnet die Adresse auf die Kopie im Haupt-RAM um (`GBA_ADDR_TO_DS_HIGH/LOW`) und läuft dort weiter.

## Speicherlayout (DS)

Definiert in `common/common_defs.s`. Die Datei wird von ARM7 **und** ARM9 sowie von C **und** Assembler eingebunden.

```
0x02000000  GBA EWRAM (256 KB)
0x02040000  ROM-Daten: die ersten 2 MB des Spiels (rom3M: 3 MB)
0x02240000  GBARunner-Daten (rom3M: 0x02340000)
              SD-Cache (1424 KB, rom3M: 400 KB): ROM-Teile jenseits der ersten 2/3 MB
              Cluster-Tabellen, Cache-Verwaltung, sd_info
              Sound-Emulations-Queue, Save-Arbeitsbereich
0x023E0000  Save-Daten (128 KB)
0x02400000  Ende
```

ROM-Zugriffe jenseits von `ROM_DATA_LENGTH` gehen an den SD-Cache (`arm9/source/sd_access.cpp`, `sd_access_asm_itcm.s`). Fehlt ein Block, wird er per DLDI nachgeladen. Je größer der Cache, desto seltener sind SD-Zugriffe, also gibt es weniger Ruckler.

### Wo der Code liegt (ARM9)

Die Platzierung hängt vom **Dateinamen** ab (Linker-Script `arm9/ds_arm9_gbarunner.ld`):

| Dateiname | Region | Größe | Zweck |
|---|---|---|---|
| `*.itcm*`, `.itcm`-Sections | ITCM | 32 KB | Heißer Pfad: Abort-Handler, Cache-Zugriff |
| `*.vram.*`, `PUT_IN_VRAM` | VRAM `0x06800000` | 256 KB | Alles andere: UI, FatFS, Settings, Patches, Save-Erkennung |
| DTCM | `0x0B000000` | 16 KB | Stacks, Sprungtabellen |
| Rest | EWRAM | – | Startcode, wird nach dem Start nicht mehr gebraucht |

`./build.sh` zeigt nach jedem Build die Auslastung an. Stand Fork-Start: **VRAM ≈ 61 %, ITCM ≈ 85 %.** Neue UI und neue Features müssen mit dem freien VRAM (≈ 100 KB) auskommen.

Grafiken und Fonts aus `arm9/data/` (`.ntft`, `.nbfc`, `.nbfp`, `.nbfs`) werden per `bin2o` eingebunden, und ihre Section wird auf `.vram` umbenannt (siehe `arm9/Makefile`).

## ARM7 vs. ARM9

| | ARM7 (`arm7/`) | ARM9 (`arm9/`) |
|---|---|---|
| Aufgabe | Sound-Ausgabe und GBA-Sound-Mixing (`sound.cpp`, `gbsound.cpp`), RTC, Save-Rückschreiben, Tasten X/Y/Touch, **DLDI** im `ARM7_DLDI`-Build | Spiel ausführen, IO-Emulation, Grafik, UI, FatFS |
| Kommunikation | IPC-FIFO (`common/fifo.h`) und gemeinsame, ungecachte Speicherbereiche (`*_uncached` in `common_defs.s`) | |

**arm7dldi vs. arm9dldi:** Gibt an, welche CPU den Flashcard-Treiber (DLDI) ausführt. Manche Karten funktionieren nur mit einer der beiden Varianten. Im arm9-Build braucht der ARM9 mehr VRAM (≈ 67 %).

## Startablauf

1. `arm9/source/setup.s` (`gba_setup`): kopiert Code nach ITCM, VRAM und DTCM, richtet MPU und Caches ein, wartet auf den ARM7.
2. `initSystem()` (`init.cpp`) → `sd_init()` (`sd_access.cpp`):
   - startet die UI (`UIContext`) und mountet die SD (FatFS)
   - legt `/_gba` an, lädt das BIOS (`bios/bios.vram.cpp`: `bios.bin` suchen, sonst Normmatt-BIOS)
   - `settings_initialize()` liest `/_gba/gbarunner2.ini`
   - **argv-Boot**: Hat ein Loader einen Pfad übergeben (z.B. TWiLight Menu++), startet direkt diese ROM
   - sonst Schleife aus `FileBrowser` (ROM-Auswahl) und `SettingsScreen` (Taste R)
3. `gbab_loadRom()` (`gbaBoot.vram.cpp`): ROM öffnen, Save-Typ suchen (`save/Save.vram.cpp` → `save_findTag`), Save laden oder anlegen, Spiel-Patches anwenden (`gamePatches.vram.cpp`), Rahmen laden
4. Cache initialisieren, GPIO/RTC (`emu/romGpio.vram.cpp`), Grafik aufsetzen → Sprung ins Spiel

## Wichtige Stellen für typische Änderungen

| Vorhaben | Datei(en) |
|---|---|
| Neuer Eintrag in den Einstellungen | `arm9/source/gui/SettingsScreen.vram.cpp` (Tabellen `s*Items`), Variable in `settings.h/.vram.cpp` (laden und speichern in der ini) |
| About-Screen | `sInfoItems` in `SettingsScreen.vram.cpp`. Fork-Name kommt aus `fork.mk`. |
| In-Game-Menü | `arm9/source/gui/InGameMenu.vram.cpp` |
| ROM-Browser | `arm9/source/gui/FileBrowser*.vram.cpp` |
| Spiel braucht RTC | Game-Code in die Tabelle in `arm9/source/emu/romGpio.vram.cpp` eintragen |
| Spiel-spezifischer Code-Patch | `arm9/source/gamePatches.vram.cpp` (nach Game-Code, Beispiel: DBZ Buu's Fury) |
| Save-Typ-Erkennung | `arm9/source/save/` |
| Speicheraufteilung / Cache | `common/common_defs.s` (beide CPUs neu bauen!) |
| UI-Grundbausteine | `arm9/source/gui/core/` (UIManager, ListRecycler, OAM-/Paletten-/VRAM-Manager, NtftFont) |

## Build-Flags

| Flag | Wirkung |
|---|---|
| `ARM7_DLDI=1` | DLDI-Treiber läuft auf dem ARM7 |
| `ROM3M=1` | DS: 3 MB ROM im RAM, 400 KB SD-Cache |
| `USE_DSI_16MB=1` / `USE_3DS_32MB=1` | DSi- bzw. 3DS-Speicherlayout (Haupt-RAM ab `0x0C000000`) |
| `ISNITRODEBUG=1` | Build für IS-NITRO-Debughardware (`Makefile_isnitro`) |
