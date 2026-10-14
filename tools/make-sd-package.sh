#!/bin/sh
# Assembles a ready-to-copy SD card package for the M3 DS Real:
#   _nds/TWiLightMenu/main.srldr
#                        DsRunner; this is the file the autoboot chain starts
#   _pico/               themes + Pico Loader for the M3DS platform (see DSRUNNER_LOADER below)
#   SYSTEM/, boot.*, loader.eng, _system_/_sys_data/r4i.sys, SRESET.dat
#                        M3 DS Real autoboot files from the TWiLight Menu++ release:
#                        firmware -> SYSTEM/g6dsload.eng -> r4i.sys (TWiLight booter with
#                        the M3 DLDI driver) -> /_nds/TWiLightMenu/main.srldr
#   BOOT.NDS             DsRunner again, to start it manually from other menus
#   _dsrunner/emulators/, snemul.cfg
#                        emulators for GBA, NES, GB/GBC and SNES with their licenses
#                        (tools/fetch-emulators.sh, pinned versions + sha256, no GBA BIOS)
#   LIESMICH.txt         short German installation and update guide with the version
#                        (from VERSION, marked as development state unless DSRUNNER_RELEASE=1)
# Usage: tools/make-sd-package.sh [output dir]   (run after make, needs curl, unzip, 7z)
# Pico Loader, chosen with DSRUNNER_LOADER:
#   custom   (default) the DsRunner build from loader/ with "zurueck zu DsRunner" from games
#            (L+R+Down+B). Taken from DSRUNNER_LOADER_DIR (default loader/out); loader/build.sh
#            runs first when the files are missing or were built from other sources.
#   upstream the unmodified Pico Loader release below, without returning from games
set -e

PICO_LOADER_VERSION="v1.7.1"
PICO_PLATFORM="M3DS"
# keep TWILIGHT_VERSION the same as in fetch-emulators.sh
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
DSRUNNER_LOADER="${DSRUNNER_LOADER:-custom}"
DSRUNNER_LOADER_DIR="${DSRUNNER_LOADER_DIR:-$ROOT/loader/out}"
case "$DSRUNNER_LOADER" in
	custom|upstream) ;;
	*) echo "DSRUNNER_LOADER must be custom or upstream (is: $DSRUNNER_LOADER)"; exit 1 ;;
esac
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

