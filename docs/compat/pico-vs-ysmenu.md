# Kompatibilität: Pico Loader (M3DS) vs. YSMenu

DS Lite + M3 DS Real. Hier sammeln wir Spiele, die mit DsRunner (Pico Loader) schlechter laufen als mit YSMenu, und ob „zurück zu DsRunner“ (L+R+Unten+B, 2 s) im Spiel klappt. Hintergrund: [LOADER.md](../LOADER.md), Loader-Build: [loader/README.md](../../loader/README.md).

Pico-Version: DsRunner-Build (v1.7.1 + develop `b9f923e` + Patches, steht in `_pico/picoLoader-DsRunner.txt`) · YSMenu-Version: 7.06 · DsRunner: v0.x

Status: ✅ läuft · ⚠️ läuft mit Fehlern · ❌ startet nicht / hängt · – nicht getestet

Hotkey → Launcher: ✅ DsRunner startet mit SD-Zugriff · ❌ reagiert nicht (Spiel läuft weiter) · ⚠️ hängt / schwarzer Bildschirm · 💾 Problem mit dem Spielstand. Dazu die Zeile `in-game reset:` aus `/_pico/igr-log.txt` in die Notiz.

| Spiel (No-Intro-Name) | Game-Code | Region | Pico | YSMenu | Hotkey → Launcher | Fehler bei Pico (wo genau?) | Immer? | Save-Typ | Cheats an? | Upstream-Issue | Notiz |
|---|---|---|---|---|---|---|---|---|---|---|---|
| *Beispiel: Spielname* | *ABCP* | *EUR* | *❌* | *✅* | *–* | *weißer Bildschirm nach dem Intro* | *ja* | | *nein* | | |

## So trägst du ein Spiel ein

