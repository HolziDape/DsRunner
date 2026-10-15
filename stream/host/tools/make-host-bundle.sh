#!/bin/sh
# Packs the DsRunner Stream host for the release: DsRunnerStream-Host.zip with one folder
# DsRunnerStream-Host/ (the Python host, mappings, systemd examples, tools/install-pi.sh,
# hotspot.sh, the guide README.md from stream/README.md, PROTOCOL.md, QUICKSTART.txt and
# VERSION.txt). Without tests, venvs and caches: Pi users only need to run it.
# Usage: stream/host/tools/make-host-bundle.sh [output zip]   (default DsRunnerStream-Host.zip)
# The version line follows tools/make-sd-package.sh: development state unless DSRUNNER_RELEASE=1.
set -e

HOST="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(cd "$HOST/../.." && pwd)"
OUT="${1:-DsRunnerStream-Host.zip}"
case "$OUT" in
	/*) ;;
	*) OUT="$(pwd)/$OUT" ;;
esac
NAME="DsRunnerStream-Host"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
DEST="$TMP/$NAME"
mkdir -p "$DEST"

for f in dsstream mappings systemd tools requirements.txt start.sh QUICKSTART.txt; do
	cp -R "$HOST/$f" "$DEST/"
done
# the bundle tool itself is of no use inside the bundle
rm -f "$DEST/tools/make-host-bundle.sh"
find "$DEST" \( -name __pycache__ -o -name .pytest_cache -o -name '.venv' -o -name 'venv' \) -prune \
	-exec rm -rf {} +
find "$DEST" \( -name '*.pyc' -o -name '*.pyo' \) -exec rm -f {} +
chmod +x "$DEST/start.sh" "$DEST/tools/"*.sh

cp "$ROOT/stream/README.md" "$DEST/README.md"
cp "$ROOT/stream/PROTOCOL.md" "$DEST/PROTOCOL.md"

VERSION_TEXT="$(tr -d ' \t\r\n' < "$ROOT/VERSION")"
[ "$DSRUNNER_RELEASE" = "1" ] || VERSION_TEXT="$VERSION_TEXT (Entwicklungsstand)"
COMMIT="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || true)"
{
	echo "DsRunner Stream Host $VERSION_TEXT"
	[ -z "$COMMIT" ] || echo "Commit: $COMMIT"
	echo "https://github.com/HolziDape/DsRunner"
} > "$DEST/VERSION.txt"

rm -f "$OUT"
(cd "$TMP" && zip -q -r -X "$OUT" "$NAME")
echo "$OUT:"
unzip -l "$OUT" | sed -n '4,$p' | sed '$d' | sed '$d' | awk '{print "  " $4}'
