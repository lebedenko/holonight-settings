#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
build=build/dev
prefix="$PWD/build/deps/prefix"
stage="$PWD/build/installed"
mkdir -p /work/output
    task configure PRESET=release
    cmake -S . -B build/release -DCMAKE_INSTALL_PREFIX="$stage"
    cmake --build build/release --parallel "$JOBS"
    mkdir -p "$stage"
    cp -a "$prefix/." "$stage/"
    cmake --install build/release --prefix "$stage"
    test -x "$stage/bin/holonight-settings"
    if find "$stage" -type f -name 'test_*' | grep .; then exit 1; fi
    service="$stage/share/dbus-1/services/org.holonight.Settings.service"
    grep -Fx "Exec=$stage/bin/holonight-settings" "$service"
    test -x "$(sed -n 's/^Exec=//p' "$service")"
    for mode in default environment command-line configuration; do
      python3 tests/check_settings_startup.py "$stage/bin/holonight-settings" "$stage/lib/qt6/qml" "$mode" \
        --forbid-qml-root "$prefix/lib/qt6/qml" --evidence-dir /work/output/installed-startup
    done
    cp -a "$stage" /work/output/stage
