#!/bin/sh
# Builds GBARunner2 variants inside the pinned devkitARM docker image.
# Usage: ./build.sh [variant...]   (default: all)
# Variants: arm9dldi_ds arm7dldi_ds arm7dldi_ds_rom3M arm7dldi_dsi arm7dldi_3ds
set -e

DEVKITARM_IMAGE="${DEVKITARM_IMAGE:-devkitpro/devkitarm:20200730}"

if [ -z "$DEVKITARM" ]; then
	# not inside the toolchain container: re-run this script inside it
	exec docker run --rm -v "$(pwd)":/GBARunner2 -w /GBARunner2 "$DEVKITARM_IMAGE" sh ./build.sh "$@"
fi

git config --global --add safe.directory /GBARunner2 2>/dev/null || true

VARIANTS="$*"
[ -z "$VARIANTS" ] && VARIANTS="arm9dldi_ds arm7dldi_ds arm7dldi_ds_rom3M arm7dldi_dsi arm7dldi_3ds"

TARGET=$(basename "$(pwd)")
mkdir -p build

for v in $VARIANTS; do
	case $v in
		arm9dldi_ds)       FLAGS="" ;;
		arm7dldi_ds)       FLAGS="ARM7_DLDI=1" ;;
		arm7dldi_ds_rom3M) FLAGS="ARM7_DLDI=1 ROM3M=1" ;;
		arm7dldi_dsi)      FLAGS="ARM7_DLDI=1 USE_DSI_16MB=1" ;;
		arm7dldi_3ds)      FLAGS="ARM7_DLDI=1 USE_3DS_32MB=1" ;;
		*) echo "unknown variant: $v"; exit 1 ;;
	esac
	echo "=== Building $v ($FLAGS)"
	make clean >/dev/null
	make all $FLAGS
	echo "--- ARM9 memory usage ($v)"
	sh tools/memreport.sh "arm9/$TARGET.elf"
	mv "$TARGET.nds" "build/DsRunner_$v.nds"
done
make clean >/dev/null
ls -l build
