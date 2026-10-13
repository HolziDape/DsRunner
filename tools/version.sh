#!/bin/sh
# Version helper for releases, see docs/RELEASE.md.
#
#   tools/version.sh                print the version from VERSION
#   tools/version.sh set X.Y.Z      set the version: VERSION, DsRunner Art (__version__) and
#                                   CHANGELOG.md ("## Unveröffentlicht" becomes the X.Y.Z
#                                   section with today's date, a new empty one goes on top)
#   tools/version.sh check [vX.Y.Z] check that VERSION, DsRunner Art and CHANGELOG.md agree,
#                                   and with a tag also that the tag is vX.Y.Z (release workflow)
#   tools/version.sh notes [X.Y.Z]  print the CHANGELOG.md section of the version (release notes)
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION_FILE="$ROOT/VERSION"
ART_INIT="$ROOT/tools/dsrunner-art/dsrunnerart/__init__.py"
CHANGELOG="$ROOT/CHANGELOG.md"
UNRELEASED="## Unveröffentlicht"
REPO_URL="https://github.com/HolziDape/DsRunner"

fail() {
	if [ "$GITHUB_ACTIONS" = "true" ]; then
		echo "::error::$1"
	else
		echo "Fehler: $1" >&2
	fi
	exit 1
}

read_version() {
	[ -f "$VERSION_FILE" ] || fail "VERSION fehlt"
	tr -d ' \t\r\n' < "$VERSION_FILE"
}

# is_version <text>: X.Y.Z with numbers only
is_version() {
	echo "$1" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'
}

art_version() {
	sed -n 's/^__version__ = "\(.*\)"$/\1/p' "$ART_INIT"
}

# has_section <version>: CHANGELOG.md has a "## [X.Y.Z]" heading
has_section() {
	grep -Fq "## [$1]" "$CHANGELOG"
}

# notes <version>: the lines between "## [X.Y.Z]" and the next "## " heading,
# without blank lines at the start and the end; links into docs/ point to the tag on
# GitHub, because relative links don't work on the release page
notes() {
	section "$1" | sed "s#](docs/#]($REPO_URL/blob/v$1/docs/#g"
}

section() {
	awk -v head="## [$1]" '
		index($0, head) == 1 { found = 1; next }
		found && /^## / { exit }
		found {
			if ($0 ~ /^[ \t\r]*$/) { blank++; next }
			if (printed) { while (blank-- > 0) print "" }
			blank = 0; printed = 1; print
		}
		END { if (!found) exit 1 }
	' "$CHANGELOG"
}

cmd="${1:-}"
case "$cmd" in
"")
	read_version
	echo
	;;
set)
	new="$2"
	is_version "$new" || fail "Version im Format X.Y.Z angeben, ohne v (z. B. 0.2.0)"
	printf '%s\n' "$new" > "$VERSION_FILE"
	sed -i "s/^__version__ = \".*\"$/__version__ = \"$new\"/" "$ART_INIT"
	if has_section "$new"; then
		echo "CHANGELOG.md hat schon einen Abschnitt für $new, unverändert."
	elif grep -Fxq "$UNRELEASED" "$CHANGELOG"; then
		heading="## [$new]($REPO_URL/releases/tag/v$new) - $(date +%Y-%m-%d)"
		tmp="$(mktemp)"
		awk -v unreleased="$UNRELEASED" -v heading="$heading" '
			$0 == unreleased && !done { print unreleased; print ""; print heading; done = 1; next }
			{ print }
		' "$CHANGELOG" > "$tmp"
		mv "$tmp" "$CHANGELOG"
		echo "CHANGELOG.md: \"Unveröffentlicht\" ist jetzt $new, bitte die Einträge prüfen."
	else
		fail "CHANGELOG.md hat weder \"$UNRELEASED\" noch einen Abschnitt für $new"
	fi
	echo "Version $new gesetzt (VERSION, DsRunner Art)."
	;;
check)
	version="$(read_version)"
	is_version "$version" || fail "VERSION enthält \"$version\", erwartet X.Y.Z (z. B. 0.2.0)"
	art="$(art_version)"
	[ "$art" = "$version" ] || fail "DsRunner Art hat Version \"$art\" (tools/dsrunner-art/dsrunnerart/__init__.py), VERSION sagt $version. Mit tools/version.sh set $version angleichen."
	has_section "$version" || fail "CHANGELOG.md hat keinen Abschnitt \"## [$version]\". Mit tools/version.sh set $version anlegen."
	if [ -n "$2" ]; then
		[ "$2" = "v$version" ] || fail "Tag \"$2\" passt nicht zu VERSION ($version). Der Tag muss v$version heißen, oder VERSION muss vor dem Release angepasst werden (docs/RELEASE.md)."
	fi
	echo "Version $version: VERSION, DsRunner Art und CHANGELOG.md passen${2:+ zum Tag $2}."
	;;
notes)
	version="${2:-$(read_version)}"
	has_section "$version" || fail "CHANGELOG.md hat keinen Abschnitt \"## [$version]\""
	notes "$version"
	;;
*)
	echo "Aufruf: tools/version.sh [set X.Y.Z | check [vX.Y.Z] | notes [X.Y.Z]]" >&2
	exit 2
	;;
esac
