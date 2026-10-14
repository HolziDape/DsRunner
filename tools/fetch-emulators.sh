#!/bin/sh
# Puts the emulators DsRunner finds by itself (DsRunnerEmulators.h) into a package:
#   _dsrunner/emulators/GBARunner2_arm7dldi_ds.nds   GBA, release of the author
#   _dsrunner/emulators/nesDS.nds                    NES        \
#   _dsrunner/emulators/GameYob.nds                  GB, GBC     | from the Virtual Console
#   _dsrunner/emulators/SNEmulDS-legacy.nds          SNES        | add-on of TWiLight Menu++
#   snemul.cfg                                       settings of SNEmulDS (read from the root)
#   _dsrunner/emulators/LIZENZEN.txt + licenses/     sources and licenses (from licenses/emulators)
# Only emulators whose license allows redistribution; S8DS, NitroGrafx and jEnesisDS are left
# out (see licenses/emulators/LIZENZEN.txt). Never add the GBA BIOS (Nintendo copyright).
# Usage: tools/fetch-emulators.sh <package dir>   (needs curl and 7z; used by make-sd-package.sh
# and for DsRunner.zip in .github/workflows/release.yml)
set -e

# keep TWILIGHT_VERSION the same as in make-sd-package.sh
TWILIGHT_VERSION="v27.25.1"
GBARUNNER2_VERSION="v20200812-131430_6e4ce45"
EMU_DIR="_dsrunner/emulators"
# file in the TWiLight add-on (under _nds/TWiLightMenu/emulators/) -> name DsRunner looks for
TWILIGHT_EMULATORS="nesDS.nds:nesDS.nds gameyob.nds:GameYob.nds SNEmulDS-legacy.nds:SNEmulDS-legacy.nds"

# expected sha256 of every download and extracted file; update together with the versions above
SHA256_VC_7Z="2ee80e97ca4c1c9cac1981a18c74a74cd6356ae6e4f36457c0948a3c5f33b74d"
SHA256_FILES="
c87325c647bcfcc7887827acd87585af22595e01d94791e2f211a0b71d94142d  GBARunner2_arm7dldi_ds.nds
746bd85d0cd040e37e44438f06f9027a3221c57df0f8135abd46e6e2a6864da8  nesDS.nds
357d1dfaed138240f78076efce1bda11e5ac43926a8d773ac129b4e1632fc1ac  GameYob.nds
6ec30da30098d8b3632fc3107b1a9cbc594011fa69992fe04ac6a13cb6c4e599  SNEmulDS-legacy.nds
f0839dfdffa30f55f02058f78db0fd367f90297505783f9f86d28486a6f96705  snemul.cfg
"

# verify_sha256 <file> <expected hash>
verify_sha256() {
	actual="$(sha256sum "$1" | cut -d' ' -f1)"
	if [ "$actual" != "$2" ]; then
		echo "Checksum mismatch for $1 (expected $2, got $actual)"
		exit 1
	fi
}

# expected_sha256 <file name>
expected_sha256() {
	echo "$SHA256_FILES" | grep "  $1\$" | cut -d' ' -f1
}

# download <url> <file>
download() {
	curl -fsSL --retry 5 --retry-all-errors --connect-timeout 20 -o "$2" "$1"
}

[ -n "$1" ] || { echo "Usage: $0 <package dir>"; exit 1; }
OUT="$1"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT/$EMU_DIR/licenses"

echo "Emulators: GBARunner2 $GBARUNNER2_VERSION"
download "https://github.com/Gericom/GBARunner2/releases/download/$GBARUNNER2_VERSION/GBARunner2_arm7dldi_ds.nds" "$TMP/GBARunner2_arm7dldi_ds.nds"
verify_sha256 "$TMP/GBARunner2_arm7dldi_ds.nds" "$(expected_sha256 GBARunner2_arm7dldi_ds.nds)"
cp "$TMP/GBARunner2_arm7dldi_ds.nds" "$OUT/$EMU_DIR/"

echo "Emulators: nesDS, GameYob, SNEmulDS (TWiLight Menu++ $TWILIGHT_VERSION Virtual Console add-on)"
download "https://github.com/DS-Homebrew/TWiLightMenu/releases/download/$TWILIGHT_VERSION/AddOn-VirtualConsole-DSi-or-Flashcard.7z" "$TMP/vc.7z"
verify_sha256 "$TMP/vc.7z" "$SHA256_VC_7Z"
for pair in $TWILIGHT_EMULATORS; do
	# stream each known entry to its destination, nothing else is extracted
	from="${pair%%:*}"
	to="${pair#*:}"
	7z e -so "$TMP/vc.7z" "_nds/TWiLightMenu/emulators/$from" > "$OUT/$EMU_DIR/$to"
	verify_sha256 "$OUT/$EMU_DIR/$to" "$(expected_sha256 "$to")"
done
# SNEmulDS reads its settings (per-game fixes) from the root of the card
7z e -so "$TMP/vc.7z" "snemul.cfg" > "$OUT/snemul.cfg"
verify_sha256 "$OUT/snemul.cfg" "$(expected_sha256 snemul.cfg)"

# sources and licenses; LIZENZEN.txt with CRLF so it opens anywhere on Windows
sed 's/$/\r/' "$ROOT/licenses/emulators/LIZENZEN.txt" > "$OUT/$EMU_DIR/LIZENZEN.txt"
for f in GPL-2.0.txt GameYob-MIT.txt nesDS-PublicDomain.txt; do
	sed 's/$/\r/' "$ROOT/licenses/emulators/$f" > "$OUT/$EMU_DIR/licenses/$f"
done
