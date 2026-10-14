# Ein Release erstellen

Anleitung für den Maintainer. Ein Release besteht aus einem Git-Tag `vX.Y.Z` und einem GitHub-Release mit drei Dateien, die die CI baut und anhängt:

| Datei | Inhalt |
|---|---|
| `DsRunner_M3DSReal.zip` | fertiges SD-Paket für die M3 DS Real: DsRunner als `_nds/TWiLightMenu/main.srldr` und `BOOT.NDS`, `_pico` mit Pico Loader (M3DS), Autostart-Dateien aus TWiLight Menu++, `LIESMICH.txt` |
| `DsRunner.zip` | `DsRunner.nds` und `_pico` für andere Flashkarten |
| `DsRunnerArt.exe` | PC-Programm DsRunner Art für Windows |

Die Dateinamen enthalten keine Versionsnummer, damit der Link im README (`releases/latest`) und die Anleitung immer stimmen.

## Wo die Version steht

Die Datei **`VERSION`** im Hauptverzeichnis ist die einzige Quelle, Format `X.Y.Z` ohne `v`.

| Wo | Wie |
|---|---|
| Launcher | unten rechts im Menü „Anzeige“ (Zahnrad), z. B. `v0.2.0`. `Makefile.arm9` schreibt die Version in `build/arm9/dsrunner_version.c` (nur bei Änderung, deshalb wird sonst nichts neu übersetzt). |
| Banner | Titelzeile „DsRunner 0.2.0“ (`GAME_TITLE` im `Makefile`) |
| `LIESMICH.txt` im SD-Paket | erste Zeile (`tools/make-sd-package.sh`) |
| DsRunner Art | `__version__` in `tools/dsrunner-art/dsrunnerart/__init__.py`: Fenstertitel, `--version`, User-Agent. Python kann `VERSION` in der `.exe` nicht lesen, deshalb steht die Nummer dort ein zweites Mal. `tools/version.sh` setzt und prüft beide. |

**Release-Build oder Entwicklungsstand:** Nur der Release-Workflow baut mit `DSRUNNER_RELEASE=1` und zeigt die reine Nummer. Jeder andere Build (`./build.sh`, CI bei jedem Push) zeigt `0.2.0-dev`, mit Commit, wenn git ihn kennt: `0.2.0-dev+36184e8`. So ist ein selbst gebauter Zwischenstand nie mit einem Release zu verwechseln. Einen Release-Build lokal: `./build.sh DSRUNNER_RELEASE=1` bzw. `DSRUNNER_RELEASE=1 ./build.sh package`.

**Welche Nummer?** `VERSION` enthält die Version des **nächsten** Releases. Vor 1.0: neue Funktionen erhöhen die mittlere Zahl (0.1.2 → 0.2.0), reine Fehlerbehebungen die letzte (0.2.0 → 0.2.1).

**Der Tag entscheidet.** Der Release-Workflow nimmt die Version aus dem Tag (`v0.2.0`, auch `V0.2.0`). Passen `VERSION`, DsRunner Art oder `CHANGELOG.md` noch nicht dazu, setzt er sie in seinem Checkout selbst (`tools/version.sh release <tag>`, wie `set`) und zeigt eine Warnung im Lauf; die Release-Notizen kommen dann aus „Unveröffentlicht“. Abgebrochen wird nur, wenn der Tag keine Version ist. Sauberer ist trotzdem, vorher `tools/version.sh set` auszuführen (Schritt 2): dann zeigt auch ein lokaler Build aus demselben Commit dieselbe Nummer.

## Schritt für Schritt

1. **Stand prüfen.** Alles für das Release ist auf `main`, die CI („Build DsRunner“) ist grün, und das Wichtigste ist auf dem DS Lite getestet (Autostart, Spiele starten, Spielstände).

2. **Version setzen und Changelog abschließen.**
   ```sh
   tools/version.sh set 0.2.0
   ```
   Das schreibt `VERSION` und `__version__` von DsRunner Art und macht aus `## Unveröffentlicht` in `CHANGELOG.md` den Abschnitt `## [0.2.0](…) - <heute>`, darüber kommt ein neuer, leerer Abschnitt „Unveröffentlicht“.

3. **Changelog lesen und glätten.** Einträge für Spieler formulieren (was ist neu, was geht jetzt besser), keine Interna. Rubriken: `### Hinzugefügt`, `### Geändert`, `### Behoben`, bei Bedarf `### Entfernt`. Der leere Abschnitt „Unveröffentlicht“ bleibt stehen. Vorschau der Release-Notizen:
   ```sh
   tools/version.sh notes
   ```

