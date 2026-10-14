#!/bin/sh
# Builds Pico Loader with the DsRunner patches (in-game reset back to the launcher):
#   1. clones LNH-team/pico-loader at the commit pinned in loader/UPSTREAM (with submodules),
#      reusing the clone in the work dir when there is one
#   2. applies loader/patches/*.patch in order with git am
#   3. builds the loader for PICO_PLATFORM (default M3DS) with BlocksDS, and aplist.bin,
#      savelist.bin and patchlist.bin with .NET, like the upstream CI
#   4. copies picoLoader7.bin, picoLoader9.bin, aplist.bin, patchlist.bin, savelist.bin
#      and BUILDINFO.txt to the output dir
# Usage: loader/build.sh [output dir]          (default: loader/out)
#        loader/build.sh --id                  prints the id of the current sources
# Toolchain: BlocksDS when $BLOCKSDS is set (inside the BlocksDS docker image or with a local
# install), otherwise docker with the image from loader/UPSTREAM. .NET 9 when "dotnet" is
# found, otherwise docker with the .NET SDK image.
# Environment (all optional): PICO_PLATFORM, LOADER_WORK_DIR (default loader/work),
# PICO_LOADER_BLOCKSDS_IMAGE, PICO_LOADER_DOTNET_IMAGE
set -e

LOADER_DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_BLOCKSDS_IMAGE="$PICO_LOADER_BLOCKSDS_IMAGE"
# shellcheck disable=SC1091
. "$LOADER_DIR/UPSTREAM"

# identifies the sources of a build: upstream commit + hash of all patches
# (tools/make-sd-package.sh rebuilds loader/out when it doesn't match)
BUILD_ID="$PICO_LOADER_COMMIT-$(cat "$LOADER_DIR"/patches/*.patch | sha256sum | cut -c1-16)"
if [ "$1" = "--id" ]; then
	echo "$BUILD_ID"
	exit 0
fi

PICO_PLATFORM="${PICO_PLATFORM:-M3DS}"
OUT="${1:-$LOADER_DIR/out}"
WORK="${LOADER_WORK_DIR:-$LOADER_DIR/work}"
BLOCKSDS_IMAGE="${ENV_BLOCKSDS_IMAGE:-$PICO_LOADER_BLOCKSDS_IMAGE}"
DOTNET_IMAGE="${PICO_LOADER_DOTNET_IMAGE:-mcr.microsoft.com/dotnet/sdk:$PICO_LOADER_DOTNET_VERSION}"
LOADER_FILES="aplist.bin patchlist.bin picoLoader7.bin picoLoader9.bin savelist.bin"

mkdir -p "$WORK"
WORK="$(cd "$WORK" && pwd)"
SRC="$WORK/pico-loader"

echo "Pico Loader $PICO_LOADER_COMMIT + $(ls "$LOADER_DIR"/patches/*.patch | wc -l) patches ($PICO_PLATFORM)"

# 1. clone or reuse, then reset to the pinned commit
git config --global --add safe.directory "$SRC" 2>/dev/null || true
git config --global --add safe.directory "$SRC/libs/libtwl" 2>/dev/null || true
if [ ! -d "$SRC/.git" ]; then
	rm -rf "$SRC"
	git clone --quiet "$PICO_LOADER_REPO" "$SRC"
fi
git -C "$SRC" remote set-url origin "$PICO_LOADER_REPO"
if ! git -C "$SRC" cat-file -e "$PICO_LOADER_COMMIT^{commit}" 2>/dev/null; then
	git -C "$SRC" fetch --quiet origin
fi
# a failed earlier run can leave a patch half applied
git -C "$SRC" am --abort > /dev/null 2>&1 || true
git -C "$SRC" checkout --quiet --force --detach "$PICO_LOADER_COMMIT"
git -C "$SRC" clean --quiet -ffdx
git -C "$SRC" submodule --quiet update --init --recursive --force

