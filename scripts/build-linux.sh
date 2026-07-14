#!/usr/bin/env bash
#
# Linux build for OwpenGram Desktop.
#
# Two modes:
#   (default)  Native build against system libraries (DESKTOP_APP_USE_PACKAGED),
#              the same mode Arch's own `telegram-desktop` package uses. Fast,
#              but the binary is tied to THIS machine's exact library versions
#              (Qt6, glibc, ...) - fine for local testing, not for distributing.
#   --docker   Portable build via the official Rocky Linux 8 Docker image
#              (Telegram/build/docker/centos_env). Slower (builds Qt6/WebRTC
#              from scratch, one-time image build + full compile), but the
#              resulting binary runs on essentially any Linux from the last
#              ~6 years (Ubuntu 18.04+, Debian 10+, ...). Output goes to
#              out-docker/, separate from out/ used by the native build.
#
# For a menu-driven wizard (server patching, saved settings), see
# scripts/interactive-build-linux.sh instead.
#
# Usage:
#   scripts/build-linux.sh                  # native, Release (default)
#   scripts/build-linux.sh --debug          # native, Debug
#   scripts/build-linux.sh --docker         # portable, Release
#   scripts/build-linux.sh --docker --debug # portable, Debug
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

# shellcheck source=lib/build-linux-steps.sh
source "$SCRIPT_DIR/lib/build-linux-steps.sh"

CONFIGURATION="Release"
USE_DOCKER=0

for arg in "$@"; do
    case "$arg" in
        --debug) CONFIGURATION="Debug" ;;
        --release) CONFIGURATION="Release" ;;
        --docker) USE_DOCKER=1 ;;
        -h|--help)
            cat <<EOF
Usage: $(basename "$0") [--docker] [--debug|--release]

  --release   Optimized build (default).
  --debug     Debug build (assertions, no optimization; much slower to run).
  --docker    Build via the portable Rocky Linux 8 Docker image instead of
              against this machine's system libraries. Output: out-docker/.

Env overrides:
  OWPENGRAM_DEPS_PREFIX   Install prefix for tde2e, native mode only (default: \$HOME/.local)
EOF
            exit 0
            ;;
        *)
            echo "[ERROR] Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

step "Configuration: $CONFIGURATION $([ "$USE_DOCKER" -eq 1 ] && echo '(docker, portable)' || echo '(native)')"
echo "Repo root: $REPO_ROOT"

if [ "$USE_DOCKER" -eq 1 ]; then
    build_via_docker "$CONFIGURATION"
else
    DEPS_PREFIX="${OWPENGRAM_DEPS_PREFIX:-$HOME/.local}"
    echo "Deps prefix: $DEPS_PREFIX"
    install_system_deps
    ensure_submodules
    build_and_install_tde2e "$DEPS_PREFIX"
    configure_project "$CONFIGURATION" "$DEPS_PREFIX"
    build_project
fi
