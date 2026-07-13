#!/usr/bin/env bash
#
# Shared build steps for scripts/build-linux.sh and
# scripts/interactive-build-linux.sh. Not meant to be run directly - source it.
#
# Expects REPO_ROOT to already be set by the caller.

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

step() { printf '\n\033[1;33m>> %s\033[0m\n' "$1"; }
ok()   { printf '\033[1;32m[OK]\033[0m %s\n' "$1"; }
warn() { printf '\033[1;33m[WARN]\033[0m %s\n' "$1"; }
err()  { printf '\033[1;31m[X]\033[0m %s\n' "$1" >&2; }

install_system_deps() {
    if command -v pacman >/dev/null 2>&1; then
        step "Checking system dependencies (pacman)"
        local missing=()
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
}

detect_mold() {
    if command -v mold >/dev/null 2>&1; then
        MOLD_AVAILABLE=1
        ok "mold found, will use it for linking."
    else
        MOLD_AVAILABLE=0
        warn "mold not found - final linking will use the default linker and can need 8+ GB RAM/swap. Install 'mold' to avoid this."
    fi
}

ensure_submodules() {
    step "Checking submodules"
    if git -C "$REPO_ROOT" submodule status | grep -q '^-'; then
        git -C "$REPO_ROOT" submodule update --init --recursive
    else
        ok "Submodules already initialized."
    fi
}

# On Linux, tde2e is always resolved via find_package(tde2e) regardless of
# DESKTOP_APP_USE_PACKAGED - there is no bundled-source fallback for it.
build_and_install_tde2e() {
    local deps_prefix="$1"
    local tde2e_cmake_dir="$deps_prefix/lib/cmake/tde2e"

    if [ -f "$tde2e_cmake_dir/tde2eConfig.cmake" ]; then
        ok "tde2e already installed at $deps_prefix"
        return
    fi

    step "Building tde2e (E2E calls library from tdlib/td)"

    local td_commit
    td_commit="$(awk '/^  tde2e:/{f=1; next} f && /source-commit:/{print $2; exit}' "$REPO_ROOT/snap/snapcraft.yaml")"
    td_commit="${td_commit:-51743dfd01dff6179e2d8f7095729caa4e2222e9}"
    echo "Using tdlib/td commit: $td_commit"

    local td_src_dir="$REPO_ROOT/out/_deps/td"
    if [ ! -d "$td_src_dir/.git" ]; then
        mkdir -p "$(dirname "$td_src_dir")"
        git clone https://github.com/tdlib/td.git "$td_src_dir"
    fi
    git -C "$td_src_dir" checkout "$td_commit"

    cmake -S "$td_src_dir" -B "$td_src_dir/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$deps_prefix" \
        -DTD_E2E_ONLY=ON
    cmake --build "$td_src_dir/build" -j"$(nproc)"
    cmake --install "$td_src_dir/build"
}

# Sets API_ID / API_HASH. Falls back to the same test credentials
# scripts/build-windows.ps1 uses. Copy api_credentials.local.sh.example to
# override with your own.
resolve_api_credentials() {
    API_ID="17349"
    API_HASH="344583e45741c457fe1862106095a5eb"
    local f
    for f in "$REPO_ROOT/api_credentials.local.sh" "$(dirname "$REPO_ROOT")/api_credentials.local.sh"; do
        if [ -f "$f" ]; then
            # shellcheck disable=SC1090
            source "$f"
            API_ID="$TDESKTOP_API_ID"
            API_HASH="$TDESKTOP_API_HASH"
            ok "Using API credentials from $f"
            return
        fi
    done
}

configure_project() {
    local configuration="$1"
    local deps_prefix="$2"

    step "Configuring CMake ($configuration)"
    resolve_api_credentials
    detect_mold

    local cmake_args=(
        -B "$REPO_ROOT/out" -G Ninja
        -DCMAKE_BUILD_TYPE="$configuration"
        -DCMAKE_PREFIX_PATH="$deps_prefix"
        -DDESKTOP_APP_USE_PACKAGED=ON
        -DTDESKTOP_API_ID="$API_ID"
        -DTDESKTOP_API_HASH="$API_HASH"
    )
    if [ "${MOLD_AVAILABLE:-0}" -eq 1 ]; then
        cmake_args+=(
            -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold
            -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=mold
            -DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=mold
        )
    fi
    cmake "${cmake_args[@]}"
}

build_project() {
    step "Building"
    cmake --build "$REPO_ROOT/out" -j"$(nproc)"
    ok "Build finished: $REPO_ROOT/out/OwpenGram"
}
