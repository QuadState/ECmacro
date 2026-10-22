#!/usr/bin/env bash
# Copyright (c) 2026 QuadState
# SPDX-License-Identifier: GPL-2.0-or-later
# Fetch the pinned QMK release and install the QMK CLI into a local virtualenv.
# Run once; build.sh does the rest. Needs git, make, Python 3.11 or later with
# venv, and arm-none-eabi-gcc with newlib (on Debian and Ubuntu, the packages
# git, make, python3-venv, gcc-arm-none-eabi and libnewlib-arm-none-eabi).
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
deps=$project/.build-deps
qmk=$deps/qmk_firmware
qmk_tag=0.34.7
qmk_commit=180f394b489c238cb231c2bf8b080262155e03c0

for command in git python3 make arm-none-eabi-gcc; do
    command -v "$command" >/dev/null || { echo "Missing build dependency: $command" >&2; exit 1; }
done
# The pinned packages in requirements-build.txt need Python 3.11 or later.
python3 -c 'import sys; sys.exit(sys.version_info < (3, 11))' || { echo "Python 3.11 or later is needed" >&2; exit 1; }
[[ ! -e "$qmk" ]] || { echo "$qmk already exists; remove it to set up again" >&2; exit 1; }
mkdir -p -- "$deps"
# The virtualenv first, so that a missing venv module or package stops the
# setup before the large QMK download.
python3 -m venv "$deps/venv"
"$deps/venv/bin/python" -m pip install -r "$project/requirements-build.txt"
# QMK goes under a temporary name until it is complete, so that an
# interrupted download does not stop the next run.
rm -rf -- "$qmk.partial"
git clone --depth 1 --branch "$qmk_tag" https://github.com/qmk/qmk_firmware.git "$qmk.partial"
[[ $(git -C "$qmk.partial" rev-parse HEAD) == "$qmk_commit" ]] || { echo "Unexpected QMK $qmk_tag commit" >&2; exit 1; }
git -C "$qmk.partial" submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/pico-sdk lib/printf lib/lufa
mv -- "$qmk.partial" "$qmk"
echo "Ready: scripts/build.sh"
