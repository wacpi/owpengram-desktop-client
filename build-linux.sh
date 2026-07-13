#!/usr/bin/env bash
#
# Double-clickable / terminal entry point for the Linux build, mirroring
# build-windows.bat: asks Release or Debug (unless passed explicitly) and
# builds. No menus, no server patching - see docs/building-linux.md for that.
#
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CONFIGURATION=""
for arg in "$@"; do
    case "$arg" in
        --debug) CONFIGURATION="Debug" ;;
        --release) CONFIGURATION="Release" ;;
    esac
done

if [ -z "$CONFIGURATION" ]; then
    echo "OwpenGram Desktop build"
    echo "Repo: $ROOT"
    echo ""
    while [ -z "$CONFIGURATION" ]; do
        read -r -p "Build type: [R]elease or [D]ebug [R]: " answer
        case "$(echo "$answer" | tr '[:upper:]' '[:lower:]')" in
            ""|r|release) CONFIGURATION="Release" ;;
            d|debug) CONFIGURATION="Debug" ;;
            *) echo "  Please enter R or D." ;;
        esac
    done
fi

exec "$ROOT/scripts/build-linux.sh" "--$(echo "$CONFIGURATION" | tr '[:upper:]' '[:lower:]')"
