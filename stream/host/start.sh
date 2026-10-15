#!/bin/sh
# Starts the DsRunner Stream host with the settings from ~/.config/dsstream.env
# (example: systemd/dsstream.env). Used by the systemd service and the desktop
# autostart; extra arguments are passed on, e.g. start.sh --headful
HERE="$(cd "$(dirname "$0")" && pwd)"
CONF="${DSSTREAM_CONF:-$HOME/.config/dsstream.env}"
if [ -f "$CONF" ]; then
	# shellcheck source=/dev/null
	. "$CONF"
fi
PY="${DSSTREAM_PYTHON:-$HOME/dsstream-venv/bin/python}"
cd "$HERE" || exit 1
# shellcheck disable=SC2086
exec "$PY" -m dsstream --url "${DSSTREAM_URL:-test}" --mode "${DSSTREAM_MODE:-256x192}" \
	--mapping "${DSSTREAM_MAPPING:-default.json}" $DSSTREAM_EXTRA "$@"
