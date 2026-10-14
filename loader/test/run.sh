#!/bin/sh
# Offline checks of the in-game reset (no DS needed). Run after loader/build.sh:
#   1. irq_dispatcher_search_test.cpp: the real dispatcher search from the patched source,
#      compiled for the pc, on synthetic arm9 images (host g++)
#   2. igr_log_test.cpp: the text of /_pico/igr-log.txt (igrLog.cpp on top of a stdio FatFs)
#   3. igr_patchcode_test.py: the assembled patch code from the built picoLoader9 elf, run in
#      an ARM emulator (python3 with: pip install unicorn pyelftools)
# Usage: loader/test/run.sh [pico-loader source dir]   (default: loader/work/pico-loader)
set -e

TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="${1:-$TEST_DIR/../work/pico-loader}"
SRC="$(cd "$SRC" && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

IGR="$SRC/arm9/source/patches/arm9/InGameReset"
echo "== dispatcher search ($SRC)"
# -fpermissive: the code casts pointers to u32 as on the DS; the test image is mapped at its DS address
g++ -std=gnu++20 -fpermissive -w -DARM9 \
	-I "$TEST_DIR/host" -I "$SRC/common" -I "$IGR" \
	"$TEST_DIR/irq_dispatcher_search_test.cpp" "$IGR/IrqDispatcher.cpp" "$IGR/IrqDispatcherSdk.cpp" \
	-o "$OUT/irq_dispatcher_search_test"
"$OUT/irq_dispatcher_search_test"

echo
echo "== /_pico/igr-log.txt"
gcc -c -w -I "$TEST_DIR/host" -I "$SRC/common" "$SRC/common/core/mini-printf.c" -o "$OUT/mini-printf.o"
g++ -std=gnu++20 -w \
	-I "$TEST_DIR/host7" -I "$TEST_DIR/host" -I "$SRC/common" -I "$SRC/arm7/source" \
	"$TEST_DIR/igr_log_test.cpp" "$SRC/arm7/source/fat/igrLog.cpp" "$OUT/mini-printf.o" \
	-o "$OUT/igr_log_test"
"$OUT/igr_log_test"

echo
echo "== patch code in the emulator"
python3 "$TEST_DIR/igr_patchcode_test.py" "$SRC/build/picoLoader9_${PICO_PLATFORM:-M3DS}.elf"
