#!/bin/sh

(set -u
cd "$(dirname "$0")" || exit 1
. ./version || { printf "Version formatted wrong" >&2; exit 1; }
bin="BullshitCore-$MINECRAFT_VERSION-$VERSION" || { printf "Version formatted wrong" >&2; exit 1; }
if [ "$(stat -c %y "$bin" config.h | sort -nr | head -1)" = "$(stat -c %y config.h)" ]; then
	./build.sh
fi)
