#!/bin/sh
# Builds DsRunnerStream.nds inside the pinned BlocksDS docker image (same as DsRunner).
# The image ships DSWiFi (libdswifi9 + arm7_dswifi.elf), nothing else is needed.
# Usage: stream/ds/build.sh [make args]      e.g. stream/ds/build.sh clean
set -e

BLOCKSDS_IMAGE="${BLOCKSDS_IMAGE:-skylyrac/blocksds:slim-v1.16.0}"
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"

if [ -z "$BLOCKSDS" ]; then
	# not inside the toolchain container: run make there (repo root mounted, the
	# Makefile uses ../../icon.png)
	exec docker run --rm -v "$ROOT":/DsRunner -w /DsRunner/stream/ds "$BLOCKSDS_IMAGE" sh ./build.sh "$@"
fi

# inside the container (CI) the working directory can be the repo root
cd "$HERE"
make -j"$(nproc)" DSSTREAM_VERSION="$(tr -d ' \t\r\n' < ../../VERSION)" "$@"
