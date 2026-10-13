# Emulator-Workspace

Liegt außerhalb des Repos (Standard: `H:\dev\ds\DsRunner-emu`) und wird mit `tools\setup-emu.ps1` angelegt.

```
DsRunner-emu\
  melonDS\        melonDS 1.1 (portabel, Konfiguration in melonDS.toml)
  mGBA\           mGBA 0.10.5 (portabel) als GBA-Referenz
  sd\             Inhalt der emulierten SD-Karte (wird mit sd.img synchronisiert)
    DsRunner.nds
    _gba\bios.bin
    gba\          GBA-ROMs, hier liegt auch die Test-ROM
  sd.img          von melonDS erzeugt
```

## Benutzung

```powershell
.\tools\setup-emu.ps1 -Bios H:\dev\ds\ds-lite-backup\bios.bin   # einmalig
.\tools\emu-run.ps1 -Build                            # bauen, kopieren, melonDS starten
```

Tastatur in melonDS: **X** = A, **Z** = B, **S** = X, **A** = Y, **Q**/**W** = L/R, **Enter** = Start, **Backspace** = Select, Pfeiltasten, **Tab** = vorspulen. Touchscreen = Maus.

## Was geht im Emulator und was nicht

| | melonDS | echter DS Lite |
|---|---|---|
| ROM-Browser, Einstellungen, About | ✅ | ✅ |
| UI-Entwicklung (neue Screens, Fonts, Icons) | ✅ | ✅ |
| **GBA-Spiele starten** | ❌ | ✅ |
| In-Game-Menü, Helligkeit, Saves, rom3M | ❌ | ✅ |

GBARunner2 und damit DsRunner nutzen Hardware-Tricks (MPU-Aborts u.a.), die melonDS nicht emuliert, siehe [melonDS #2132](https://github.com/melonDS-emu/melonDS/issues/2132). Nach dem Start eines Spiels bleiben beide Bildschirme schwarz. **Alles, was im Spiel passiert, muss auf echter Hardware getestet werden.**

melonDS patcht nur den DLDI-Treiber des ARM9-Teils, deshalb nutzt `emu-run.ps1` den **arm9dldi**-Build.

## Test-ROM

`testrom\` enthält eine kleine eigene GBA-ROM (libgba), mit der man ohne kommerzielle Spiele prüfen kann:

- Bootet die ROM, funktioniert die Textausgabe?
- Laufen VBlank-IRQ und Timer, stimmen die FPS (≈ 59–60)?
- Kommen alle Tasten an?
- **Speichern:** Der Bootzähler im SRAM muss nach einem Neustart hochzählen. Der Save-Typ `SRAM_F_V100` wird ohne Patch erkannt.

Bauen: `cd testrom; sh ./build.sh` (Docker). Ergebnis: `testrom\dsrunner-test.gba`. In mGBA läuft sie zur Kontrolle direkt.

## Debugging

melonDS hat einen GDB-Stub (`[Instance0.Gdb]` in `melonDS.toml`, ARM9 Port 3333, ARM7 Port 3334). Damit kann man UI-Code mit `arm-none-eabi-gdb` und der `.elf` aus dem Build schrittweise debuggen.
