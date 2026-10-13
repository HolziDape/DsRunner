DsRunner
========

> Ein Fork von [Gericom/GBARunner2](https://github.com/Gericom/GBARunner2) mit Fokus auf den **DS Lite mit Flashcard**.
> *A fork of Gericom's GBARunner2, focused on the DS Lite with a flashcart.*

GBARunner2 ist ein Hypervisor, der GBA-Spiele auf DS/DSi/3DS im DS-Modus ausführt. Die ganze Arbeit am Kern stammt von **Gericom**. Dieser Fork baut darauf auf.

## Unterschiede zum Original

- **rom3M-Build** (`DsRunner_arm7dldi_ds_rom3M.nds`): Es passen 3 MB statt 2 MB ROM in den RAM, z.B. für Pokémon Smaragd. Der SD-Cache ist dafür kleiner (400 KB statt 1424 KB).
- Reproduzierbarer Build mit einer festen devkitARM-Version (Docker) und GitHub Actions
- Fork-Name und Basis werden im About-Screen angezeigt

Geplante Arbeiten stehen in den [Issues](../../issues).

## Welche Datei?

| Datei | Für |
|---|---|
| `DsRunner_arm9dldi_ds.nds` | DS/DS Lite mit Flashcard (zuerst versuchen) |
| `DsRunner_arm7dldi_ds.nds` | DS/DS Lite mit Flashcard, falls arm9dldi nicht startet |
| `DsRunner_arm7dldi_ds_rom3M.nds` | wie oben, für ROMs bis 3 MB im RAM (weniger SD-Cache) |
| `DsRunner_arm7dldi_dsi.nds` / `_3ds.nds` | DSi / 3DS im DSi-Modus von SD (nicht aktiv getestet) |

## Benutzung

- Ein Ordner `gba` im Wurzelverzeichnis der SD wird standardmäßig geöffnet.
- Einstellungen öffnest du mit **R** im ROM-Menü. Sie liegen in `/_gba/gbarunner2.ini`.
- Spiele brauchen meist **keinen** SRAM-Patch, außer die [Kompatibilitätsliste](https://wiki.gbatemp.net/wiki/GBARunner2) sagt es. Dann [gbata](http://www.no-intro.org/gbadat/tools/gbata7a-en.zip) verwenden.
- Rahmen: `/_gba/frames/<GAMECODE>.bin` oder `default.bin`. Cover: `/_gba/covers/<A>/<B>/<GAMECODE>.bin`.

### BIOS

Normmatts Open-Source-BIOS ist eingebaut. Ein echtes GBA-BIOS verbessert aber die Kompatibilität. Es wird **nicht** mitgeliefert. Ablageort: `/bios.bin`, `/gba/bios.bin` oder `/_gba/bios.bin`.

Prüfsummen eines gültigen BIOS:
- CRC32: `81977335`
- MD5: `a860e8c0b6d573d191e4ec7db1b1e4f6`
- SHA1: `300c20df6731a33952ded8c436f7f186d25d3492`
- SHA256: `fd2547724b505f487e6dcb29ec2ecff3af35a841a77ab2e85fd87350abd36570`

## Bauen

Voraussetzung ist nur **Docker**, unter Windows in WSL2. Die Toolchain ist auf `devkitpro/devkitarm:20200730` festgelegt, weil neuere GCC-Versionen anderen Code erzeugen und das knappe VRAM-Budget sprengen können.

```sh
./build.sh                      # alle Varianten nach build/
./build.sh arm7dldi_ds          # nur eine Variante
```

Varianten: `arm9dldi_ds`, `arm7dldi_ds`, `arm7dldi_ds_rom3M`, `arm7dldi_dsi`, `arm7dldi_3ds`.
Nach jedem Build zeigt das Skript, wie viel von VRAM/ITCM/DTCM des ARM9 belegt ist. Wie das Projekt aufgebaut ist, steht in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

Den Namen des Forks ändert man in `fork.mk`.

## Branches

- `main`: aktueller Stand von DsRunner
- `develop`: Integrationszweig, wird nach Tests nach `main` übernommen
- `upstream-master`: unverändertes Original von Gericom
- `feature/*`, `fix/*`: Arbeitszweige, die per Pull Request nach `develop` gehen

## Kompatibilität

- Original-Liste DS/DS Lite: https://wiki.gbatemp.net/wiki/GBARunner2
- Ergebnisse dieses Forks: [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md)

## Credits

- **Gericom**: GBARunner2 ([Original](https://github.com/Gericom/GBARunner2), [Spenden](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RSVWHQ3XR2UX6))
- **Normmatt**: Open-Source-GBA-BIOS
- **unresolvedsymbol**: [DSL-Enhanced](https://github.com/unresolvedsymbol/GBARunner2-DSL-Enhanced), Vorbild für rom3M auf dem DS Lite
- [FatFS](http://elm-chan.org/fsw/ff/00index_e.html) von ChaN

Das Original-Repository enthält keine Lizenzdatei. Alle Rechte am ursprünglichen Code liegen bei Gericom. Dieser Fork ist ein nicht-kommerzielles Hobbyprojekt. Es werden keine ROMs und kein BIOS verbreitet.
