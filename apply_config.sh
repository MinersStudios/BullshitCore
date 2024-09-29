#!/bin/sh

(set -u
cd "$(dirname "$0")" || exit 1
. ./version || { printf "Version formatted wrong\n" >&2; exit 1; }
bin="BullshitCore-$MINECRAFT_VERSION-$VERSION" ||
{ printf "Version formatted wrong\n" >&2; exit 1; }
modification_dates="$(stat -c %y config.h "$bin")"
if [ "$(printf "%s\n" "$modification_dates" | sort -nr | head -1)" = \
"$(printf "%s\n" "$modification_dates" | head -1)" ]
then ./build.sh
fi)
