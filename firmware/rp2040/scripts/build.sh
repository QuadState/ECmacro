#!/usr/bin/env bash
# Copyright (c) 2026 QuadState
# SPDX-License-Identifier: GPL-2.0-or-later
# Build the ECmacro 4H firmware into build/ecmacro_4h_<keymap>.uf2.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
keymap=${1:-via}
keyboard=quadstate/ecmacro4h
source=$project/qmk/keyboards/$keyboard
[[ $# -le 1 && $keymap =~ ^[a-z0-9_]+$ && -d "$source/keymaps/$keymap" ]] || { echo "Usage: $0 [KEYMAP]  (via or default)" >&2; exit 2; }
deps=$project/.build-deps
qmk=$deps/qmk_firmware
[[ -x "$deps/venv/bin/qmk" && -f "$qmk/Makefile" ]] || { echo "Run scripts/setup.sh first" >&2; exit 1; }
export PATH="$deps/venv/bin:$PATH"

# Stage the keyboard (with its PIO sensing code) into QMK.
target=$qmk/keyboards/$keyboard
rm -rf -- "$target"
mkdir -p -- "$target"
cp -a -- "$source/." "$target/"

# SKIP_VERSION keeps the QMK build date fixed. VIA keeps its stored key
# assignments only while the build date stays the same, so a rebuild on another
# day would otherwise clear them.
make -C "$qmk" clean
make -C "$qmk" "$keyboard:$keymap" SKIP_VERSION=yes
mkdir -p -- "$project/build"
cp -- "$qmk/${keyboard//\//_}_$keymap.uf2" "$project/build/ecmacro_4h_$keymap.uf2"
echo "Built build/ecmacro_4h_$keymap.uf2"