[ -f "$ROOT/DsRunner.nds" ] || { echo "DsRunner.nds not found, run make first"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/_pico" "$OUT/SYSTEM" "$OUT/_system_/_sys_data" "$(dirname "$OUT/$BOOT_TARGET")"

cp "$ROOT/DsRunner.nds" "$OUT/$BOOT_TARGET"
cp "$ROOT/DsRunner.nds" "$OUT/BOOT.NDS"
cp -r "$ROOT/_pico/." "$OUT/_pico/"

if [ "$DSRUNNER_LOADER" = "custom" ]; then
	# rebuild when a file is missing or the build is from other sources (upstream commit, patches)
	LOADER_ID="$(sh "$ROOT/loader/build.sh" --id)"
	BUILT_ID="$(sed -n 's/^id: //p' "$DSRUNNER_LOADER_DIR/BUILDINFO.txt" 2>/dev/null || true)"
	for f in $LOADER_FILES; do
		[ -f "$DSRUNNER_LOADER_DIR/$f" ] || BUILT_ID=""
	done
	if [ "$BUILT_ID" != "$LOADER_ID" ]; then
		echo "Building Pico Loader for DsRunner (loader/build.sh)"
		if ! PICO_PLATFORM="$PICO_PLATFORM" sh "$ROOT/loader/build.sh" "$DSRUNNER_LOADER_DIR"; then
			echo "Pico Loader build failed. DSRUNNER_LOADER=upstream packs the unmodified release instead"
			echo "(then there is no way back to DsRunner from games)."
			exit 1
		fi
	fi
	echo "Pico Loader for DsRunner ($PICO_PLATFORM, $LOADER_ID)"
	for f in $LOADER_FILES; do
		cp "$DSRUNNER_LOADER_DIR/$f" "$OUT/_pico/$f"
	done
	# which loader build is on the card, for bug reports
	cp "$DSRUNNER_LOADER_DIR/BUILDINFO.txt" "$OUT/_pico/picoLoader-DsRunner.txt"
else
	echo "Pico Loader $PICO_LOADER_VERSION ($PICO_PLATFORM, unmodified release)"
	download "https://github.com/LNH-team/pico-loader/releases/download/$PICO_LOADER_VERSION/Pico_Loader_$PICO_PLATFORM.zip" "$TMP/loader.zip"
	verify_sha256 "$TMP/loader.zip" "$SHA256_LOADER_ZIP"
	# extract only the known file names into a scratch dir, then copy them over
	mkdir "$TMP/loader"
	for f in $LOADER_FILES; do
		unzip -q -j "$TMP/loader.zip" "$f" -d "$TMP/loader"
		cp "$TMP/loader/$f" "$OUT/_pico/$f"
	done
fi

echo "Autoboot files (TWiLight Menu++ $TWILIGHT_VERSION)"
download "https://github.com/DS-Homebrew/TWiLightMenu/releases/download/$TWILIGHT_VERSION/TWiLightMenu-Flashcard.7z" "$TMP/twilight.7z"
verify_sha256 "$TMP/twilight.7z" "$SHA256_TWILIGHT_7Z"
for f in $AUTOBOOT_FILES; do
	# stream each known entry to its destination, nothing else is extracted
	7z e -so "$TMP/twilight.7z" "$AUTOBOOT_DIR/$f" > "$OUT/$f"
	verify_sha256 "$OUT/$f" "$(echo "$SHA256_AUTOBOOT" | grep "  $f\$" | cut -d' ' -f1)"
done

sh "$ROOT/tools/fetch-emulators.sh" "$OUT"

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
     Games\GBA   fuer .gba Spiele
     Games\NES, Games\GB, Games\GBC, Games\SNES   fuer NES, Game Boy, SNES
4. DS einschalten: DsRunner startet automatisch.

Emulatoren
----------
Die Emulatoren fuer GBA (GBARunner2), NES (nesDS), Game Boy (GameYob) und
SNES (SNEmulDS) sind schon dabei, im Ordner _dsrunner\emulators.
- GBA: Mit dem GBA-BIOS laufen mehr Spiele. Es darf nicht mitgeliefert werden:
  selbst als _gba\bios.bin auf die Karte legen.
- Master System, Game Gear, Mega Drive und PC Engine: Diese Emulatoren duerfen
  nicht mitgeliefert werden. S8DS.nds, jEnesisDS.nds und NitroGrafx.nds selbst
  herunterladen und nach _dsrunner\emulators kopieren (Links im README).
- Herkunft und Lizenzen: _dsrunner\emulators\LIZENZEN.txt

Update auf eine neue Version
----------------------------
Nur die Datei _nds\TWiLightMenu\main.srldr aus dem neuen Paket auf die Karte
kopieren und die alte ersetzen (optional auch BOOT.NDS). Spiele, Spielstaende und
Einstellungen bleiben erhalten. Welche Version laeuft, steht unten rechts im
Menue "Anzeige" (Zahnrad oben rechts).
Steht im CHANGELOG "Pico Loader geaendert", auch die Dateien picoLoader7.bin,
picoLoader9.bin, aplist.bin, patchlist.bin und savelist.bin im Ordner _pico
ersetzen.
Die Emulatoren kommen nur mit dem kompletten Paket. Fehlen sie auf deiner Karte
noch, einmal den Ordner _dsrunner\emulators und die Datei snemul.cfg kopieren.

EOF
if [ "$DSRUNNER_LOADER" = "custom" ]; then
	cat >> "$OUT/LIESMICH.txt" <<'EOF'
Zurueck zu DsRunner aus einem Spiel
-----------------------------------
Im Spiel L + R + Unten + B zwei Sekunden lang halten: der DS startet DsRunner neu.
- Vorher im Spiel speichern. Nicht gespeicherter Fortschritt geht verloren,
  wie beim Ausschalten. Ein laufender Speichervorgang wird nicht abgebrochen.
- Klappt nicht in jedem Spiel. Reagiert ein Spiel nicht, wie gewohnt ausschalten.
- Abschalten: in der Datei _pico\settings.json "inGameReset": false eintragen.
- Andere Tasten, z. B. wenn L/R ausgeleiert sind: in _pico\settings.json
  "inGameResetKeys": "SELECT+START+DOWN" eintragen (oder "SELECT+DOWN+B").
  Erlaubt: A B SELECT START UP DOWN LEFT RIGHT L R, mindestens 3, nicht X/Y.

EOF
fi
cat >> "$OUT/LIESMICH.txt" <<'EOF'
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
Die Rueckkehr aus Spielen stammt von danyocom (Pico-Loader-Fork, Zlib-Lizenz).
Die Autostart-Dateien stammen aus TWiLight Menu++ (DS-Homebrew).
Die Emulatoren stammen von ihren Autoren, siehe _dsrunner\emulators\LIZENZEN.txt.
EOF
sed -i "s/@VERSION@/$VERSION_TEXT/; s/\$/\r/" "$OUT/LIESMICH.txt"

find "$OUT" -type f | sort
