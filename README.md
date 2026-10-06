# HoloNight Settings

The standalone Qt 6 settings application edits shared appearance and Shell preferences. Shell owns its exported
configuration schema; the toolkit-neutral Config library owns document editing and appearance compatibility.
Each other application retains its own configuration and settings UI.

## Build

Qt 6 (Core, Gui, Quick, QML, D-Bus), CMake 3.25+, Ninja, toml++, and the sibling
[`holonight-qt`](https://github.com/lebedenko/holonight-qt) package are required. Python 3 and GTest are required for verification.

```sh
task build
task test
```

Appearance persistence is provided by `HoloNight::Config`, while Shell product settings use
`HoloNightShellConfig::Config`. Settings owns only edit state and user-facing save coordination.

## Quick Controls style and discovery

Standard application controls use `import QtQuick.Controls as Controls`. Explicit `Holonight.Core` and
`Holonight.Controls` imports retain the shared design system. Run `task qml-import-check` to check this policy.
The executable embeds `:/qtquickcontrols2.conf` with `Style=Holonight`. Qt supports an environment override
(`QT_QUICK_CONTROLS_STYLE=Fusion`), `-style Fusion`, or an external `QT_QUICK_CONTROLS_CONF` file; there is no
imperative style selection.

`task` stages sibling dependencies under `build/deps/prefix` with provider tests/examples disabled.
CMake derives `SETTINGS_DEPENDENCY_QML_DIR` from the installed HolonightQt package; override that cache path for
nonstandard layouts. Only build executables use it. Installed settings discovers `../lib/qt6/qml` (using the
configured install libdir) relative to its executable, then Qt's ordinary module paths.

For isolated acceptance, configure with `BUILD_TESTS=ON`, a staged `CMAKE_PREFIX_PATH`, and a disposable
`CMAKE_INSTALL_PREFIX`. CTest runs the real window under HoloNight/Fusion and actual build launches in four
style-selection modes using private D-Bus sessions and temporary configuration. After installing settings and
its dependencies into the configured prefix, run `tests/check_settings_startup.py` against its `bin/holonight-settings`
and `lib/qt6/qml` in `default`, `environment`, `command-line`, and `configuration` modes.
Install at the configure-time prefix: the D-Bus activation service records that exact executable path.
See the [UQC-103 verification record](docs/sdd/unified-qtquick-controls/IMPLEMENTATION.md).

## Standalone developer tooling

See [tooling/README.md](tooling/README.md) for presets, local dependency overrides, editor refresh,
`task tooling:doctor`, and the independent Serena project.

## Local CI rehearsal

Run `task ci` with Python 3, Task and Docker (Podman fallback). It runs every push
validation lane in fresh isolated builds with the same pinned tools and providers
as GitHub. New non-ignored inputs are included and reported for adding before
pushing. Complete logs and evidence live in ignored `build/ci/`; any required
failure returns nonzero and prints its full log. Existing developer tasks remain
available. Publication/uploads remain remote operations. See
[local SDD](docs/sdd/local-ci/README.md).

## Configuration editing

Save writes pending value patches and retains unrelated text, comments and unknown fields. Reset removes the selected
override; absent values use domain defaults. Valid appearance v1 files upgrade to sparse v2 on their first successful
save while retaining other explicit values. Unsupported versions and invalid external documents cannot be overwritten.

External changes update untouched controls. Pending edits remain visible; review conflicts per preference and either
keep the pending value or accept the external value. Save checks the latest disk values again. Discard loads the latest
valid document; a failed reload retains edits. Defaults and override status, sparse resets and diagnostics appear on
the existing Appearance, Bar and Weather pages. API key conflict values are masked.

Appearance and Shell saves report separate outcomes. Appearance is staged before native application; failures restore
the exact prior document only while both its staged revision and physical target remain current. An intervening editor
change is preserved and reported as an application failure. Cooperating writers lock the target's sibling lock file;
arbitrary editors retain a race between the final revision check and replacement. No daemon or running Shell is needed
for document editing. Files, Viewer and other applications are outside this settings migration.