1. **Game-Code:** steht in DsRunner Art im Protokoll (`[Game-Code ABCP]`) oder im ROM-Header ab Byte `0x0C` (4 Zeichen). Pico ordnet seine Korrekturen nach dem Game-Code zu.
2. **Gegenprobe:** dieselbe ROM-Datei mit YSMenu testen, beide Male mit einem frischen Spielstand. Ein alter Spielstand von einer anderen Karte kann das Ergebnis verfälschen.
3. **Fehler genau beschreiben:** wann (Start, Intro, Menü, Speichern, bestimmte Stelle) und was (Grafik, Ton, Hänger, zu langsam).
4. **Vor einer Meldung bei Pico** in den [Issues](https://github.com/LNH-team/pico-loader/issues) nach Titel und Game-Code suchen. Bei einer Meldung Plattform `M3DS` und Konsole `DS Lite` angeben. Fehler, die **nur** mit dem DsRunner-Build auftreten (nicht mit dem offiziellen Release, Paket mit `DSRUNNER_LOADER=upstream`), gehören nicht zu Pico, sondern hierher bzw. als Kommentar zu danyocoms Arbeit ([#21](https://github.com/LNH-team/pico-loader/issues/21), [#220](https://github.com/LNH-team/pico-loader/pull/220)).

## Testplan „Hotkey → Launcher“

Für den ersten Test des DsRunner-Loaders auf dem DS Lite + M3 DS Real. Vorher **alle `.sav` sichern**. Ergebnisse in die Tabelle oben.

### 0. Diagnose lesen: `/_pico/igr-log.txt`

Ab Patch 0015 schreibt der Loader bei jedem Spielstart `/_pico/igr-log.txt` und hängt nach einem Hotkey-Neustart einen Abschnitt an. **Nach jedem Versuch gleich am PC öffnen** (vor dem nächsten Spielstart) und bei Problemen den Inhalt mitschicken. Was die Zeilen bedeuten: [loader/README.md](../../loader/README.md#diagnose-_picoigr-logtxt). Kurz:

- **Datei fehlt** → auf der Karte liegt nicht der DsRunner-Loader. Alle `_pico/*.bin` aus dem Paket kopieren.
- `in-game reset: applied` → Hotkey eingebaut. `NOT applied: …` nennt den Grund (abgeschaltet, kein Dispatcher gefunden, kein Patchplatz).
- Abschnitt `[start after the in-game reset (PIGR)…]` vorhanden → der Hotkey hat ausgelöst. Fehlt er nach dem Halten, kam der Reset nie an.

Am Bildschirm: Nach 2 s Halten wird der Hauptbildschirm **halb weiß**. Hell, aber das Spiel läuft weiter → wartet auf den Slot-1-Lock. Gar nicht hell → nicht eingebaut. Hell und dann eingefroren/schwarz → Neustart von Pico hängt. Bitte genau notieren, was passiert.

### 1. Grundfunktion (ein Abend)

1. `DsRunner_M3DSReal.zip` (oder `./build.sh package`) auf die Karte. In `_pico/` müssen `picoLoader-DsRunner.txt` und die fünf `.bin` liegen.
2. **Normal spielen, ohne Hotkey:** 3–4 Spiele starten, die schon mit v1.7.1 liefen, speichern, neu starten, Spielstand laden. Unser Build ist ein eigener Build (auch ohne Patches nicht byte-gleich mit dem Release), deshalb zuerst prüfen, dass nichts schlechter geworden ist.
3. Nach dem ersten Spielstart am PC nachsehen: `_pico/dldi.bin` (DLDI-Cache) und `_pico/launcher.txt` müssen da sein. Was steht in `launcher.txt`? (Zeigt, ob der Autostart `argv[0]` übergibt: mit `fat:` davor sicher ja.)
4. **Hotkey:** im Spiel (nach dem Titelbild, im Spielgeschehen) **L+R+Unten+B gedrückt halten**. Nach etwa 2 s muss DsRunner erscheinen. Dann einen Spieleordner öffnen (SD-Zugriff klappt?) und ein **anderes** Spiel starten.
5. Hotkey nur 1 s halten und loslassen: es darf nichts passieren.
6. **Abschalten:** in `_pico/settings.json` `"inGameReset": false` eintragen, Spiel starten → Hotkey ohne Wirkung, `_pico/igr.off` existiert. Wieder `true` → Datei ist nach dem nächsten Spielstart weg, Hotkey geht wieder.

### 2. Spielstände

7. Spiel mit langem Speichervorgang (z. B. Pokémon Diamant/Perl/Platin, HeartGold/SoulSilver): „Speichern“ wählen und **sofort** den Hotkey halten. Erwartung: der Reset kommt erst nach dem Speichern. Danach den Spielstand mit DsRunner **und** mit YSMenu laden.
8. Ein Spiel mit EEPROM- und eins mit Flash-Speicher (Save-Typ in die Tabelle).

### 3. Spiele quer durch die SDK-Versionen (3–5 Abende)

Mindestens ein Spiel pro Zeile, wenn vorhanden. Pro Spiel: startet es, reagiert der Hotkey, startet danach DsRunner mit SD-Zugriff.

| Gruppe | Beispiele (Game-Code EUR/DE, ohne Gewähr) | Warum |
|---|---|---|
| SDK 2 (2004/05) | Super Mario 64 DS (`ASMP`), Nintendogs, Mario Kart DS (`AMCP`) | alter Dispatcher; MKDS hat eigene Reset-Logik |
| SDK 3 | New Super Mario Bros. (`A2DP`), Animal Crossing: Wild World (`ADMP`), Metroid Prime Hunters | sehr verbreitet |
| SDK 4 | Pokémon Diamant/Perl (`ADAD`/`APAD`), Zelda: Phantom Hourglass (`AZEP`) | große Saves |
| SDK 5 / DSi-Hybrid | Pokémon HeartGold/SoulSilver (`IPKD`/`IPGD`), Pokémon Schwarz/Weiß (`IRBD`/`IRAD`), Zelda: Spirit Tracks (`BKIP`) | Hybrid-ROMs, viele DMA-Patches, wenig Patchplatz |
| eigener IRQ-Dispatcher | Golden Sun: Die dunkle Dämmerung (`BO5P`), Diddy Kong Racing DS (`YDKP`), Black Sigil | eigene Hooks von danyocom, sonst ohne Hotkey |
| ohne `OS_ResetSystem` | Metal Slug 7, Die Simpsons | frühere Ansätze scheiterten hier |
| WLAN / Download-Spiel | Mario Kart DS im WLAN-Menü, Download-Spiel-Menü | Hotkey während WLAN aktiv ist |

### 4. Sonderfälle

9. **Cheats an:** dasselbe Spiel einmal ohne und einmal mit aktivem Cheat (Y im Menü) starten. Läuft das Spiel, geht der Hotkey?
10. **Eingebauter Soft-Reset** des Spiels (meist L+R+Start+Select): startet weiter **dasselbe** Spiel neu, nicht DsRunner.
11. **Homebrew** (z. B. ein Emulator mit „Beenden“): kehrt wie bisher zu DsRunner zurück.
12. Mehrmals hintereinander: Spiel → Hotkey → anderes Spiel → Hotkey (5×). Läuft alles weiter?

Reagiert ein Spiel nicht auf den Hotkey, ist das kein Fehler im Spiel: Pico baut den Hotkey nur ein, wenn er den Einsprungpunkt erkennt und Platz hat. Dann ❌ eintragen, Game-Code notieren und an danyocom bzw. in [#21](https://github.com/LNH-team/pico-loader/issues/21) melden.
