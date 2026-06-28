#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$(uname -s)" in
    Darwin)
        default_preset="macos-local"
        ;;
    *)
        default_preset="default"
        ;;
esac

preset="${1:-$default_preset}"
if [[ $# -gt 0 ]]; then
    shift
fi

for path_prefix in "$@"; do
    export PATH="$path_prefix:$PATH"
done

if ! cmake --list-presets 2>/dev/null | grep -qF "\"$preset\""; then
    printf "Preset '%s' not found, falling back to 'default'\n" "$preset" >&2
    preset="default"
fi

cmake --preset "$preset"
cmake --build --preset "$preset"

mac_executable="$project_root/build/$preset/RenderLaz.app/Contents/MacOS/RenderLaz"
unix_executable="$project_root/build/$preset/RenderLaz"

if [[ -x "$mac_executable" ]]; then
    "$mac_executable"
elif [[ -x "$unix_executable" ]]; then
    "$unix_executable"
else
    printf 'RenderLaz executable was not found for preset %s.\n' "$preset" >&2
    exit 1
fi
