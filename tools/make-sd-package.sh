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
LOADER_FILES="aplist.bin patchlist.bin picoLoader7.bin picoLoader9.bin savelist.bin"

# expected sha256 of every download; update together with the pinned versions above
SHA256_LOADER_ZIP="e8fc3e965c929c4ed75d58682c711967521463b0caaed76e5ad89ce594037b9b"
SHA256_AUTOBOOT="
c1f756ce847d66ad8f4bcbe6f568ba09030b0c33a54155bce73ed5479aaf73c7  SYSTEM/g6dsload.1
48a01008fe95f83dbec01ee4b32db1da78a1a82ae3559b325c388969894f52e6  SYSTEM/g6dsload.eng
a19e4de0fec73c19de443c9f81c275a898e2f5c8dfd0faf5c3e9785f1c260a51  boot.eng
0856a761926df72052dd4fb0aef05f47124aef08f288e0a82148da7febc3a62d  boot.gb
3367c3d37a3fbe1ecc3728a951e8f690044291a197260971abb267476aee2082  boot.jp
b2094053a5e30064d326aaeaa588511c6850123890ff6614ede8b909bef59895  loader.eng
"

# verify_sha256 <file> <expected hash>
verify_sha256() {
	actual="$(sha256sum "$1" | cut -d' ' -f1)"
	if [ "$actual" != "$2" ]; then
		echo "Checksum mismatch for $1 (expected $2, got $actual)"
		exit 1
	fi
}

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
verify_sha256 "$TMP/loader.zip" "$SHA256_LOADER_ZIP"
# extract only the known file names into a scratch dir, then copy them over
mkdir "$TMP/loader"
for f in $LOADER_FILES; do
	unzip -q -j "$TMP/loader.zip" "$f" -d "$TMP/loader"
	cp "$TMP/loader/$f" "$OUT/_pico/$f"
done

echo "Autoboot files (TWiLight Menu++ ${TWILIGHT_COMMIT%"${TWILIGHT_COMMIT#???????}"})"
for f in $AUTOBOOT_FILES; do
	curl -fsSL --retry 5 --retry-all-errors --connect-timeout 20 -o "$OUT/$f" "https://raw.githubusercontent.com/DS-Homebrew/TWiLightMenu/$TWILIGHT_COMMIT/$AUTOBOOT_DIR/$f"
	verify_sha256 "$OUT/$f" "$(echo "$SHA256_AUTOBOOT" | grep "  $f\$" | cut -d' ' -f1)"
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
