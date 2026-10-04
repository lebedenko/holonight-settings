# Local CI rehearsal

Baseline ae64854feb896d0c81bea4d553464824ef700776. Umbrella CI-015.

Use identical repository-owned scripts, immutable images and checksum-pinned
supplements locally and in the existing independent GitHub jobs. Snapshot tracked
edits and non-ignored new inputs with modes and symlinks; report new inputs.
Disposable source/build trees and host-owned logs, revision/dirty status, images,
versions and lane results live under ignored build/ci. Every required failure fails
Task. Host input is read-only; normal development builds are preserved.

Fresh Release providers and Debug BUILD_TESTS=ON consumers. Config
81b01d3ae8433f3a4b017db2feb588a1ee62b714 and System Services
4556dc9b22237823387110340347cd1301ed9245 remain unchanged. Qt is corrected from
50c59558bb3817f57a992dd72730dba141db1bc8 to published
8d11e3e91fea5ad0d20a34f2ed27e5e5f485124a: NavPanel.qml sets rendering, absent
from the former HnIcon.qml and present in the latter. Shell Config is corrected
from 587c8164024986725b8a0f4b4a7fa03d3fc4c8cd (0.1.0) to published
50143ee0f211d658b26f74bee1ddd5f310e89f7e (0.2.0), matching the existing
find_package minimum and HUD schema. These are source-verified incompatibilities.

Preserve full private-D-Bus CTest, package consumer, activation-prefix fixtures,
import policy and its fixtures, QML types/lint, all four installed startup modes,
activation Exec validation, full formatting/static analysis and REUSE 6.2.0.
The standalone provider consumer has its own compiler database for tidy.
Qt5 probes remain disabled. No live desktop automation, host installation,
publication, pin updates or host security configuration changes.

Owned host clang-tidy diagnostics must be resolved before acceptance. Keep all
check families and scope headers to their owning repository. Fresh explicit
compiler contexts avoid unrelated stale developer artifacts.

## Implementation and acceptance — 2026-10-04

Shared scripts/manifests live in scripts/ci. Task and existing independent build,
static and licensing jobs invoke the same launcher. Full startup traces and CTest
evidence are collected; publication and image-building workflows are unchanged.
The primary static target checks 19 owned translation units; the standalone
installed-provider consumer is built and analyzed in its own compiler context.
Default compilation/analysis parallelism is two workers.

Owned host diagnostics were corrected with trailing commas, empty lambda parameter
removal and the internal apply_mode field spelling. The external applyMode QML/JSON
key and all product/protocol/fixture string values/counts remain unchanged. Existing
assertions and QML behavior are preserved. An explicit HOLONIGHT_TIDY_DATABASE
override and scoped coverage regressions avoid stale unrelated compiler contexts.

`python3 build/ci/acceptance.py` invokes `task ci`, which exits zero. All three lanes
pass in build/ci/20261004T110053Z-4n1k00u1: 55 CTest checks, private-D-Bus Controls
acceptance, startup, package-consumer/activation-prefix/import-policy fixtures,
QML types/lint, all four installed startup modes and activation Exec checks,
full format/tidy and standalone consumer analysis, and REUSE 6.2.0. Complete logs
were inspected; only expected Qt private-header compatibility notices remain with
the same Qt build on both sides. All 12 evidence files are host-owned. Content,
modes, timestamps and symlinks of 1,995 source/development files remain unchanged.

Fresh native Debug build, all 55 CTest checks, full CMake tidy with clang 23,
separate consumer build/run/tidy, format, QML types/lint, import-policy fixtures and
licensing pass. `HOLONIGHT_TIDY_DATABASE=build/ci/native-all-compile-commands.json
task tidy` passes with both primary and standalone-consumer compiler contexts.
Complete logs are build/ci/native-*-final.log and build/ci/native-consumer.log.
Four launcher regressions, two compiler-context regressions, shell/Python syntax,
string comparison and final diff checks pass. Initial owned diagnostic logs remain
available; no check families were disabled to hide source errors.

Native provider provenance is build/ci/native-providers/provenance.json. Config,
Shell Config and Audio are built at their accepted revisions. Compatible Qt
Release artifacts at 8d11e3e are reused after checking compiler, Qt, build type and
feature options, including disabled Qt5 probes. The Config headers, implementation
and package contract at accepted 81b01d3 are unchanged from fe69a59 used to build
that artifact; accepted Config is installed separately and its library prefix is
selected explicitly. Container providers are always built fresh at their declared
revisions. Real Podman remains unverified because it is unavailable. No push,
submodule pin update, live desktop interaction or host configuration change.