# 2. patches, committed with a fixed identity so the result does not depend on the git config
git -C "$SRC" -c user.name="DsRunner loader build" -c user.email="dsrunner@invalid" \
	am --quiet --keep-cr "$LOADER_DIR"/patches/*.patch

# docker runs with the caller's ids, so the work dir stays writable for the next run;
# HOME in the work dir keeps the nuget cache there
DOCKER_ARGS="--rm --user $(id -u):$(id -g) -e HOME=/work/.home -v $WORK:/work -w /work/pico-loader"
mkdir -p "$WORK/.home"

# 3. build (DSRUNNER_BUILD_ID ends up in /_pico/igr-log.txt, patch 0015)
if [ -n "$BLOCKSDS" ]; then
	make -C "$SRC" -j"$(nproc)" PICO_PLATFORM="$PICO_PLATFORM" DSRUNNER_BUILD_ID="$BUILD_ID" checklibtwl loader9 loader7
else
	# shellcheck disable=SC2086
	docker run $DOCKER_ARGS "$BLOCKSDS_IMAGE" \
		sh -c "make -j\$(nproc) PICO_PLATFORM=$PICO_PLATFORM DSRUNNER_BUILD_ID=$BUILD_ID checklibtwl loader9 loader7"
fi

if command -v dotnet > /dev/null 2>&1; then
	make -C "$SRC" apList saveList patchList
else
	# The converter restores its packages from nuget.org. Host network, so a proxy on
	# localhost (HTTPS_PROXY) also works in the container, and the CA file of SSL_CERT_FILE
	# for proxies that inspect TLS.
	CA_ARGS=""
	if [ -n "$SSL_CERT_FILE" ] && [ -f "$SSL_CERT_FILE" ]; then
		CA_ARGS="-v $SSL_CERT_FILE:/etc/ssl/dsrunner-ca.crt:ro -e SSL_CERT_FILE=/etc/ssl/dsrunner-ca.crt"
	fi
	# shellcheck disable=SC2086
	docker run $DOCKER_ARGS --network host $CA_ARGS \
		-e HTTPS_PROXY -e HTTP_PROXY -e NO_PROXY -e DOTNET_CLI_HOME=/work/.home \
		-e DOTNET_CLI_TELEMETRY_OPTOUT=1 -e DOTNET_NOLOGO=1 -e DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1 \
		"$DOTNET_IMAGE" sh -c '
			set -e
			dotnet build -nologo -v q tools/PicoLoaderConverter/PicoLoaderConverter.sln
			c=tools/PicoLoaderConverter/PicoLoaderConverter/bin/Debug/net9.0/PicoLoaderConverter.dll
			dotnet $c aplist -i data/aplist.csv -o data/aplist.bin
			dotnet $c savelist -i data/savelist.csv -o data/savelist.bin
			dotnet $c patchlist -i data/patchlist.json -o data/patchlist.bin'
fi

# 4. output, same file names as in the upstream release zip
mkdir -p "$OUT"
cp "$SRC/picoLoader7.bin" "$OUT/picoLoader7.bin"
cp "$SRC/picoLoader9_$PICO_PLATFORM.bin" "$OUT/picoLoader9.bin"
for f in aplist.bin patchlist.bin savelist.bin; do
	cp "$SRC/data/$f" "$OUT/$f"
done
{
	echo "Pico Loader for DsRunner ($PICO_PLATFORM)"
	echo "id: $BUILD_ID"
	echo "upstream: $PICO_LOADER_REPO @ $PICO_LOADER_COMMIT"
	echo "patches:"
	for p in "$LOADER_DIR"/patches/*.patch; do
		echo "  $(basename "$p")"
	done
	echo "sha256:"
	# shellcheck disable=SC2086
	(cd "$OUT" && sha256sum $LOADER_FILES | sed 's/^/  /')
} > "$OUT/BUILDINFO.txt"
cat "$OUT/BUILDINFO.txt"
