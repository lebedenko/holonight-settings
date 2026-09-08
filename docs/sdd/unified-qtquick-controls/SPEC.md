# UQC-103: Runtime Quick Controls adoption

Status: Locally verified; publication handoff pending

Settings baseline: `579515ffb456c59cd1299e5852c392c3064c8262`.
Umbrella baseline: `e3811c90ba8e14fae84f4419bdcb02f822b338d6`.
Provider prerequisite: `50c59558bb3817f57a992dd72730dba141db1bc8`.

Application standard controls use `QtQuick.Controls as Controls`, including enums and attached properties.
Every executable embeds an overridable HoloNight default. Explicit Core/composite visuals remain intentional.
Preserve model bindings, validators, custom swatches, frame composition, save/conflict behavior and activation prefix.
No public schema, D-Bus or provider API changes.

See [design](DESIGN.md), [tasks](TASKS.md) and [verification record](IMPLEMENTATION.md).

## Acceptance matrix

- Separate HoloNight/Fusion processes inspect the production window, all five implemented pages and a placeholder.
- Verify standard-control origins, provider plugin paths, Core/composites, Switch sizing, editing, ComboBox scrolling,
  and the footer conflict dialog using disposable configuration and in-memory edits.
- Actual build and staged-install executables: embedded default, environment Fusion, command-line Fusion over
  environment HoloNight, and external Fusion configuration; require origins, plugin paths and no QML errors.
- Private D-Bus sessions, temporary XDG directories, offscreen software rendering and unavailable audio endpoint.
  No desktop activation, pointer/focus automation, live adapters or system settings writes.
- Focused checks, full CTest, formatting, tidy, QML lint/types, package/activation tests, syntax, links and whitespace.
- Publish verified settings commits before the umbrella pin; human Hyprland/Sway and ecosystem checks remain UQC-201.
