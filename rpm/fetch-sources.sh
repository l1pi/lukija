#!/bin/sh
# Downloads the third-party sources the spec lists as Source1, Source2, ...
# into a directory, under the names the spec expects. For builds without
# network access: give the directory to rpmbuild as _sourcedir.
#
#   rpm/fetch-sources.sh <directory>
#
# The build checks each archive against the checksum in 3rdparty/CMakeLists.txt.
set -eu

[ $# -eq 1 ] || { echo "usage: $0 <directory>" >&2; exit 2; }
dest=$1
spec="$(dirname "$0")/harbour-lukija.spec"
mkdir -p "$dest"

# "Source1: <url>#/<name>": the name after "#/", else the URL's last part.
sed -n 's/^Source[1-9][0-9]*:[[:space:]]*//p' "$spec" | while read -r source; do
    url=${source%%#/*}
    name=${source##*/}
    if [ -f "$dest/$name" ]; then
        echo "$name: already there"
        continue
    fi
    echo "$name: downloading"
    curl -fL --progress-bar -o "$dest/$name.part" "$url"
    mv "$dest/$name.part" "$dest/$name"
done
