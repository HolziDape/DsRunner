# Kompatibilität: Pico Loader (M3DS) vs. YSMenu

DS Lite + M3 DS Real. Hier sammeln wir Spiele, die mit DsRunner (Pico Loader) schlechter laufen als mit YSMenu. Hintergrund: [LOADER.md](../LOADER.md).

Pico-Version: v1.7.1 (M3DS) · YSMenu-Version: 7.06 · DsRunner: v0.x

Status: ✅ läuft · ⚠️ läuft mit Fehlern · ❌ startet nicht / hängt · – nicht getestet

| Spiel (No-Intro-Name) | Game-Code | Region | Pico | YSMenu | Fehler bei Pico (wo genau?) | Immer? | Save-Typ | Cheats an? | Upstream-Issue | Notiz |
|---|---|---|---|---|---|---|---|---|---|---|
| *Beispiel: Spielname* | *ABCP* | *EUR* | *❌* | *✅* | *weißer Bildschirm nach dem Intro* | *ja* | | *nein* | | |

## So trägst du ein Spiel ein

1. **Game-Code:** steht in DsRunner Art im Protokoll (`[Game-Code ABCP]`) oder im ROM-Header ab Byte `0x0C` (4 Zeichen). Pico ordnet seine Korrekturen nach dem Game-Code zu.
2. **Gegenprobe:** dieselbe ROM-Datei mit YSMenu testen, beide Male mit einem frischen Spielstand. Ein alter Spielstand von einer anderen Karte kann das Ergebnis verfälschen.
3. **Fehler genau beschreiben:** wann (Start, Intro, Menü, Speichern, bestimmte Stelle) und was (Grafik, Ton, Hänger, zu langsam).
4. **Vor einer Meldung bei Pico** in den [Issues](https://github.com/LNH-team/pico-loader/issues) nach Titel und Game-Code suchen. Bei einer Meldung Plattform `M3DS` und Konsole `DS Lite` angeben.

Sobald es „zurück zum Launcher“ gibt, kommt eine Spalte **Hotkey → Launcher** dazu (✅ / ❌ reagiert nicht / ⚠️ hängt / 💾 Problem mit dem Spielstand).
