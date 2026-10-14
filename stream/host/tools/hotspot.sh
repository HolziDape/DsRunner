#!/bin/sh
# DsRunner Stream: open WiFi hotspot for the DS on a Raspberry Pi (Raspberry Pi OS
# Bookworm, NetworkManager). The Pi itself stays online over Ethernet (eth0);
# wlan0 becomes an open 802.11b/g access point only for the DS.
#
#   sudo tools/hotspot.sh setup [SSID] [CHANNEL]  create the connection once
#                                                 (default DsRunner-Stream, channel 6)
#   sudo tools/hotspot.sh on                      switch the hotspot on
#   sudo tools/hotspot.sh off                     switch it off again
#   sudo tools/hotspot.sh status                  state, address, connected devices
#   sudo tools/hotspot.sh lock [DS-MAC]           firewall on wlan0: only DHCP, DNS and
#                                                 the stream port reach the Pi, nothing is
#                                                 routed to the internet; with a MAC only
#                                                 that DS (shown by DsRunnerStream)
#   sudo tools/hotspot.sh unlock                  remove that firewall again
#   sudo tools/hotspot.sh autostart on|off        hotspot on at every boot (or not)
#
# Environment: IFACE (wlan0), ADDR (192.168.4.1/24), PORT (7770)
set -e

CON="DsRunner-Stream"
IFACE="${IFACE:-wlan0}"
ADDR="${ADDR:-192.168.4.1/24}"
PORT="${PORT:-7770}"

need_root() {
	[ "$(id -u)" = "0" ] || { echo "Bitte mit sudo starten."; exit 1; }
}

case "$1" in
setup)
	need_root
	SSID="${2:-DsRunner-Stream}"
	CHANNEL="${3:-6}"
	rfkill unblock wifi 2>/dev/null || true
	if nmcli -t -f NAME connection show | grep -qx "$CON"; then
		nmcli connection delete "$CON" >/dev/null
	fi
	# no wifi-sec settings = open network (the DS Lite can't do WPA)
	# band bg = 2.4 GHz with 802.11b rates, which the DS Lite needs
	# ipv4.method shared = NetworkManager runs DHCP (dnsmasq) for the DS
	nmcli connection add type wifi ifname "$IFACE" con-name "$CON" autoconnect no ssid "$SSID" \
		802-11-wireless.mode ap 802-11-wireless.band bg 802-11-wireless.channel "$CHANNEL" \
		802-11-wireless.powersave 2 \
		ipv4.method shared ipv4.addresses "$ADDR" ipv6.method disabled >/dev/null
	echo "Hotspot \"$SSID\" (Kanal $CHANNEL, offen) angelegt. Einschalten: sudo $0 on"
	;;
on)
	need_root
	rfkill unblock wifi 2>/dev/null || true
	nmcli connection up "$CON"
	echo "Hotspot an. Adresse des Pi im Hotspot: ${ADDR%/*}"
	;;
off)
	need_root
	nmcli connection down "$CON"
	;;
status)
	nmcli -f GENERAL.STATE,GENERAL.CONNECTION,IP4.ADDRESS device show "$IFACE" || true
	echo "Verbundene Geraete:"
	iw dev "$IFACE" station dump 2>/dev/null | grep -i '^station' || echo "  keine"
	nft list table inet dsstream >/dev/null 2>&1 && echo "Firewall (lock): an" || echo "Firewall (lock): aus"
	;;
lock)
	need_root
	MACRULE=""
	if [ -n "$2" ]; then
		MACRULE="iifname \"$IFACE\" ether saddr != $2 drop"
	fi
	nft delete table inet dsstream 2>/dev/null || true
	nft -f - <<EOF
table inet dsstream {
	chain input {
		type filter hook input priority -5; policy accept;
		$MACRULE
		iifname "$IFACE" udp dport { 53, 67, $PORT } accept
		iifname "$IFACE" tcp dport 53 accept
		iifname "$IFACE" icmp type echo-request accept
		iifname "$IFACE" ct state established,related accept
		iifname "$IFACE" drop
	}
	chain forward {
		type filter hook forward priority -5; policy accept;
		iifname "$IFACE" drop
		oifname "$IFACE" drop
	}
}
EOF
	echo "Firewall an: im Hotspot nur DHCP, DNS und UDP $PORT${2:+, nur MAC $2}. Gilt bis zum Neustart."
	;;
unlock)
	need_root
	nft delete table inet dsstream 2>/dev/null || true
	echo "Firewall aus."
	;;
autostart)
	need_root
	case "$2" in
	on) nmcli connection modify "$CON" connection.autoconnect yes; echo "Hotspot startet jetzt beim Booten." ;;
	off) nmcli connection modify "$CON" connection.autoconnect no; echo "Hotspot startet nicht mehr beim Booten." ;;
	*) echo "autostart on|off"; exit 1 ;;
	esac
	;;
*)
	sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
	exit 1
	;;
esac