4. **Prüfen.**
   ```sh
   tools/version.sh check v0.2.0
   ./build.sh DSRUNNER_RELEASE=1   # optional: zeigt v0.2.0 im Menü „Anzeige“
   ```

5. **Committen und auf `main` bringen**, z. B. als Commit „Release v0.2.0“ per Pull Request. Erst danach das Release anlegen, denn der Tag zeigt auf den aktuellen Stand von `main`.

6. **Release auf GitHub anlegen:** [Releases → Draft a new release](https://github.com/HolziDape/DsRunner/releases/new)
   - **Choose a tag:** `v0.2.0` eintippen, „Create new tag: v0.2.0 on publish“, Target `main`
   - **Release title:** `v0.2.0`
   - **Beschreibung:** leer lassen oder „Generate release notes“ klicken. Der Workflow setzt den Changelog-Abschnitt und einen Installationshinweis darüber, die automatische Liste der Pull Requests bleibt darunter.
   - **„Set as a pre-release“ NICHT ankreuzen.** Der Download-Link im README zeigt auf `releases/latest`, und das überspringt Vorabversionen. „Set as the latest release“ bleibt an.
   - **Publish release**

   Per Kommandozeile geht dasselbe mit `gh release create v0.2.0 --target main --title v0.2.0 --generate-notes`.

7. **Workflow abwarten:** [Actions → „Build DsRunner release“](https://github.com/HolziDape/DsRunner/actions/workflows/release.yml), etwa 5 bis 10 Minuten. Die Jobs:

   | Job | Was er tut |
   |---|---|
   | Check version | `tools/version.sh release <tag>`: Version aus dem Tag, Warnungen, wenn `VERSION` oder Changelog nicht passen; nur bei einem Tag ohne Version laufen die anderen nicht |
   | Build DsRunner | `make` im BlocksDS-Container mit `DSRUNNER_RELEASE=1`, packt `DsRunner.zip` und mit `tools/make-sd-package.sh` `DsRunner_M3DSReal.zip` (lädt Pico Loader und TWiLight Menu++ mit festen Versionen und SHA-256) und hängt beide an |
   | Build DsRunner Art (Windows) | Tests, `DsRunnerArt.exe` mit PyInstaller, hängt sie an |
   | Release notes from CHANGELOG.md | setzt den Changelog-Abschnitt an den Anfang der Release-Beschreibung (läuft zuletzt, auch wenn ein Build fehlschlägt) |

8. **Ergebnis prüfen** auf der Release-Seite:
   - drei Dateien: `DsRunner.zip`, `DsRunner_M3DSReal.zip`, `DsRunnerArt.exe`
   - die Beschreibung beginnt mit dem Changelog-Abschnitt
   - `DsRunner_M3DSReal.zip` herunterladen: `LIESMICH.txt` beginnt mit „DsRunner 0.2.0 fuer M3 DS Real“ (ohne „Entwicklungsstand“)
   - auf dem DS: `_nds/TWiLightMenu/main.srldr` auf die Karte kopieren, im Menü „Anzeige“ steht `v0.2.0`
   - `DsRunnerArt.exe` zeigt „DsRunner Art 0.2.0“ im Fenstertitel

## Wenn etwas schiefgeht

- **Warnung „VERSION enthält …“ oder „keinen Abschnitt“ im Lauf:** Das Release ist trotzdem gebaut. Danach `tools/version.sh set X.Y.Z` ausführen und committen, damit `main` zum Release passt.
- **„Der Tag … ist keine Version“:** Release und Tag löschen (auf GitHub Release löschen und unter *Tags* den Tag löschen, oder `gh release delete <tag> --cleanup-tag`) und mit einem Tag `vX.Y.Z` neu anlegen.
- **Build bricht wegen eines Downloads ab** (Pico Loader, TWiLight Menu++, apt): im Workflow-Lauf „Re-run failed jobs“. Die Notizen werden dabei nicht doppelt eingefügt.
- **Prüfsumme passt nicht** in `make-sd-package.sh`: Die Datei auf GitHub hat sich geändert. Erst prüfen, warum, dann Version und SHA-256 im Skript zusammen anpassen (siehe [DEVELOPMENT.md](DEVELOPMENT.md)).
- **Datei fehlt am Release:** im Lauf nachsehen, welcher Job fehlgeschlagen ist. Die gebauten Dateien liegen außerdem als Artefakte („DsRunner“, „DsRunnerArt“) am Workflow-Lauf.

## Nach dem Release

- In `docs/PLAN.md` den Stand nachtragen, wenn eine Phase damit abgeschlossen ist.
- Neue Änderungen kommen wieder unter `## Unveröffentlicht` in `CHANGELOG.md`, am besten gleich im selben Pull Request wie die Änderung.
