# HoloNight Settings

The standalone Qt 6 settings application and configuration-schema package for HoloNight. The application edits the
same TOML configuration consumed by `holonight-shell`; this repository is the single owner of that schema.

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

`task` stages sibling dependencies under `build-dependencies/prefix` with provider tests/examples disabled.
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
