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
# Usage:
#   scripts/build-linux.sh              # Release (default)
#   scripts/build-linux.sh --debug      # Debug
#   scripts/build-linux.sh --release
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

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

step() { printf '\n\033[1;33m>> %s\033[0m\n' "$1"; }
ok()   { printf '\033[1;32m[OK]\033[0m %s\n' "$1"; }
warn() { printf '\033[1;33m[WARN]\033[0m %s\n' "$1"; }

DEPS_PREFIX="${OWPENGRAM_DEPS_PREFIX:-$HOME/.local}"

step "Configuration: $CONFIGURATION"
echo "Deps prefix:   $DEPS_PREFIX"
echo "Repo root:     $REPO_ROOT"

# --- 1. system dependencies -------------------------------------------------
# Union of: Arch's own telegram-desktop runtime deps, plus what this fork
# additionally needs at build time (Qt shader tools, tde2e build tools, mold).
PACMAN_PACKAGES=(
    base-devel cmake ninja python git gperf mold
    qt6-base qt6-declarative qt6-svg qt6-wayland qt6-imageformats qt6-shadertools
    ffmpeg openal boost protobuf hunspell minizip lz4 xxhash range-v3
    tl-expected microsoft-gsl cmark-gfm kcoreaddons kimageformats fcitx5-qt
    libtg_owt abseil-cpp ada libavif libdispatch libheif libjpeg-turbo libjxl
    libpipewire libxcb libxcomposite libxdamage libxext libxfixes libxkbcommon
    libxrandr libxtst openh264 openssl pipewire rnnoise zlib glib2
    hicolor-icon-theme libx11
)

if command -v pacman >/dev/null 2>&1; then
    step "Checking system dependencies (pacman)"
    missing=()
    for pkg in "${PACMAN_PACKAGES[@]}"; do
        pacman -Qq "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
    done
    if [ "${#missing[@]}" -gt 0 ]; then
        echo "Installing: ${missing[*]}"
        sudo pacman -S --needed "${missing[@]}"
    else
        ok "All required packages already installed."
    fi
else
    warn "pacman not found - install the equivalent of: ${PACMAN_PACKAGES[*]}"
fi

MOLD_AVAILABLE=0
if command -v mold >/dev/null 2>&1; then
    MOLD_AVAILABLE=1
    ok "mold found, will use it for linking."
else
    warn "mold not found - final linking will use the default linker and can need 8+ GB RAM/swap. Install 'mold' to avoid this."
fi

# --- 2. submodules -----------------------------------------------------------
step "Checking submodules"
if git submodule status | grep -q '^-'; then
    git submodule update --init --recursive
else
    ok "Submodules already initialized."
fi

# --- 3. tde2e (td/e2e) --------------------------------------------------------
# On Linux this is always looked up via find_package(tde2e), regardless of
# DESKTOP_APP_USE_PACKAGED - there is no bundled-source fallback for it.
TDE2E_CMAKE_DIR="$DEPS_PREFIX/lib/cmake/tde2e"
if [ ! -f "$TDE2E_CMAKE_DIR/tde2eConfig.cmake" ]; then
    step "Building tde2e (E2E calls library from tdlib/td)"

    TD_COMMIT="$(awk '/^  tde2e:/{f=1; next} f && /source-commit:/{print $2; exit}' snap/snapcraft.yaml)"
    TD_COMMIT="${TD_COMMIT:-51743dfd01dff6179e2d8f7095729caa4e2222e9}"
    echo "Using tdlib/td commit: $TD_COMMIT"

    TD_SRC_DIR="$REPO_ROOT/out/_deps/td"
    if [ ! -d "$TD_SRC_DIR/.git" ]; then
        mkdir -p "$(dirname "$TD_SRC_DIR")"
        git clone https://github.com/tdlib/td.git "$TD_SRC_DIR"
    fi
    git -C "$TD_SRC_DIR" checkout "$TD_COMMIT"

    cmake -S "$TD_SRC_DIR" -B "$TD_SRC_DIR/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" \
        -DTD_E2E_ONLY=ON
    cmake --build "$TD_SRC_DIR/build" -j"$(nproc)"
    cmake --install "$TD_SRC_DIR/build"
else
    ok "tde2e already installed at $DEPS_PREFIX"
fi

# --- 4. API credentials -------------------------------------------------------
# Falls back to the same test credentials scripts/build-windows.ps1 uses.
# Copy api_credentials.local.sh.example to override with your own.
API_ID="17349"
API_HASH="344583e45741c457fe1862106095a5eb"
for f in "$REPO_ROOT/api_credentials.local.sh" "$(dirname "$REPO_ROOT")/api_credentials.local.sh"; do
    if [ -f "$f" ]; then
        # shellcheck disable=SC1090
        source "$f"
        API_ID="$TDESKTOP_API_ID"
        API_HASH="$TDESKTOP_API_HASH"
        ok "Using API credentials from $f"
        break
    fi
done

# --- 5. configure --------------------------------------------------------------
step "Configuring CMake ($CONFIGURATION)"
CMAKE_ARGS=(
    -B out -G Ninja
    -DCMAKE_BUILD_TYPE="$CONFIGURATION"
    -DCMAKE_PREFIX_PATH="$DEPS_PREFIX"
    -DDESKTOP_APP_USE_PACKAGED=ON
    -DTDESKTOP_API_ID="$API_ID"
    -DTDESKTOP_API_HASH="$API_HASH"
)
if [ "$MOLD_AVAILABLE" -eq 1 ]; then
    CMAKE_ARGS+=(
        -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold
        -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=mold
        -DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=mold
    )
fi
cmake "${CMAKE_ARGS[@]}"

# --- 6. build ------------------------------------------------------------------
step "Building ($CONFIGURATION)"
cmake --build out -j"$(nproc)"

ok "Build finished: $REPO_ROOT/out/OwpenGram"
