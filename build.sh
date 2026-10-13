#!/bin/sh
# Builds DsRunner inside the pinned BlocksDS docker image (same as CI).
# Usage: ./build.sh [make args]      e.g. ./build.sh clean
#        ./build.sh package          build + SD package for the M3 DS Real in sd-package/
set -e

BLOCKSDS_IMAGE="${BLOCKSDS_IMAGE:-skylyrac/blocksds:slim-v1.16.0}"

if [ "$1" = "package" ]; then
	sh ./build.sh
	# packaging only downloads and copies files, so it runs outside the container
	# (https from inside docker on WSL can be unreliable)
	sh tools/make-sd-package.sh sd-package
	exit 0
fi

if [ -z "$BLOCKSDS" ]; then
	# not inside the toolchain container: re-run this script inside it
	exec docker run --rm -v "$(pwd)":/DsRunner -w /DsRunner "$BLOCKSDS_IMAGE" sh ./build.sh "$@"
fi

git config --global --add safe.directory /DsRunner 2>/dev/null || true
make -j"$(nproc)" "$@"
