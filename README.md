# HoloNight Settings

The standalone Qt 6 settings application and configuration-schema package for HoloNight. The application edits the
same TOML configuration consumed by `holonight-shell`; this repository is the single owner of that schema.

## Build

Qt 6 (Core, Gui, Quick, QML, D-Bus), CMake 3.25+, Ninja, toml++, and the sibling
[`holonight-qt`](https://github.com/lebedenko/holonight-qt) package are required.

```sh
task build
task test
```

Appearance persistence is provided by `HoloNight::Config`, while Shell product settings use
`HoloNightShellConfig::Config`. Settings owns only edit state and user-facing save coordination.
