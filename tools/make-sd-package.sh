#!/bin/sh
# Assembles a ready-to-copy SD card package for the M3 DS Real:
#   BOOT.NDS             DsRunner (started by the card on power on)
#   _pico/               themes + Pico Loader for the M3DS platform
#   SYSTEM/, boot.*, loader.eng
#                        M3 DS Real autoboot files from TWiLight Menu++ (they boot /BOOT.NDS)
# Usage: tools/make-sd-package.sh [output dir]   (run after make, needs curl + unzip)
set -e

PICO_LOADER_VERSION="v1.7.1"
PICO_PLATFORM="M3DS"
# TWiLight Menu++ commit the autoboot files are taken from
TWILIGHT_COMMIT="8d3a32ff0b5cb7e588b9d030ab6e88bc2412f1bc"
AUTOBOOT_DIR="7zfile/Flashcard%20users/Autoboot/M3%20DS%20Real%2C%20M3i%20Zero%20(non-GMP-Z003)%2C%20iTouchDS%2C%20R4(i)%20SDHC%20(r4rts.com)"
AUTOBOOT_FILES="SYSTEM/g6dsload.1 SYSTEM/g6dsload.eng boot.eng boot.gb boot.jp loader.eng"

OUT="${1:-sd-package}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

[ -f "$ROOT/DsRunner.nds" ] || { echo "DsRunner.nds not found, run make first"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/_pico" "$OUT/SYSTEM"

cp "$ROOT/DsRunner.nds" "$OUT/BOOT.NDS"
cp -r "$ROOT/_pico/." "$OUT/_pico/"

echo "Pico Loader $PICO_LOADER_VERSION ($PICO_PLATFORM)"
curl -fsSL --retry 5 --retry-all-errors --connect-timeout 20 -o "$TMP/loader.zip" \
	"https://github.com/LNH-team/pico-loader/releases/download/$PICO_LOADER_VERSION/Pico_Loader_$PICO_PLATFORM.zip"
unzip -q -o "$TMP/loader.zip" -d "$OUT/_pico"

echo "Autoboot files (TWiLight Menu++ ${TWILIGHT_COMMIT%"${TWILIGHT_COMMIT#???????}"})"
for f in $AUTOBOOT_FILES; do
	curl -fsSL --retry 5 --retry-all-errors --connect-timeout 20 -o "$OUT/$f" "https://raw.githubusercontent.com/DS-Homebrew/TWiLightMenu/$TWILIGHT_COMMIT/$AUTOBOOT_DIR/$f"
done

cat > "$OUT/LIESMICH.txt" <<'EOF'
DsRunner fuer M3 DS Real
========================
1. SD-Karte sichern (vor allem alle .sav Dateien und den Ordner SYSTEM).
2. Den INHALT dieses Ordners ins Hauptverzeichnis der SD-Karte kopieren
   und vorhandene Dateien ersetzen.
3. DS einschalten: DsRunner startet automatisch.

Zurueck zu YSMenu: die gesicherten Dateien SYSTEM\g6dsload.1 und
SYSTEM\g6dsload.eng zurueckkopieren und boot.*/loader.eng loeschen.

DsRunner basiert auf Pico Launcher und Pico Loader (LNH team, Zlib-Lizenz).
Die Autostart-Dateien stammen aus TWiLight Menu++ (DS-Homebrew).
EOF
sed -i 's/$/\r/' "$OUT/LIESMICH.txt"

find "$OUT" -type f | sort
