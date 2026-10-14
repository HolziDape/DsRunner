# DsRunner Stream (experimentell)

Browser-Spiele auf dem Nintendo DS spielen: Ein **Raspberry Pi 5** (oder ein PC) öffnet das Spiel in Chromium, verkleinert das Bild auf DS-Größe und schickt es per WLAN an den DS. Die Tasten und der Touchscreen des DS gehen zurück an den Pi und landen dort als Tastatur und Maus im Spiel.

Der DS selbst könnte solche Spiele nie ausführen (67 MHz, 4 MB RAM, WLAN nur 802.11b mit WEP/offen). Er zeigt nur Bilder an, wie ein kleiner Fernseher mit Controller.

> **Experimentell.** Getestet ist bisher nur die Software am PC (Host, Testseite, ein simulierter DS, der DS-Teil im Emulator DeSmuME). Auf echtem DS Lite + Raspberry Pi 5 ist es noch **nicht** getestet. Rückmeldungen willkommen.

Was du erwarten kannst:

- Bild: 256×192 Pixel mit 256 Farben, je nach Spiel etwa 10–20 Bilder pro Sekunde. Bei viel Bewegung (Scrollen, 3D) weniger; dann hilft der Modus `128x96`.
- Verzögerung: etwa 100–200 ms. Für Rätsel-, Klick-, Karten- und Strategiespiele gut, für schnelle Reaktionsspiele eher nicht.
- **Kein Ton auf dem DS.** Der Ton kommt am Pi raus (HDMI oder Klinke), falls du dort Lautsprecher anschließt.
- Es funktioniert mit jeder Webseite, die in Chromium läuft. Spiele, die nur mit Maus und Tastatur gehen, brauchen eine passende [Tastenbelegung](#tastenbelegung).

## Wie es aufgebaut ist

```
Internet ── Router ══ Ethernet ══ Raspberry Pi 5 ─ ─ WLAN "DsRunner-Stream" (offen) ─ ─ DS Lite
                                  (Chromium + dsstream)        nur für den DS        (DsRunnerStream.nds)
```

- Der Pi hängt per **Kabel (eth0)** am Router und hat darüber Internet für das Browser-Spiel.
- Das **eingebaute WLAN des Pi (wlan0)** wird ein eigener, offener Hotspot nur für den DS. Der DS Lite kann kein WPA, deshalb muss es ein offenes Netz sein (Sicherheit: siehe [unten](#sicherheit)).
- Der DS braucht kein Internet, nur den Pi.

## Was du brauchst

- Raspberry Pi 5 mit **Raspberry Pi OS (Bookworm, 64 Bit, mit Desktop)**, per Ethernet am Router. Ein Pi 4 geht auch, ist aber langsamer.
- Den DS mit DsRunner (M3 DS Real) und die Datei `DsRunnerStream.nds`.
- Am Pi: Bildschirm und Tastatur für die Einrichtung, oder SSH.

## 1. Pi einrichten

### WLAN-Land setzen

Ohne Land ist das WLAN des Pi gesperrt: `sudo raspi-config` → *Localisation Options* → *WLAN Country* → `DE` (bzw. dein Land).

### Programme installieren

```sh
sudo apt update
sudo apt install -y git python3-venv chromium
# (auf älteren Bookworm-Ständen heißt das Paket chromium-browser)

cd ~
git clone https://github.com/HolziDape/DsRunner.git
python3 -m venv ~/dsstream-venv
~/dsstream-venv/bin/pip install -r ~/DsRunner/stream/host/requirements.txt
```

Der Host nimmt auf dem Pi automatisch das Chromium von Raspberry Pi OS (`/usr/bin/chromium` bzw. `/usr/bin/chromium-browser`), weil das die GPU des Pi nutzt. `playwright install` ist deshalb **nicht** nötig. Auf einem PC dagegen einmal `~/dsstream-venv/bin/python -m playwright install chromium` ausführen.

### Erster Test ohne DS

```sh
cd ~/DsRunner/stream/host
~/dsstream-venv/bin/python -m dsstream --url test --headful --gpu
```

Ein Chromium-Fenster mit der Testseite geht auf (Farbbalken, ein gelbes Quadrat, Bilder-Zähler) und im Terminal steht `Warte auf den DS an UDP-Port 7770`. In einem zweiten Terminal simuliert ein „Schein-DS“ den echten:

```sh
cd ~/DsRunner/stream/host
~/dsstream-venv/bin/python -m dsstream.fakeclient --seconds 10 --save /tmp/ds --script "RIGHT@2-4,A@5"
```

Er zeigt am Ende Bilder pro Sekunde und KB/s an und legt Bilder in `/tmp/ds` ab. Das Quadrat sollte darauf nach rechts gewandert sein und die Farbe gewechselt haben.

## 2. Hotspot für den DS (wlan0)

Das Skript `stream/host/tools/hotspot.sh` erledigt alles mit NetworkManager (Standard auf Bookworm):

```sh
cd ~/DsRunner/stream/host
sudo tools/hotspot.sh setup                 # einmal: offenes WLAN "DsRunner-Stream", Kanal 6
sudo tools/hotspot.sh on                    # einschalten
sudo tools/hotspot.sh status                # Zustand und verbundene Geräte
sudo tools/hotspot.sh autostart on          # optional: bei jedem Start einschalten
```

Anderer Name oder Kanal: `sudo tools/hotspot.sh setup MeinDSNetz 11` (nimm 1, 6 oder 11, je nachdem, welcher Kanal in deiner Umgebung am wenigsten belegt ist).

Was das Skript macht, falls du es lieber von Hand eingibst:

```sh
sudo nmcli connection add type wifi ifname wlan0 con-name DsRunner-Stream autoconnect no \
  ssid DsRunner-Stream 802-11-wireless.mode ap 802-11-wireless.band bg 802-11-wireless.channel 6 \
  802-11-wireless.powersave 2 ipv4.method shared ipv4.addresses 192.168.4.1/24 ipv6.method disabled
sudo nmcli connection up DsRunner-Stream
```

- Ohne `wifi-sec`-Einstellungen ist das Netz offen. `band bg` heißt 2,4 GHz mit den alten 802.11b-Raten, die der DS Lite braucht.
- `ipv4.method shared` lässt NetworkManager die IP-Adressen verteilen (DHCP). Der Pi hat im Hotspot die Adresse **192.168.4.1**.
- Internet für den Pi: Der Browser auf dem Pi geht direkt über eth0 ins Internet, dafür ist nichts weiter nötig. `shared` würde zusätzlich auch den DS ins Internet weiterleiten (NAT). Das braucht er nicht; `hotspot.sh lock` schaltet es ab (empfohlen, siehe [Sicherheit](#sicherheit)).
- Ethernet bleibt dabei ganz normal in Betrieb.

## 3. DS einrichten

1. `DsRunnerStream.nds` nach `Games/Apps/` auf die SD-Karte kopieren (im DsRunner-Paket ist sie dort schon drin, falls sie mitgebaut wurde).
2. Die Datei `/_dsrunner/stream.ini` anlegen (Text, z. B. mit dem Windows-Editor):

```ini
; DsRunner Stream
ssid=DsRunner-Stream   ; Name des Hotspots (Standard)
host=                  ; leer: Pi automatisch suchen. Klappt das nicht: host=192.168.4.1
port=7770
wifi=auto              ; auto, wfc (nur WFC-Einstellungen) oder list (immer Liste zeigen)
key=                   ; nur bei WEP-Netzen: Schlüssel (5/13 Zeichen oder 10/26 Hex-Ziffern)
screen=bottom          ; Bild unten (Touch trifft das Bild) oder top
```

**Ohne `stream.ini` geht es auch:** Der DS verbindet sich dann mit dem Netz „DsRunner-Stream“ (so heißt der Hotspot aus `hotspot.sh`), sonst mit einem Netz aus seinen WFC-Einstellungen (die man einmal in einem Nintendo-WFC-Spiel einrichtet), sonst zeigt er eine Liste der Netze in der Nähe zur Auswahl (Steuerkreuz, A). Die Datei brauchst du also nur für einen anderen Netznamen, WEP oder eine feste Host-Adresse.

## 4. Spielen

1. Am Pi den Host starten, mit der Adresse des Spiels:

   ```sh
   cd ~/DsRunner/stream/host
   ~/dsstream-venv/bin/python -m dsstream --url "ADRESSE-DES-SPIELS" --headful --gpu
   ```

   Für `ADRESSE-DES-SPIELS` die Adresse einsetzen, die du sonst im Browser eintippst (mit `https://`).

2. Am DS in DsRunner den Ordner **Apps** öffnen und **DsRunner Stream** starten. Nach 2 Sekunden verbindet er sich (oder sofort mit A), sucht den Pi und zeigt das Bild.
3. Bedienung am DS:
   - Tasten und Touchscreen gehen ans Spiel (Belegung siehe unten).
   - **SELECT + START eine Sekunde halten**: Pause-Menü (weiter, Pi neu suchen, beenden). Beenden führt zurück zu DsRunner.
   - Auf dem Bildschirm ohne Bild stehen Bilder pro Sekunde, Empfang und Verluste.
4. Manche Spiele wollen erst einen Klick (z. B. „Start“ oder wegen des Tons): einmal auf den Touchscreen tippen.

### Optionen des Hosts

| Option | Bedeutung |
|---|---|
| `--url ADRESSE` | Seite, die gestreamt wird. `test` = eingebaute Testseite, `chrome://gpu` zeigt, ob die GPU genutzt wird |
| `--mode 256x192` | ein Bildschirm (Standard) |
| `--mode 128x96` | halbe Auflösung, der DS vergrößert in Hardware. Schneller und flüssiger, aber grob |
| `--mode 256x384` | beide Bildschirme (Seite im Hochformat), Touch auf der unteren Hälfte |
| `--mapping datei.json` | Tastenbelegung (Dateien in `stream/host/mappings/`) |
| `--viewport 640x480` | Fenstergröße der Seite im Browser (Standard 512×384, bei 256x384: 512×768). Größer = mehr vom Spiel, aber kleinere Schrift auf dem DS |
| `--fps 15` | höchstens so viele Bilder pro Sekunde (Standard 20) |
| `--headful` | Chromium mit Fenster. Auf dem Pi empfohlen, dort rechnet die GPU |
| `--gpu` | GPU-Optionen für Chromium (WebGL) |
| `--palette fixed` | feste Farben statt passend zum Bild: weniger Rechenarbeit, sieht schlechter aus |
| `--rate 120 --max-rate 200` | Start- und Höchstrate in KB/s. Der Host passt die Rate selbst an Verluste an |
| `--mute` | Ton am Pi aus |
| `--port 7770` | UDP-Port (am DS dann `port=` gleich setzen) |

Alle Optionen: `python -m dsstream --help`.

## Tastenbelegung

Standard (`mappings/default.json`):

| DS | Spiel |
|---|---|
| Steuerkreuz | Pfeiltasten |
| A | Leertaste |
| B | Shift |
| X / Y | X / Z |
| L / R | Q / E |
| START | Enter |
| SELECT | Escape |
| Touchscreen | Mausklick an der Stelle (ziehen = Maus gedrückt bewegen) |

Weitere: `wasd.json` (Steuerkreuz = W/A/S/D) und `maus.json` (Touchscreen bewegt nur den Mauszeiger, A = Linksklick, B = Rechtsklick).

Eigene Belegung: eine der Dateien kopieren und ändern, dann `--mapping meine.json`.

```json
{
  "name": "Mein Spiel",
  "keys": {
    "UP": "KeyW", "DOWN": "KeyS", "LEFT": "KeyA", "RIGHT": "KeyD",
    "A": ["Space", "Enter"],
    "B": "MouseRight",
    "START": "KeyP"
  },
  "touch": "click"
}
```

- DS-Tasten: `A B X Y L R START SELECT UP DOWN LEFT RIGHT`.
- Ziel: Tastennamen wie im Browser, z. B. `ArrowUp`, `Space`, `Enter`, `Shift`, `Control`, `KeyA` … `KeyZ`, `Digit1`, `F5`; mehrere als Liste. `MouseLeft`, `MouseRight`, `MouseMiddle` klicken an der zuletzt berührten Stelle.
- `touch`: `click` (Tippen = Klicken, Ziehen = gedrückt bewegen), `move` (nur Mauszeiger bewegen) oder `off`.

## Leistung auf dem Raspberry Pi

- Der Host selbst ist sparsam: Bild verkleinern, Farben wählen und packen dauert am PC 1–2 ms pro Bild, auf dem Pi 5 grob geschätzt 3–8 ms.
- Die meiste Arbeit hat **Chromium mit dem Spiel**. 2D-Spiele laufen auf dem Pi 5 meist gut. **3D/WebGL-Spiele können ruckeln.** Mit `--headful --gpu` rechnet die GPU des Pi 5 (VideoCore VII). Ob das klappt, zeigt `--url chrome://gpu` (direkt auf dem DS oder im Fenster): bei „WebGL“ sollte „Hardware accelerated“ stehen.
- Ohne Fenster (Standard ohne `--headful`, z. B. als Dienst ohne Desktop) rechnet Chromium meist ohne GPU (Software). Für 2D reicht das oft, für 3D kaum.
- Der Engpass ist meist das WLAN des DS: 802.11b mit 1–2 Mbit/s, real etwa 100–180 KB/s. Ruhige Spiele brauchen 30–60 KB/s, viel Bewegung 100 KB/s und mehr. Dann `--mode 128x96` oder `--fps 15` probieren.
- Gemessen am PC mit der Testseite und dem Schein-DS: 256×192 normal 20 fps bei ~50 KB/s, viel Bewegung 20 fps bei ~100 KB/s, 128×96 viel Bewegung 20 fps bei ~40 KB/s, beide Bildschirme 20 fps bei ~70 KB/s.

## Automatisch starten

Die Einstellungen stehen in einer Datei, die das Start-Skript `stream/host/start.sh` liest:

```sh
cp ~/DsRunner/stream/host/systemd/dsstream.env ~/.config/dsstream.env
nano ~/.config/dsstream.env       # DSSTREAM_URL usw. eintragen
```

**Variante A, mit Fenster und GPU (empfohlen für WebGL):** Start nach der Anmeldung am Desktop. Der Pi sollte sich dafür automatisch anmelden (`sudo raspi-config` → *System Options* → *Boot / Auto Login* → *Desktop Autologin*).

```sh
mkdir -p ~/.config/autostart
cp ~/DsRunner/stream/host/systemd/dsstream.desktop ~/.config/autostart/
```

**Variante B, systemd-Dienst ohne Fenster** (auch ohne Desktop, z. B. Raspberry Pi OS Lite; ohne GPU):

```sh
mkdir -p ~/.config/systemd/user
cp ~/DsRunner/stream/host/systemd/dsstream.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now dsstream
sudo loginctl enable-linger $USER    # startet beim Booten, auch ohne Anmeldung
journalctl --user -u dsstream -f     # Ausgaben ansehen
```

Beide erwarten das Repository in `~/DsRunner` und das venv in `~/dsstream-venv`; sonst die Pfade in den Dateien anpassen. Den Hotspot dazu mit `sudo tools/hotspot.sh autostart on` beim Booten einschalten.

## Sicherheit

Ein offenes WLAN kann jeder in Reichweite benutzen. Deshalb:

- **Eigenes Netz** nur für den DS (nicht dein normales WLAN öffnen), mit eigenem Namen.
- **Ausschalten, wenn du nicht spielst:** `sudo tools/hotspot.sh off`.
- **Abschotten:** `sudo tools/hotspot.sh lock` erlaubt im Hotspot nur noch DHCP, DNS und den Stream-Port, nichts wird ins Internet oder dein Heimnetz weitergeleitet. Mit MAC-Adresse nur noch dein DS: `sudo tools/hotspot.sh lock 00:09:BF:12:34:56` (DsRunner Stream zeigt die MAC des DS nach dem Verbinden an). Eine MAC lässt sich fälschen, es ist nur eine zusätzliche Hürde. Die Regeln gelten bis zum Neustart des Pi.
- Wer im Hotspot ist, kann die Steuerung des Browsers am Pi übernehmen (Tasten und Klicks). Also keine Seiten streamen, in denen du mit wichtigen Konten angemeldet bist.
- Den Pi aktuell halten (`sudo apt update && sudo apt full-upgrade`) und für SSH ein gutes Passwort oder Schlüssel nutzen.

WEP statt offen ist auf dem Pi meist nicht möglich (der WLAN-Treiber des Pi unterstützt WEP im Hotspot-Betrieb in der Regel nicht) und wäre ohnehin leicht zu knacken.

## Fehlerbehebung

| Problem | Lösung |
|---|---|
| DS findet das Netz nicht | Hotspot an? (`hotspot.sh status`) WLAN-Land gesetzt? Anderen Kanal versuchen (1, 6, 11). Manche Hotspots lassen keine 802.11b-Geräte zu; dann hilft ggf. ein USB-WLAN-Stick mit hostapd (`hw_mode=g`, ohne `ieee80211n`). |
| DS verbindet, „Noch kein Host“ | Läuft `dsstream`? Firewall am Pi (falls `ufw` installiert: `sudo ufw allow in on wlan0 to any port 7770 proto udp`). Sonst in `stream.ini` `host=192.168.4.1` eintragen. |
| „Keine Bilder vom Host“ | Seite lädt noch oder hängt; Ausgabe im Terminal ansehen. Mit `--url test` prüfen. |
| Bild ruckelt stark | `--mode 128x96`, `--fps 15`, DS näher an den Pi, anderer Kanal. Bei 3D-Spielen: `--headful --gpu`. |
| Tasten wirken nicht | Erst einmal ins Spiel tippen (Fokus). Passt die Belegung? Manche Spiele wollen WASD (`--mapping wasd.json`). |
| Farben sehen streifig aus | 256 Farben sind die Grenze des DS-Bildes. `--palette adaptive` (Standard) wählt sie passend zum Bild. |

## Für Entwickler

- Protokoll und Bildformat: [PROTOCOL.md](PROTOCOL.md).
- DS-Programm bauen (Docker, gleiches BlocksDS-Image wie DsRunner, bringt DSWiFi mit): `stream/ds/build.sh` → `stream/ds/DsRunnerStream.nds`.
- Tests des Hosts (ohne Browser): `cd stream/host && python -m pytest tests` (oder `python -m unittest discover -s tests -t tests`). Sie bauen auch den Dekoder des DS für den PC (`stream/ds/hosttest`) und vergleichen ihn mit dem Python-Dekoder.
- Testaufnahme für Messungen: `python -m dsstream.record --url test --out packets.bin`; daraus sind die Daten für den Geschwindigkeitstest des DS (`stream/ds/data`, Taste Y im Startbildschirm).
- Windows als Host geht auch (`py -m dsstream ...`, vorher `py -m playwright install chromium`); Windows-Firewall für UDP 7770 freigeben. Der Hotspot muss dann ein anderes offenes Gerät sein.
