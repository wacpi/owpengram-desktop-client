#!/usr/bin/env bash
#
# Native Linux build for OwpenGram Desktop - no Docker, no snap.
#
# Builds against system libraries (DESKTOP_APP_USE_PACKAGED=ON), the same
# mode Arch's own `telegram-desktop` package is built with. Handles the
# parts that mode doesn't cover out of the box: installing missing system
# packages, building/installing tde2e (required via find_package on Linux,
# no bundled fallback), and picking mold as the linker when available (the
# default bfd `ld` can need 8+ GB RAM to link this binary).
#
# For a menu-driven wizard (server patching, saved settings), see
# scripts/interactive-build-linux.sh instead.
#
# Usage:
#   scripts/build-linux.sh              # Release (default)
#   scripts/build-linux.sh --debug      # Debug
#   scripts/build-linux.sh --release
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

# shellcheck source=lib/build-linux-steps.sh
source "$SCRIPT_DIR/lib/build-linux-steps.sh"

CONFIGURATION="Release"

for arg in "$@"; do
    case "$arg" in
        --debug) CONFIGURATION="Debug" ;;
        --release) CONFIGURATION="Release" ;;
        -h|--help)
            cat <<EOF
Usage: $(basename "$0") [--debug|--release]

Configures, prepares and builds OwpenGram Desktop natively on Linux
(no Docker, no snap), using system libraries (DESKTOP_APP_USE_PACKAGED).

  --release   Optimized build (default).
  --debug     Debug build (assertions, no optimization; much slower to run).

Env overrides:
  OWPENGRAM_DEPS_PREFIX   Install prefix for tde2e (default: \$HOME/.local)
EOF
            exit 0
            ;;
        *)
            echo "[ERROR] Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

DEPS_PREFIX="${OWPENGRAM_DEPS_PREFIX:-$HOME/.local}"

step "Configuration: $CONFIGURATION"
echo "Deps prefix:   $DEPS_PREFIX"
echo "Repo root:     $REPO_ROOT"

install_system_deps
ensure_submodules
build_and_install_tde2e "$DEPS_PREFIX"
configure_project "$CONFIGURATION" "$DEPS_PREFIX"
build_project
