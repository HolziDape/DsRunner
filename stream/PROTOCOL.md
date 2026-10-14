# DsRunner Stream: Protokoll (Version 1)

Technische Beschreibung für Entwickler. Referenz-Implementierungen: Host `stream/host/dsstream/protocol.py` und `codec.py`, DS `stream/ds/src/decode.c` und `main.c`.

Alles läuft über **UDP, ein Port am Host (Standard 7770)**. Alle Zahlen sind **little-endian** (wie der DS, er liest sie ohne Umrechnen). Jedes Paket beginnt mit `u8 typ, u8 version` (version = 1); Pakete mit anderer Version werden ignoriert.

| Typ | Zeichen | Richtung | Zweck |
|---|---|---|---|
| HELLO | `H` (0x48) | DS → Broadcast | „Gibt es einen Host?“ |
| HERE | `W` (0x57) | Host → DS | Antwort auf HELLO |
| INPUT | `I` (0x49) | DS → Host | Tasten, Touch, Quittung, Statistik; 60-mal pro Sekunde |
| FRAME | `F` (0x46) | Host → DS | ein Stück eines kodierten Bildes |
| BYE | `Q` (0x51) | DS → Host | DS beendet |

## Ablauf

1. Der DS ist im WLAN und schickt alle 0,5 s ein HELLO an `255.255.255.255:7770` und an die Broadcast-Adresse seines Netzes. Steht `host=` in `/_dsrunner/stream.ini`, entfällt die Suche.
2. Der Host antwortet mit HERE an Absender-IP und -Port.
3. Der DS schickt ab jetzt bei jedem VBlank ein INPUT an den Host. Das erste INPUT einer neuen Adresse startet eine Sitzung (ein anderer DS übernimmt eine laufende).
4. Der Host schickt FRAME-Pakete an die Adresse, von der die INPUTs kommen.
5. Kommt 3 s lang kein INPUT, oder ein BYE, endet die Sitzung; der Host lässt dann alle gedrückten Tasten los.

## HELLO (DS → Host)

| Offset | Typ | Inhalt |
|---|---|---|
| 0 | u8 | `H` |
| 1 | u8 | Version |
| 2 | u16 | Zufallszahl (unbenutzt) |
| 4 | char[] | `DsRunnerStream` (ohne Null) |

## HERE (Host → DS)

| Offset | Typ | Inhalt |
|---|---|---|
| 0 | u8 | `W` |
| 1 | u8 | Version |
| 2 | u16 | UDP-Port für INPUT (normalerweise derselbe) |
| 4 | u8 | Bildmodus des Hosts (siehe unten, nur zur Anzeige) |
| 5 | char[] | Name des Hosts, bis 31 Zeichen, ohne Null |

## INPUT (DS → Host), 20 Byte

| Offset | Typ | Inhalt |
|---|---|---|
| 0 | u8 | `I` |
| 1 | u8 | Version |
| 2 | u16 | Laufende Nummer (ältere/doppelte werden verworfen) |
| 4 | u16 | Tasten, Bits wie libnds `keysHeld()`: A=0, B=1, SELECT=2, START=3, RECHTS=4, LINKS=5, OBEN=6, UNTEN=7, R=8, L=9, X=10, Y=11, TOUCH=12 |
| 6 | u8 | Touch x (0-255), gültig wenn Bit TOUCH gesetzt |
| 7 | u8 | Touch y (0-191) |
| 8 | u16 | `cur_frame`: Nummer des Bildes, das der DS gerade zeigt (0 = keins) |
| 10 | u8 | Flags: 0x01 `NEED_REF` (der DS hat ein Delta-Bild verworfen oder seit 1 s nichts bekommen), 0x02 `HELLO` (noch kein Bild) |
| 11 | u8 | 0 |
| 12 | u16 | Anzahl angezeigter Bilder (läuft über) |
| 14 | u16 | Anzahl verlorener/verworfener Bilder (läuft über) |
| 16 | u32 | Empfangene UDP-Nutzdaten in Byte (läuft über) |

Der Host macht aus Tasten-Änderungen Tastatur- und Maus-Ereignisse (`mappings/*.json`). Touch: Bildschirm-Koordinate × Seitengröße / 256 bzw. / 192; im Modus 256x384 liegt der Touchscreen auf der unteren Hälfte (y + 192, geteilt durch 384).

## FRAME (Host → DS)

Ein kodiertes Bild („Payload“, siehe unten) wird in Stücke geteilt. Jedes Paket ist höchstens 1400 Byte groß (passt in ein 802.11-Paket ohne IP-Fragmentierung).

| Offset | Typ | Inhalt |
|---|---|---|
| 0 | u8 | `F` |
| 1 | u8 | Version |
| 2 | u16 | Bildnummer (1…65535, danach wieder 1; 0 gibt es nicht) |
| 4 | u8 | Index dieses Stücks (0…count-1) |
| 5 | u8 | Anzahl Stücke (höchstens 255) |
| 6 | u16 | Stückgröße `chunk`: alle Stücke außer dem letzten haben genau so viele Daten |
| 8 | u32 | Gesamtlänge der Payload |
| 12 | … | Daten ab Payload-Offset `index × chunk` |

