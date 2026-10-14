#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
lane=$1
mkdir /work/settings
cp -a /input/. /work/settings/
cd /work/settings
export HOME=/work/build/home LC_ALL=C.UTF-8 TZ=UTC
export JOBS=${JOBS:-2} CMAKE_BUILD_PARALLEL_LEVEL=${JOBS:-2}
export XDG_RUNTIME_DIR=/work/runtime QT_FORCE_STDERR_LOGGING=1
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QSG_RHI_BACKEND=software
export QT_QPA_PLATFORMTHEME= QT_STYLE_OVERRIDE=
mkdir -p "$HOME"
mkdir -m 700 "$XDG_RUNTIME_DIR"
if [ "$lane" = licensing ]; then
  reuse --version
  reuse lint
  exit
fi
mkdir -p build/ci-provenance
pacman -Q > build/ci-provenance/packages.txt
c++ --version > build/ci-provenance/compiler.txt
cmake --version > build/ci-provenance/cmake.txt
{ python3 --version; task --version; ninja --version; clang-format --version; clang-tidy --version; qmake6 --version; wayland-scanner --version; } > build/ci-provenance/versions.txt 2>&1
capture() {
  status=$?
  cp -a build/ci-provenance /output/
  if [ -d build/test/uqc-launch-logs ]; then cp -a build/test/uqc-launch-logs /output/; fi
  if [ -d build/test/Testing ]; then cp -a build/test/Testing /output/; fi
  if [ -f build/test/compile_commands.json ]; then cp build/test/compile_commands.json /output/; fi
  if [ -d .cache/tooling ]; then cp -a .cache/tooling /output/tooling; fi
  exit "$status"
}
trap capture 0
if [ "$lane" = prepare ]; then
  export HOLONIGHT_CONFIG_SOURCE=/prepared/sources/holonight-config
  export HOLONIGHT_QT_SOURCE=/prepared/sources/holonight-qt
  export HOLONIGHT_SYSTEM_SERVICES_SOURCE=/prepared/sources/holonight-system-services
  if [ -d /prepared/deps ]; then cp -a /prepared/deps build/deps; fi
  export HOLONIGHT_SHELL_SOURCE=/prepared/sources/holonight-shell
  task deps PRESET=test
  mkdir -p /prepared/deps
  cp -a build/deps/. /prepared/deps/
  cp -a build/ci-provenance /prepared/environment
  exit
fi
cp -a /prepared/deps build/deps
cp /prepared/provider-revisions.json build/ci-provenance/
cp build/deps/provider-revisions.tsv build/ci-provenance/
export LD_LIBRARY_PATH="$PWD/build/deps/prefix/lib"
python3 scripts/ci/test_launcher.py
python3 scripts/ci/test_tooling_tidy.py
python3 scripts/ci/test_dependencies.py
prefix=$PWD/build/deps/prefix
build=build/test
stage=/work/settings-install
package_acceptance() {
  cmake -S tests/package-consumer -B build/package-consumer -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_PREFIX_PATH="$prefix"
  cmake --build build/package-consumer --parallel "$JOBS"
  build/package-consumer/acf005-package-consumer
}
case "$lane" in
  build-test)
    task build PRESET=test
    export HOLONIGHT_CI_STARTUP_LOG_DIR=/output/build-startup
    dbus-run-session -- ctest --test-dir "$build" --output-on-failure --no-tests=error
    cat "$build/Testing/Temporary/LastTest.log"
    package_acceptance
    ;;
  static-checks)
    task build PRESET=test
    task format-check
    package_acceptance
    python3 scripts/ci/tidy-package-consumer.py
    cmake --build "$build" --target qml-lint
    python3 scripts/check-qml-import-policy.py
    python3 tests/test_qml_import_policy.py
    bash scripts/check-qmltypes.sh "$build"
    ;;
  release)
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
        --forbid-qml-root "$prefix/lib/qt6/qml" --evidence-dir /output/installed-startup
    done
    cp -a "$stage" /output/stage
    ;;
  *) echo "Unknown lane: $lane" >&2; exit 2 ;;
esac
