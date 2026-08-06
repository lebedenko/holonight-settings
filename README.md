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

To build and install only the shared configuration package:

```sh
cmake -S . -B build-config -G Ninja -DBUILD_APP=OFF
cmake --build build-config
cmake --install build-config --prefix /desired/prefix
```

Consumers use `find_package(HolonightConfig CONFIG REQUIRED)` and link `HolonightConfig::Config`. Existing includes
such as `<holonight_config/config_structs.h>` remain supported.