Der DS sammelt **ein Bild gleichzeitig**. Kommt ein Stück eines neueren Bildes, bevor das aktuelle vollständig ist, wird das aktuelle verworfen (zählt als verloren). Stücke älterer Bilder werden ignoriert (Vergleich mit Überlauf: `(neu - alt) mod 65536 < 32768`).

### Payload

16 Byte Kopf, dann der Rumpf:

| Offset | Typ | Inhalt |
|---|---|---|
| 0 | u8 | Flags: 0x01 `KEY` (Keyframe: alle Kacheln, keine Bitmaske), 0x02 `PALETTE` (Rumpf beginnt mit einer Palette), 0x04 `LZ4` (Rumpf ist ein LZ4-Block) |
| 1 | u8 | Modus: 0 = 256×192, 1 = 128×96 (der DS vergrößert 2× in Hardware), 2 = 256×384 (beide Bildschirme, obere Hälfte oben) |
| 2 | u16 | Bildnummer |
| 4 | u16 | Referenz: Nummer des Bildes, auf dem dieses Delta aufbaut (0 bei Keyframes) |
| 6 | u16 | Anzahl geänderter Kacheln (bei Keyframes alle) |
| 8 | u32 | Größe des Rumpfs **entpackt** |
| 12 | u16 | Anzahl Paletteneinträge (0…256) |
| 14 | u16 | 0 |

Rumpf (entpackt), jeder Abschnitt auf 4 Byte aufgefüllt:

1. Palette (nur mit `PALETTE`): `n × u16` RGB555 im DS-Format (Rot in Bit 0-4, Grün 5-9, Blau 10-14).
2. Bitmaske (nur ohne `KEY`): ein Bit pro Kachel, Kachel `i` = Byte `i/8`, Bit `i%8` (LSB zuerst). Kacheln zählen zeilenweise: 32×24 (256×192), 16×12 (128×96) oder 32×48 (256×384).
3. Kacheldaten: pro geänderter Kachel (aufsteigende Nummer) 64 Byte Palettenindizes, 8 Zeilen à 8 Pixel.

Ist `LZ4` gesetzt, ist der Rumpf ein einzelner [LZ4-Block](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md) ohne Größenangabe (die steht im Kopf). Der Host lässt LZ4 weg, wenn es nicht kleiner wird.

### Regeln auf dem DS

- Keyframe: immer anwenden (Palette setzen, alle Kacheln schreiben).
- Delta: nur anwenden, wenn `Referenz == cur_frame` und der Modus gleich ist. Sonst verwerfen und `NEED_REF` setzen.
- Nach dem Anwenden: `cur_frame = Bildnummer`, `NEED_REF` löschen.
- Kommt 1 s lang kein Bild: `NEED_REF` setzen.

### Regeln auf dem Host

- Normalerweise baut jedes Delta auf dem zuletzt **gesendeten** Bild auf (optimistisch, keine Wartezeit auf Quittungen).
- Meldet der DS `NEED_REF` mit `cur_frame = R`: das nächste Bild baut auf `R` auf, wenn der Host `R` noch kennt (die letzten 64 Bilder) und dieselbe Palette gilt, sonst Keyframe. Für dasselbe `R` frühestens nach 0,3 s nochmal.
- Ist der DS 0,5 s nach dem letzten Senden noch nicht beim letzten Bild (verlorenes letztes Bild auf einer stillen Seite), ebenso.
- Keyframes mit neuer Palette: beim Start, bei Szenenwechseln (≥ 60 % der Kacheln geändert) und wenn die Farben zu weit von der Palette abweichen, höchstens einmal pro Sekunde.
- Senderate: Pakete werden gleichmäßig verteilt (Token-Bucket). Alle 2 s vergleicht der Host die gesendeten Bytes (0,1 s zeitversetzt, damit Pakete unterwegs nicht als verloren zählen) mit den Bytes, die der DS meldet: über 15 % Verlust mit verlorenen Bildern → Rate auf 90 % des Angekommenen (höchstens -20 %); über 6 % Verlust, während die Rate begrenzt hat → -15 %; unter 2 % Verlust, während die Rate begrenzt hat → +12 KB/s. Leichter zufälliger Verlust bremst also nicht. Kein Bild-Puffer: kodiert wird immer das neueste Bild, wenn das vorige gesendet ist.

## Warum so

- **8 bpp mit Palette** passt direkt in eine 256-Farben-Bitmap-BG des DS: keine Farbumrechnung auf dem DS, halb so viele Daten wie 16 bpp.
- **8×8-Kacheln** erlauben billige Deltas (nur geänderte Kacheln) und werden mit ausgerichteten 32-Bit-Speicherzugriffen in den VRAM geschrieben (der VRAM verträgt keine 8-Bit-Schreibzugriffe). Eine Kachel kostet etwa 100 CPU-Takte.
- **LZ4** entpackt mit Byte-Kopien ohne Bit-Operationen, schneller als zlib/LZ77-Bitströme. Der Dekoder liegt als ARM-Code im ITCM.
- **Bild-Quittungen statt Wiederholungen:** UDP-Verluste kosten ein Bild; repariert wird mit einem Delta gegen das Bild, das der DS wirklich hat, ohne festen Keyframe-Takt.
- Gemessen in DeSmuME (grob, siehe README): Delta 2-9 ms, Keyframe etwa 11 ms pro Bild.
