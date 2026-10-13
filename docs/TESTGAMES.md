# Testprotokoll

Vor jedem Merge nach `develop`, der Emulation, Speicher oder Saves betrifft, läuft die Testliste auf dem **echten DS Lite**.
Immer mit gültigem `bios.bin` testen.

Hardware: DS Lite · Flashcard: _(eintragen)_ · Kernel: YSMenu

## Testliste

| # | Spiel (Game-Code) | Warum in der Liste | Save-Typ |
|---|---|---|---|
| 1 | _(läuft sicher)_ | Referenz | |
| 2 | Pokémon Smaragd (BPEE / BPED) | rom3M, RTC, Flash 128K | Flash 128K |
| 3 | _(EEPROM-Spiel)_ | Save-Typ | EEPROM |
| 4 | _(SRAM-Spiel)_ | Save-Typ | SRAM |
| 5 | _(soundlastig)_ | Sound | |
| 6 | _(Mode 7 / 3D-lastig)_ | Geschwindigkeit, Cache | |
| 7 | _(läuft laut Liste nicht)_ | Kandidat für Fix | |

## Kriterien

**B**ootet · **G**rafik ok · **T**on ok · **S**peichern + Neustart + Laden ok · **P**erformance (spielbar ja/nein) · **R**TC (falls relevant)

## Ergebnisse

| Datum | Build (Commit / Variante) | # | B | G | T | S | P | R | Notizen |
|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | |
