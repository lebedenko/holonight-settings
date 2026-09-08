# Implementation record

## 2026-09-08: UQC-103 local acceptance

Started from settings `579515ffb456c59cd1299e5852c392c3064c8262`, umbrella
`e3811c90ba8e14fae84f4419bdcb02f822b338d6`, and published/pinned provider
`50c59558bb3817f57a992dd72730dba141db1bc8`. Published settings design checkpoint
`0c11035b5682fc51f608b947072573a577b0e613`, verified canonical origin/main, then published umbrella
In Progress checkpoint `8956371` before application implementation.

## Final change

- Appearance, Bar, Weather, Integrations, FooterBar and ContentStack use namespaced runtime Controls,
  including ButtonGroup, StackView transitions, Dialog and Overlay. Audio retains its existing namespace.
- Appearance Switch uses a conditional Binding for the provider-only Large size role; Fusion has native sizing.
  Explicit Core/composites, frame composition, custom swatch painting, validators and save/conflict bindings remain.
- Root embedded qtquickcontrols2.conf supplies Style=Holonight with no imperative selection. Engine discovery uses
  configured provider QML only when launched from its build executable directory; installed binaries use their
  executable-relative libdir. Activation service still records the configure-time install prefix.
- The settings CMake helper shares production sources, QML inventory and resources with the acceptance executable.
  Its separate main inspects Qt's actual window without production accessors or diagnostic modes. Generated QML
  metadata has per-target output directories; qml-lint and qmltypes scripts follow the production directory.
- Application import checker follows the provider runtime namespace and explicit Core rules. Thirteen isolated
  positive/negative fixtures include instances, enums, attached properties, fallback types and missing imports.
  CTest and CI run these checks; both CI provider references point to the UQC-101 prerequisite.
  The CI image explicitly installs Python for the new verification scripts.
- Task dependency builds stage under settings/build-dependencies, explicitly disable provider tests/examples, and
  leave sibling build trees alone. CTest, package-consumer and lint paths derive from CMake configuration.
  CI stages dependencies and verifies the installed executable in all four modes. README documents the contracts;
  local ignored AGENTS.md instructions were aligned too (they are not a published repository file).

No public configuration schema, D-Bus interface or provider API changed. Only settings implementation and umbrella
coordination are in scope; unrelated package-manager files are preserved.

## Verification commands and results

Commands below run from the settings root, using Qt 6.11.2, Debug/Ninja, and the exact sibling revisions pinned
by the initial umbrella. Build artifacts/logs are local, ignored or under /tmp; no system installation occurred.

```sh
task build:qt-dependency
cmake -S . -B build-uqc103 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/build-uqc103/stage" \
  -DCMAKE_PREFIX_PATH="$PWD/build-dependencies/prefix"
cmake --build build-uqc103 -j8
python3 scripts/check-qml-import-policy.py
python3 tests/test_qml_import_policy.py
ctest --test-dir build-uqc103 -R 'settings_(qml_import|controls_|startup_)' --output-on-failure
dbus-run-session -- ctest --test-dir build-uqc103 --output-on-failure --no-tests=error
cmake --build build-uqc103 --target format-check
cmake --build build-uqc103 --target tidy
cmake --build build-uqc103 --target qml-lint
bash scripts/check-qmltypes.sh build-uqc103
bash -n scripts/check-qmltypes.sh
task --list
git diff --check
```

All dependency/application builds passed. The complete settings suite passed **53/53** (45.53 seconds), including
installed package consumer and both activation-prefix fixtures. After strengthening only acceptance/policy checks,
all **8/8** focused entries passed again (15.98 seconds). Formatting, full clang-tidy, QML lint and generated type
metadata checks passed. The final acceptance source also passed focused static analysis:

```sh
run-clang-tidy -quiet -p build-uqc103 -removed-arg=-mno-direct-extern-access tests/settings_controls_acceptance.cpp
```

