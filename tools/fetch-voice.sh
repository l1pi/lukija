#!/bin/sh
# Downloads Piper voices from huggingface.co/rhasspy/piper-voices into the
# directory where Lukija looks for them.
#
#   tools/fetch-voice.sh [voice-id...]   (default: fi_FI-harri-medium en_US-lessac-medium)
#
# Voice ids are listed at https://huggingface.co/rhasspy/piper-voices
set -eu

dest="${XDG_DATA_HOME:-$HOME/.local/share}/io.github.l1pi/harbour-lukija/voices"
base="https://huggingface.co/rhasspy/piper-voices/resolve/main"

[ $# -eq 0 ] && set -- fi_FI-harri-medium en_US-lessac-medium
mkdir -p "$dest"

for id in "$@"; do
    # <lang>_<REGION>-<name>-<quality>, e.g. en_GB-jenny_dioco-medium
    code=${id%%-*}
    rest=${id#*-}
    quality=${rest##*-}
    name=${rest%-*}
    family=${code%%_*}
    url="$base/$family/$code/$name/$quality"

    if [ -f "$dest/$id.onnx" ] && [ -f "$dest/$id.onnx.json" ]; then
        echo "$id: already installed"
        continue
    fi
    echo "$id: downloading"
    for file in "$id.onnx.json" "$id.onnx"; do
        curl -fL --progress-bar -o "$dest/$file.part" "$url/$file" \
            || { rm -f "$dest/$file.part"; echo "$id: download failed ($url)" >&2; exit 1; }
        mv "$dest/$file.part" "$dest/$file"
    done
done
echo "Voices are in $dest"
