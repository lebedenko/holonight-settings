# CTV-105 Settings Activation Prefix

## Problem

Settings installs a D-Bus activation service whose `Exec` value must identify the installed executable. Passing a
different prefix only to `cmake --install` changes the artifact destination without regenerating the configured
service, so activation can target a binary under the earlier configure-time prefix.

## Contract

- The install prefix is final at CMake configure time.
- Local Taskfile builds configure for `$HOME/.local`; release/system builds configure for `/usr`.
- Installation does not override the configured prefix.
- `org.holonight.Settings.service` contains the absolute configured bindir plus `holonight-settings`.
- CI stages to a non-system prefix and verifies both the service `Exec` value and its installed executable.
- Deterministic CTest cases cover `/usr` and a non-system prefix without writing to either location.

## Verification

Run `task test`, `task format-check`, `task tidy`, `task qml-lint`, and `task qmltypes-check`. Configure and install a
release build into its configured staging prefix, then verify the installed service resolves an executable in that
same prefix.
