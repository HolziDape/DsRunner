#!/bin/bash
# DsRunner Stream: richtet den Host auf einem Raspberry Pi ein (Raspberry Pi OS
# Bookworm, 64 Bit, mit Desktop). Ein Befehl, mehrmals ausfuehrbar: was schon da
# ist, bleibt, wie es ist. Fragt vor allem, was etwas am System aendert.
#
#   bash tools/install-pi.sh [Optionen]      (im Ordner stream/host bzw. DsRunnerStream-Host)
#
#   --url ADRESSE     Adresse des Spiels fuer ~/.config/dsstream.env (sonst Frage, Standard "test")
#   --country XX      WLAN-Land, falls noch keins gesetzt ist (z. B. DE, AT, CH)
#   --yes, -y         alle Fragen mit der Vorgabe in [Klammern] beantworten
#   --no-hotspot      keinen Hotspot einrichten (z. B. bei eigenem Router fuer den DS)
#   --help, -h        diese Hilfe
#
# Was es macht:
#   1. apt: python3-venv und chromium installieren (falls sie fehlen)
#   2. Python-Umgebung ~/dsstream-venv mit den Paketen aus requirements.txt
#   3. WLAN-Land setzen, falls keins gesetzt ist (ohne Land ist das WLAN gesperrt)
#   4. Hotspot "DsRunner-Stream" fuer den DS anlegen (tools/hotspot.sh setup), optional
#      beim Booten einschalten
#   5. ~/.config/dsstream.env (Adresse des Spiels) und das Start-Skript ~/dsstream.sh
#   6. optional: Autostart nach der Anmeldung am Desktop (~/.config/autostart)
#
# Umgebung: DSSTREAM_VENV (Standard ~/dsstream-venv)
set -euo pipefail

HOST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV="${DSSTREAM_VENV:-$HOME/dsstream-venv}"
CONF="$HOME/.config/dsstream.env"
STARTER="$HOME/dsstream.sh"
AUTOSTART="$HOME/.config/autostart/dsstream.desktop"
CON="DsRunner-Stream"
MARKER="angelegt von install-pi.sh"
# the guide: stream/README.md in the repository, README.md in DsRunnerStream-Host.zip
README="$HOST_DIR/README.md"
[ -f "$README" ] || README="$(cd "$HOST_DIR/.." && pwd)/README.md"

URL=""
COUNTRY=""
YES=0
HOTSPOT=1

usage() {
	sed -n '2,23p' "$0" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
	case "$1" in
	--url) URL="${2:?--url braucht eine Adresse}"; shift 2 ;;
	--url=*) URL="${1#--url=}"; shift ;;
	--country) COUNTRY="${2:?--country braucht ein Land, z. B. DE}"; shift 2 ;;
	--country=*) COUNTRY="${1#--country=}"; shift ;;
	-y|--yes) YES=1; shift ;;
	--no-hotspot) HOTSPOT=0; shift ;;
	-h|--help) usage; exit 0 ;;
	*) echo "Unbekannte Option: $1"; echo; usage; exit 1 ;;
	esac
done

# ---------------------------------------------------------------- Ausgabe/Fragen

step() { printf '\n\033[1m== %s\033[0m\n' "$*"; }
ok() { printf '   \033[32mOK\033[0m  %s\n' "$*"; }
info() { printf '   %s\n' "$*"; }
warn() { printf '   \033[33mACHTUNG\033[0m  %s\n' "$*"; }
fail() { printf '\n\033[31mFEHLER\033[0m  %s\n' "$*"; exit 1; }

# interactive only when a terminal is attached (curl ... | bash has none)
if [ -t 0 ]; then INTERACTIVE=1; else INTERACTIVE=0; fi

# ask "Frage" j|n   -> 0 for yes; the second argument is the default
ask() {
	local q="$1" def="$2" hint answer
	if [ "$def" = "j" ]; then hint="J/n"; else hint="j/N"; fi
	if [ "$YES" = "1" ] || [ "$INTERACTIVE" = "0" ]; then
		printf '   %s [%s] %s\n' "$q" "$hint" "$def"
		[ "$def" = "j" ]
		return
	fi
	while true; do
		read -r -p "   $q [$hint] " answer || answer=""
		answer="${answer:-$def}"
		case "$answer" in
		[jJyY]*) return 0 ;;
		[nN]*) return 1 ;;
		esac
	done
}

# ask_value "Frage" default  -> answer on stdout
ask_value() {
	local q="$1" def="$2" answer
	if [ "$YES" = "1" ] || [ "$INTERACTIVE" = "0" ]; then
		printf '%s' "$def"
		return
	fi
	read -r -p "   $q [$def] " answer || answer=""
	printf '%s' "${answer:-$def}"
}

