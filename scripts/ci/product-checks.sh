#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
build=build/dev
prefix="$PWD/build/deps/prefix"
stage="$PWD/build/installed"
mkdir -p /work/output
package_acceptance() {
  cmake -S tests/package-consumer -B build/package-consumer -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_PREFIX_PATH="$prefix"
  cmake --build build/package-consumer --parallel "$JOBS"
  build/package-consumer/acf005-package-consumer
}
package_acceptance
python3 scripts/quality.py tidy --build build/package-consumer
