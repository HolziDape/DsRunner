#!/bin/sh
# Assembles a ready-to-copy SD card package for the M3 DS Real:
#   _nds/TWiLightMenu/main.srldr
#                        DsRunner; this is the file the autoboot chain starts
#   _pico/               themes + Pico Loader for the M3DS platform
#   SYSTEM/, boot.*, loader.eng, _system_/_sys_data/r4i.sys, SRESET.dat
#                        M3 DS Real autoboot files from the TWiLight Menu++ release:
#                        firmware -> SYSTEM/g6dsload.eng -> r4i.sys (TWiLight booter with
#                        the M3 DLDI driver) -> /_nds/TWiLightMenu/main.srldr
#   BOOT.NDS             DsRunner again, to start it manually from other menus
#   LIESMICH.txt         short German installation and update guide with the version
#                        (from VERSION, marked as development state unless DSRUNNER_RELEASE=1)
# Usage: tools/make-sd-package.sh [output dir]   (run after make, needs curl, unzip, 7z)
set -e

PICO_LOADER_VERSION="v1.7.1"
PICO_PLATFORM="M3DS"
TWILIGHT_VERSION="v27.25.1"
AUTOBOOT_DIR="Autoboot/M3 DS Real, M3i Zero (non-GMP-Z003), iTouchDS, R4(i) SDHC (r4rts.com)"
AUTOBOOT_FILES="SYSTEM/g6dsload.1 SYSTEM/g6dsload.eng boot.eng boot.gb boot.jp loader.eng _system_/_sys_data/r4i.sys SRESET.dat"
LOADER_FILES="aplist.bin patchlist.bin picoLoader7.bin picoLoader9.bin savelist.bin"
# path the TWiLight booter (r4i.sys) starts
BOOT_TARGET="_nds/TWiLightMenu/main.srldr"

# expected sha256 of every download; update together with the pinned versions above
SHA256_LOADER_ZIP="e8fc3e965c929c4ed75d58682c711967521463b0caaed76e5ad89ce594037b9b"
SHA256_TWILIGHT_7Z="d53c903481c2ac9f98d369389e2aded394e89600ea69a4a3f770e1e6b6810b62"
SHA256_AUTOBOOT="
c1f756ce847d66ad8f4bcbe6f568ba09030b0c33a54155bce73ed5479aaf73c7  SYSTEM/g6dsload.1
48a01008fe95f83dbec01ee4b32db1da78a1a82ae3559b325c388969894f52e6  SYSTEM/g6dsload.eng
a19e4de0fec73c19de443c9f81c275a898e2f5c8dfd0faf5c3e9785f1c260a51  boot.eng
0856a761926df72052dd4fb0aef05f47124aef08f288e0a82148da7febc3a62d  boot.gb
3367c3d37a3fbe1ecc3728a951e8f690044291a197260971abb267476aee2082  boot.jp
b2094053a5e30064d326aaeaa588511c6850123890ff6614ede8b909bef59895  loader.eng
776e387f7c219fb337ae3e2dd683eb23c810f007513b54959988c29b20ece5da  _system_/_sys_data/r4i.sys
776e387f7c219fb337ae3e2dd683eb23c810f007513b54959988c29b20ece5da  SRESET.dat
"

# verify_sha256 <file> <expected hash>
verify_sha256() {
	actual="$(sha256sum "$1" | cut -d' ' -f1)"
	if [ "$actual" != "$2" ]; then
		echo "Checksum mismatch for $1 (expected $2, got $actual)"
		exit 1
	fi
}

# download <url> <file>
download() {
	curl -fsSL --retry 5 --retry-all-errors --connect-timeout 20 -o "$2" "$1"
}

OUT="${1:-sd-package}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