Python scripts were parsed with ast.parse; Taskfile was parsed with task --list; workflow run blocks and the QML
metadata script passed shell syntax checks. Local relative documentation links and whitespace were checked.

Staging and actual installed launches:

```sh
cmake --install build-dependencies/config --prefix "$PWD/build-uqc103/stage"
cmake --install build-dependencies/qt --prefix "$PWD/build-uqc103/stage"
cmake --install build-dependencies/shell-config --prefix "$PWD/build-uqc103/stage"
cmake --install build-dependencies/system-services --prefix "$PWD/build-uqc103/stage"
cmake --install build-uqc103
for mode in default environment command-line configuration; do
  python3 tests/check_settings_startup.py build-uqc103/stage/bin/holonight-settings \
    build-uqc103/stage/lib/qt6/qml "$mode" \
    --forbid-qml-root "$PWD/build-dependencies/prefix/lib/qt6/qml" || exit
done
```

All four installed modes passed, including the final strict diagnostic runner. The installed D-Bus service's Exec
path exactly matches the configured staging prefix and names an executable file. The installed import trace does
not contain the build dependency QML path; all loaded HoloNight QML plugins come from the staged prefix.

## Evidence and isolation boundaries

HoloNight/Fusion acceptance constructs SettingsApplication, visits appearance/bar/weather/integrations/audio/about
by setting currentPage, and waits for StackView completion. It verifies implementation-child contexts (the instance's
qmlContext itself describes the application's creation site), standard control import resolution and actual plugin
mappings. It checks Switch compatibility/Large sizing, model-to-editor and editor-to-model changes, font ComboBox
creation/selection and deterministic forty-row scrolling, weather scrolling, Core/composite origins, swatch tokens,
unavailable audio, and a real disposable-file revision conflict with dialog open/cancel and retained dirty edits.

The startup runner checks actual production build/installed binaries for embedded HoloNight, environment Fusion,
command-line Fusion over environment HoloNight, and external Fusion configuration. Each must remain alive through
a three-second observation, resolve representative controls, load the expected plugins and emit no unexpected
diagnostics. The runner then explicitly terminates and reaps it. Acceptance must exit successfully with its completion
marker within thirty seconds; premature exits, missing evidence and timeouts fail.

Every runtime process uses a private D-Bus daemon, temporary XDG configuration/cache/data/runtime paths, an offscreen
software platform, and an unavailable PulseAudio socket. It does not invoke adapter actions, desktop activation,
pointer/focus automation, or live audio operations. Only in-memory edits and disposable files are used. Startup's
existing internal raise request produces the exact offscreen warning "This plugin does not support raise()";
that and the expected refused-audio diagnostic are the only tolerated warnings. All QML diagnostics fail.

## Tooling limitations and resolved test-development failures

The sandbox denied private D-Bus socket creation; these isolated runs used the approved escalation. Initial test
builds exposed shared QML output-directory collisions, resolved with per-target metadata directories. Initial origin
checks used creation contexts and Repeater children were missed by QObject-only traversal; corrected tests inspect
implementation children and both QObject/visual ownership. These were test harness issues, not product regressions.

Direct clang-tidy rejects GCC's existing -mno-direct-extern-access compile flag. The repository's established
run-clang-tidy -removed-arg workflow passes, including the final focused source check. No checks remain blocked.

This completes local UQC-103 acceptance. Publication and umbrella handoff follow this record. Human-operated
Hyprland/Sway, real-application and ecosystem activation acceptance remain UQC-201 gates; this is not an integrated
initiative or a claim of desktop visual/interaction acceptance.

## Published handoff — 2026-09-08

Implementation `d45141e9b9ee191c64bc334eca0ad505e25cd582` (`feat(settings): adopt overridable runtime Quick Controls`)
is published on canonical origin/main. `git ls-remote origin refs/heads/main` returned that exact revision after
push. This documentation-only handoff closes local UQC-103; the umbrella coordinator can pin its published tip.
The completed checks above establish local acceptance, while UQC-201 remains the final ecosystem gate.
