# Explicit shared icon rendering in Settings

Baseline: `0eb5028e206e12791ecbe6c2404bc37c39a4962a`.

Navigation glyphs are bundled QRC assets. Select semantic rendering explicitly and verify QML lint and focused Settings acceptance against the accepted `holonight-qt` revision.

Implementation: `apps/settings/qml/NavPanel.qml`. Local verification (2026-09-25): `task test` passed (53 tests) and `task qmltypes-check` passed against the local provider build. `task qml-lint` exits successfully but emits two false missing-property warnings for the new `HnIcon` property and enum even though both are present in the installed provider QML.