[ -f "$ROOT/DsRunner.nds" ] || { echo "DsRunner.nds not found, run make first"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/_pico" "$OUT/SYSTEM" "$OUT/_system_/_sys_data" "$(dirname "$OUT/$BOOT_TARGET")"

cp "$ROOT/DsRunner.nds" "$OUT/$BOOT_TARGET"
cp "$ROOT/DsRunner.nds" "$OUT/BOOT.NDS"
cp -r "$ROOT/_pico/." "$OUT/_pico/"

echo "Pico Loader $PICO_LOADER_VERSION ($PICO_PLATFORM)"
download "https://github.com/LNH-team/pico-loader/releases/download/$PICO_LOADER_VERSION/Pico_Loader_$PICO_PLATFORM.zip" "$TMP/loader.zip"
verify_sha256 "$TMP/loader.zip" "$SHA256_LOADER_ZIP"
# extract only the known file names into a scratch dir, then copy them over
mkdir "$TMP/loader"
for f in $LOADER_FILES; do
	unzip -q -j "$TMP/loader.zip" "$f" -d "$TMP/loader"
	cp "$TMP/loader/$f" "$OUT/_pico/$f"
done

echo "Autoboot files (TWiLight Menu++ $TWILIGHT_VERSION)"
download "https://github.com/DS-Homebrew/TWiLightMenu/releases/download/$TWILIGHT_VERSION/TWiLightMenu-Flashcard.7z" "$TMP/twilight.7z"
verify_sha256 "$TMP/twilight.7z" "$SHA256_TWILIGHT_7Z"
for f in $AUTOBOOT_FILES; do
	# stream each known entry to its destination, nothing else is extracted
	7z e -so "$TMP/twilight.7z" "$AUTOBOOT_DIR/$f" > "$OUT/$f"
	verify_sha256 "$OUT/$f" "$(echo "$SHA256_AUTOBOOT" | grep "  $f\$" | cut -d' ' -f1)"
done

# short German guide; plain ASCII with CRLF so it opens anywhere on Windows
VERSION_TEXT="$(tr -d ' \t\r\n' < "$ROOT/VERSION")"
[ "$DSRUNNER_RELEASE" = "1" ] || VERSION_TEXT="$VERSION_TEXT (Entwicklungsstand)"
cat > "$OUT/LIESMICH.txt" <<'EOF'
DsRunner @VERSION@ fuer M3 DS Real
==================================

Installation
------------
1. SD-Karte sichern (vor allem alle .sav Dateien, den Ordner SYSTEM und SRESET.DAT).
   Damit kommst du jederzeit zu YSMenu zurueck.
2. Den INHALT dieses Ordners ins Hauptverzeichnis der SD-Karte kopieren
   und vorhandene Dateien ersetzen.
3. Spiele in Ordner pro Konsole legen, zum Beispiel:
     Games\NDS   fuer .nds Spiele
     Games\GBA   fuer .gba Spiele (dazu GBARunner2_arm7dldi_ds.nds ins
                 Hauptverzeichnis und das GBA-BIOS als _gba\bios.bin)
4. DS einschalten: DsRunner startet automatisch.

Update auf eine neue Version
----------------------------
Nur die Datei _nds\TWiLightMenu\main.srldr aus dem neuen Paket auf die Karte
kopieren und die alte ersetzen (optional auch BOOT.NDS). Spiele, Spielstaende und
Einstellungen bleiben erhalten. Welche Version laeuft, steht unten rechts im
Menue "Anzeige" (Zahnrad oben rechts).

Bilder fuer den oberen Bildschirm und die Kacheln
-------------------------------------------------
Das PC-Programm DsRunnerArt.exe (beim selben Release) laedt sie auf die Karte:
SD-Karte einstecken, Programm starten, "Artworks aktualisieren" klicken.

Zurueck zu YSMenu
-----------------
Die gesicherten Dateien SYSTEM\g6dsload.1, SYSTEM\g6dsload.eng und SRESET.DAT
zurueckkopieren, dann boot.*, loader.eng und den Ordner _system_ loeschen.

Mehr
----
Anleitung, Bedienung und Aenderungen (README.md, CHANGELOG.md):
https://github.com/HolziDape/DsRunner

DsRunner basiert auf Pico Launcher und Pico Loader (LNH team, Zlib-Lizenz).
Die Autostart-Dateien stammen aus TWiLight Menu++ (DS-Homebrew).
EOF
sed -i "s/@VERSION@/$VERSION_TEXT/; s/\$/\r/" "$OUT/LIESMICH.txt"

find "$OUT" -type f | sort
