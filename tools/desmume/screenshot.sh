#!/bin/sh
# Starts DsRunner in DeSmuME without a window (Xvfb), presses keys and saves a screenshot
# of both screens. For quick UI checks on Linux; games can't be started this way.
#
# Needs: desmume-cli, xvfb, xdotool, imagemagick (apt install desmume xvfb xdotool imagemagick)
#        and docker (for dlditool from the BlocksDS image).
#
# Usage: tools/desmume/screenshot.sh <sd folder> <out.png> [seconds before keys] [keys...]
#   keys: xdotool key names with the DeSmuME defaults x=A z=B s=X a=Y q=L w=R Return=Start,
#         arrow keys = Up/Down/Left/Right, "wait" = one second pause.
# Example: tools/desmume/screenshot.sh /tmp/sd shot.png 10 Down x wait wait
set -e
SD="$(cd "$1" && pwd)"
OUT="$2"
DELAY="${3:-10}"
shift 3 2>/dev/null || shift $#
WORK="$(mktemp -d)"
BLOCKSDS_IMAGE="${BLOCKSDS_IMAGE:-skylyrac/blocksds:slim-v1.16.0}"

# DeSmuME's R4 slot-1 emulation needs the R4 DLDI driver patched in
cp DsRunner.nds "$WORK/test.nds"
docker run --rm -v "$WORK":/emu "$BLOCKSDS_IMAGE" sh -c \
    '$BLOCKSDS/tools/dlditool/dlditool $BLOCKSDS/sys/dldi_r4/r4tf.dldi /emu/test.nds' >/dev/null

Xvfb :99 -screen 0 800x600x24 >/dev/null 2>&1 &
XPID=$!
sleep 1
DISPLAY=:99 timeout $((DELAY + 60)) desmume-cli --disable-sound --slot1=R4 --slot1-fat-dir="$SD" \
    "$WORK/test.nds" >"$WORK/desmume.log" 2>&1 &
EPID=$!
sleep "$DELAY"
WINDOW=$(DISPLAY=:99 xdotool search --name "Desmume" | head -1)
for KEY in "$@"; do
    if [ "$KEY" = "wait" ]; then sleep 1; continue; fi
    DISPLAY=:99 xdotool keydown --window "$WINDOW" "$KEY"; sleep 0.12
    DISPLAY=:99 xdotool keyup --window "$WINDOW" "$KEY"; sleep 0.35
done
DISPLAY=:99 import -window root "$WORK/screen.png"
kill $EPID $XPID 2>/dev/null || true
# both screens, scaled 2x
convert "$WORK/screen.png" -trim +repage -scale 200% "$OUT"
rm -rf "$WORK"
