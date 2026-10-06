#!/bin/sh
set -eu
lane=$1
mkdir /work/source
cp -a /input/. /work/source/
cd /work/source
export HOME=/work/build/home LC_ALL=C.UTF-8 TZ=UTC
mkdir -p "$HOME"
if [ "$lane" = licensing ]; then
  reuse --version
  reuse lint
  exit
fi
python3 --version
cmake --version
ninja --version
c++ --version
clang-format --version
clang-tidy --version
pkg-config --modversion Qt6Core Qt6Quick
python3 scripts/ci/test_launcher.py
python3 scripts/ci/test_tooling_tidy.py
mkdir -p /work/providers
fetch_provider() {
  name=$1
  revision=$2
  git init -q "/work/providers/$name"
  git -C "/work/providers/$name" fetch --depth 1 "https://github.com/lebedenko/$name.git" "$revision"
  git -C "/work/providers/$name" checkout --detach FETCH_HEAD
  [ "$(git -C "/work/providers/$name" rev-parse HEAD)" = "$revision" ]
}
fetch_provider holonight-config d6a392b41991f70a004d58f7694c7b6115cb7280
fetch_provider holonight-qt 98803bca05e16ae0d0784a6cb43b0ace561385de
fetch_provider holonight-shell e490ff73f3da4b3a671aaf0496c8dbdc94a53ff8
fetch_provider holonight-system-services 398804a7cce5a57f9f6870c4e7ec99e9b1f3ddaa
prefix=/work/providers/prefix
cmake -S /work/providers/holonight-config -B /work/providers/config-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build /work/providers/config-build --parallel 2
cmake --install /work/providers/config-build --prefix "$prefix"
cmake -S /work/providers/holonight-system-services -B /work/providers/services-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build /work/providers/services-build --parallel 2
cmake --install /work/providers/services-build --prefix "$prefix"
cmake -S /work/providers/holonight-qt -B /work/providers/qt-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$prefix" \
  -DBUILD_TESTS=OFF -DBUILD_DEMO=OFF -DBUILD_CONTROLS_GALLERY=OFF -DBUILD_QT5_PROBES=OFF
cmake --build /work/providers/qt-build --parallel 2
cmake --install /work/providers/qt-build --prefix "$prefix"
cmake -S /work/providers/holonight-shell/libs/holonight-shell-config -B /work/providers/shell-config-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$prefix"
cmake --build /work/providers/shell-config-build --parallel 2
cmake --install /work/providers/shell-config-build --prefix "$prefix"
build=build/verification
stage=/work/settings-install
trap 'status=$?; if [ -d "$build/Testing" ]; then cp -a "$build/Testing" /output/; fi; exit "$status"' 0
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPython3_EXECUTABLE=/usr/bin/python3 \
  -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_INSTALL_PREFIX="$stage"
cmake --build "$build" --parallel 2
export LD_LIBRARY_PATH="$prefix/lib"
if [ "$lane" = static-checks ]; then
  cmake --build "$build" --target format-check
  cmake --build "$build" --target tidy
  consumer=build/tidy-package-consumer
  cmake -S tests/package-consumer -B "$consumer" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_PREFIX_PATH="$prefix"
  cmake --build "$consumer" --parallel 2
  python3 - "$consumer" <<'PY_DB'
from pathlib import Path
import json
import sys
build = Path(sys.argv[1])
entries = json.loads((build / 'compile_commands.json').read_text())
for entry in entries:
    entry['command'] = entry['command'].replace('-mno-direct-extern-access', '').replace('-Wno-template-id-cdtor', '')
(build / 'clang').mkdir()
(build / 'clang/compile_commands.json').write_text(json.dumps(entries))
PY_DB
  run-clang-tidy -quiet -j 2 -p "$consumer/clang" \
    -header-filter "^$PWD/(apps|libs|tests)/.*\\.(h|hpp)$" "$PWD/tests/package-consumer/main.cpp"
else
  export HOLONIGHT_CI_STARTUP_LOG_DIR=/output/build-startup
  dbus-run-session -- ctest --test-dir "$build" --output-on-failure --no-tests=error
  python3 scripts/check-qml-import-policy.py
  python3 tests/test_qml_import_policy.py
  bash scripts/check-qmltypes.sh "$build"
  cmake --build "$build" --target qml-lint
  mkdir -p "$stage"
  cp -a "$prefix/." "$stage/"
  cmake --install "$build"
  test -x "$stage/bin/holonight-settings"
  service="$stage/share/dbus-1/services/org.holonight.Settings.service"
  grep -Fx "Exec=$stage/bin/holonight-settings" "$service"
  test -x "$(sed -n 's/^Exec=//p' "$service")"
  for mode in default environment command-line configuration; do
    python3 tests/check_settings_startup.py "$stage/bin/holonight-settings" "$stage/lib/qt6/qml" "$mode" \
      --forbid-qml-root "$prefix/lib/qt6/qml" --evidence-dir /output/installed-startup
  done
fi