# value in single quotes for a shell file (start.sh sources dsstream.env)
shell_quote() {
	printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

# ---------------------------------------------------------------- Vorpruefung

step "DsRunner Stream einrichten"
info "Host-Ordner: $HOST_DIR"

[ -f "$HOST_DIR/dsstream/__main__.py" ] && [ -f "$HOST_DIR/requirements.txt" ] ||
	fail "Hier liegt kein DsRunner-Stream-Host. Das Skript muss in stream/host/tools liegen."
[ "$(id -u)" != "0" ] ||
	fail "Bitte als normaler Benutzer starten, nicht mit sudo. Das Skript fragt selbst nach sudo, wo es noetig ist."
command -v sudo >/dev/null || fail "sudo fehlt."

if [ -r /etc/os-release ]; then
	# shellcheck disable=SC1091
	. /etc/os-release
fi
if [ "${VERSION_CODENAME:-}" != "bookworm" ]; then
	warn "Gedacht fuer Raspberry Pi OS Bookworm, hier laeuft: ${PRETTY_NAME:-unbekannt}."
	ask "Trotzdem weitermachen?" n || exit 1
fi
if [ "$(uname -m)" != "aarch64" ]; then
	warn "Kein 64-Bit-System ($(uname -m)). Chromium und Playwright brauchen Raspberry Pi OS 64 Bit."
	ask "Trotzdem weitermachen?" n || exit 1
fi

# ---------------------------------------------------------------- 1. Pakete

step "1/6 Programme (apt)"
missing=()
dpkg -s python3-venv >/dev/null 2>&1 || missing+=(python3-venv)
if command -v chromium >/dev/null || command -v chromium-browser >/dev/null; then
	:
elif apt-cache show chromium >/dev/null 2>&1; then
	missing+=(chromium)
else
	# older Bookworm images call the package chromium-browser
	missing+=(chromium-browser)
fi
if [ ${#missing[@]} -eq 0 ]; then
	ok "python3-venv und Chromium sind schon da."
else
	info "Fehlt: ${missing[*]}"
	if ask "Mit apt installieren (sudo apt install ${missing[*]})?" j; then
		sudo apt-get update
		sudo apt-get install -y "${missing[@]}"
		ok "Installiert: ${missing[*]}"
	else
		fail "Ohne ${missing[*]} geht es nicht weiter."
	fi
fi

# ---------------------------------------------------------------- 2. venv

step "2/6 Python-Umgebung ($VENV)"
if [ -x "$VENV/bin/python" ]; then
	ok "Vorhanden, wird nur aktualisiert."
else
	python3 -m venv "$VENV"
	ok "Angelegt."
fi
info "Pakete installieren (playwright, numpy, pillow, lz4), das dauert beim ersten Mal ein paar Minuten ..."
"$VENV/bin/python" -m pip install --disable-pip-version-check -q --upgrade pip
"$VENV/bin/python" -m pip install --disable-pip-version-check -q -r "$HOST_DIR/requirements.txt"
(cd "$HOST_DIR" && "$VENV/bin/python" -m dsstream --help >/dev/null) ||
	fail "dsstream startet nicht mit $VENV/bin/python."
ok "Pakete installiert. Chromium kommt vom System (nutzt die GPU des Pi), playwright install ist nicht noetig."

# ---------------------------------------------------------------- 3. WLAN-Land

step "3/6 WLAN-Land"
current=""
if command -v raspi-config >/dev/null; then
	current="$(sudo raspi-config nonint get_wifi_country 2>/dev/null || true)"
fi
if [ -z "$current" ] && command -v iw >/dev/null; then
	current="$(iw reg get 2>/dev/null | sed -n 's/^country \([A-Z][A-Z]\).*/\1/p' | head -n1)"
	[ "$current" = "00" ] && current=""
fi
if [ -n "$current" ]; then
	ok "Gesetzt: $current"
else
	info "Es ist noch kein WLAN-Land gesetzt. Ohne Land ist das WLAN des Pi gesperrt,"
	info "der Hotspot fuer den DS funktioniert dann nicht. Das Land legt fest, welche"
	info "Kanaele und Sendeleistung erlaubt sind (Deutschland: DE, Oesterreich: AT, Schweiz: CH)."
	if [ -z "$COUNTRY" ]; then
		COUNTRY="$(ask_value "Land (zwei Buchstaben):" DE)"
	fi
	COUNTRY="$(printf '%s' "$COUNTRY" | tr '[:lower:]' '[:upper:]')"
	if ! printf '%s' "$COUNTRY" | grep -qx '[A-Z][A-Z]'; then
		warn "\"$COUNTRY\" ist kein Laendercode. Spaeter setzen: sudo raspi-config -> Localisation Options -> WLAN Country"
	elif ! command -v raspi-config >/dev/null; then
		warn "raspi-config fehlt. Von Hand setzen, z. B.: sudo iw reg set $COUNTRY (gilt bis zum Neustart)"
	elif ask "WLAN-Land auf $COUNTRY setzen (sudo raspi-config nonint do_wifi_country $COUNTRY)?" j; then
		sudo raspi-config nonint do_wifi_country "$COUNTRY"
		ok "WLAN-Land: $COUNTRY"
	else
		warn "Nicht gesetzt. Spaeter: sudo raspi-config -> Localisation Options -> WLAN Country"
	fi
fi

# ---------------------------------------------------------------- 4. Hotspot

step "4/6 Hotspot fuer den DS (wlan0)"
HOTSPOT_READY=0
if [ "$HOTSPOT" = "0" ]; then
	info "Uebersprungen (--no-hotspot)."
elif ! command -v nmcli >/dev/null; then
	warn "NetworkManager (nmcli) fehlt, der Hotspot kann nicht angelegt werden."
	info "Raspberry Pi OS Bookworm bringt ihn mit. Anleitung von Hand: $README, Abschnitt Hotspot."
else
	# wlan0 used for the Pi's own network? the hotspot would take it over
	wlan_con="$(nmcli -t -f DEVICE,CONNECTION device 2>/dev/null | sed -n 's/^wlan0://p')"
	eth_up="$(nmcli -t -f TYPE,STATE device 2>/dev/null | grep -c '^ethernet:connected' || true)"
	if nmcli -t -f NAME connection show | grep -qx "$CON"; then
		ok "Hotspot \"$CON\" ist schon eingerichtet."
		HOTSPOT_READY=1
		if ask "Trotzdem neu anlegen (Name und Kanal zuruecksetzen auf DsRunner-Stream, Kanal 6)?" n; then
			sudo sh "$HOST_DIR/tools/hotspot.sh" setup
		fi
	else
		info "Der Pi bekommt ein eigenes, offenes WLAN \"DsRunner-Stream\" nur fuer den DS"
		info "(der DS Lite kann kein WPA). Der Pi selbst geht per Ethernet ins Internet."
		if ask "Hotspot anlegen (sudo tools/hotspot.sh setup)?" j; then
			sudo sh "$HOST_DIR/tools/hotspot.sh" setup
			HOTSPOT_READY=1
		fi
	fi
	if [ "$HOTSPOT_READY" = "1" ]; then
		if [ -n "$wlan_con" ] && [ "$wlan_con" != "$CON" ] && [ "$wlan_con" != "--" ]; then
			warn "wlan0 ist gerade mit \"$wlan_con\" verbunden. Sobald der Hotspot an ist, ist diese"
			warn "Verbindung weg (auch SSH darueber). Den Pi vorher per Ethernet anschliessen."
		fi
		if [ "$eth_up" = "0" ]; then
			warn "Kein Ethernet verbunden: Mit Hotspot hat der Pi dann kein Internet fuer das Spiel."
		fi
		if nmcli -t -f NAME connection show --active | grep -qx "$CON"; then
			ok "Hotspot ist an."
		elif ask "Hotspot jetzt einschalten?" n; then
			sudo sh "$HOST_DIR/tools/hotspot.sh" on
		else
			info "Einschalten spaeter: sudo $HOST_DIR/tools/hotspot.sh on"
		fi
		if [ "$(nmcli -g connection.autoconnect connection show "$CON" 2>/dev/null)" = "yes" ]; then
			ok "Hotspot startet beim Booten."
		elif ask "Hotspot bei jedem Start des Pi automatisch einschalten?" n; then
			sudo sh "$HOST_DIR/tools/hotspot.sh" autostart on
		fi
	fi
fi

# ---------------------------------------------------------------- 5. Einstellungen + Start-Skript

step "5/6 Einstellungen ($CONF) und Start-Skript ($STARTER)"
mkdir -p "$(dirname "$CONF")"
if [ -f "$CONF" ]; then
	ok "Einstellungen vorhanden, bleiben wie sie sind."
	if [ -n "$URL" ]; then
		if ask "Adresse in $CONF auf \"$URL\" aendern?" j; then
			tmp="$(mktemp)"
			URLQ="$(shell_quote "$URL")" awk '/^DSSTREAM_URL=/ { print "DSSTREAM_URL=" ENVIRON["URLQ"]; done = 1; next }
				{ print } END { if (!done) print "DSSTREAM_URL=" ENVIRON["URLQ"] }' "$CONF" > "$tmp"
			cat "$tmp" > "$CONF"
			rm -f "$tmp"
			ok "Adresse geaendert."
		fi
	fi
else
	if [ -z "$URL" ]; then
		info "Adresse des Browser-Spiels (mit https://), \"test\" = eingebaute Testseite."
		info "Spaeter aendern: nano $CONF"
		URL="$(ask_value "Adresse:" test)"
	fi
	URLQ="$(shell_quote "$URL")" PYQ="$(shell_quote "$VENV/bin/python")" awk '
		/^DSSTREAM_URL=/ { print "DSSTREAM_URL=" ENVIRON["URLQ"]; next }
		/^DSSTREAM_PYTHON=/ { print "DSSTREAM_PYTHON=" ENVIRON["PYQ"]; next }
		{ print }' "$HOST_DIR/systemd/dsstream.env" > "$CONF"
	ok "Angelegt mit DSSTREAM_URL=$URL"
fi

write_starter() {
	cat > "$STARTER" <<EOF
#!/bin/sh
# DsRunner Stream starten ($MARKER, wird dort neu geschrieben).
# Adresse des Spiels und weitere Einstellungen: ~/.config/dsstream.env (DSSTREAM_URL=...)
# Optionen werden weitergegeben, z. B.: ~/dsstream.sh --url test
HOST_DIR=$(shell_quote "$HOST_DIR")
CON="$CON"
if command -v nmcli >/dev/null && nmcli -t -f NAME connection show | grep -qx "\$CON" &&
	! nmcli -t -f NAME connection show --active | grep -qx "\$CON"; then
	echo "Hinweis: Der Hotspot \$CON ist aus. Einschalten: sudo \$HOST_DIR/tools/hotspot.sh on"
fi
if [ -n "\$DISPLAY" ] || [ -n "\$WAYLAND_DISPLAY" ]; then
	# with a window Chromium uses the GPU of the Pi (WebGL)
	exec "\$HOST_DIR/start.sh" --headful "\$@"
fi
echo "Kein Desktop (z. B. SSH): Chromium laeuft ohne Fenster und ohne GPU."
exec "\$HOST_DIR/start.sh" "\$@"
EOF
	chmod +x "$STARTER"
}
if [ -f "$STARTER" ] && ! grep -q "$MARKER" "$STARTER"; then
	warn "$STARTER gibt es schon und stammt nicht von diesem Skript."
	if ask "Ersetzen (die alte Datei wird nach $STARTER.bak verschoben)?" n; then
		mv "$STARTER" "$STARTER.bak"
		write_starter
		ok "Start-Skript geschrieben, alte Datei: $STARTER.bak"
	else
		info "Bleibt unveraendert. Starten dann mit: $HOST_DIR/start.sh --headful"
	fi
else
	write_starter
	ok "Start-Skript geschrieben."
fi
chmod +x "$HOST_DIR/start.sh" "$HOST_DIR/tools/hotspot.sh" 2>/dev/null || true

# ---------------------------------------------------------------- 6. Autostart

step "6/6 Automatisch starten (optional)"
if [ -f "$AUTOSTART" ] && grep -q "$MARKER" "$AUTOSTART"; then
	ok "Autostart nach der Desktop-Anmeldung ist an ($AUTOSTART)."
	info "Abschalten: rm $AUTOSTART"
elif [ -f "$AUTOSTART" ]; then
	info "$AUTOSTART gibt es schon (nicht von diesem Skript), bleibt unveraendert."
else
	info "Startet DsRunner Stream nach der Anmeldung am Desktop, mit Fenster und GPU."
	if ask "Autostart einrichten?" n; then
		mkdir -p "$(dirname "$AUTOSTART")"
		cat > "$AUTOSTART" <<EOF
[Desktop Entry]
# DsRunner Stream nach der Desktop-Anmeldung ($MARKER). Abschalten: Datei loeschen.
Type=Application
Name=DsRunner Stream
Comment=Browser-Spiele auf den Nintendo DS streamen
Exec=sh -c 'sleep 5; exec "$STARTER"'
Terminal=false
X-GNOME-Autostart-enabled=true
EOF
		ok "Autostart eingerichtet. Abschalten: rm $AUTOSTART"
		info "Damit der Pi ohne Anmeldung bis zum Desktop startet:"
		info "sudo raspi-config -> System Options -> Boot / Auto Login -> Desktop Autologin"
	fi
fi

# ---------------------------------------------------------------- fertig

step "Fertig"
cat <<EOF
   So geht es weiter:
   1. Adresse des Spiels eintragen (falls noch nicht):  nano $CONF
      Zeile DSSTREAM_URL='https://...'   ("test" = Testseite)
   2. Hotspot an (falls noch nicht):  sudo $HOST_DIR/tools/hotspot.sh on
   3. Starten:  $STARTER
      Im Terminal steht dann "Warte auf den DS an UDP-Port 7770".
   4. Am DS in DsRunner den Ordner Apps oeffnen und DsRunner Stream starten
      (die Datei liegt im DsRunner-Paket unter Games/Apps/DsRunnerStream.nds).

   Hilfe bei Problemen: $README, Abschnitt Fehlerbehebung.
EOF
