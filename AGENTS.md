# Repository Guidelines

Use Conventional Commits for every new commit: `type(scope): imperative summary`, or `type: imperative summary` when a scope adds no clarity.

`apps/settings/` owns the executable and QML UI. The separate Config repository owns document editing and
appearance schema; Shell owns its standalone configuration export. Tests live in `tests/`; feature design
records live under `docs/sdd/`.

Use `task build`, `task test`, `task format-check`, `task tidy`, `task qml-lint`, and `task qmltypes-check` for local
development. Run `task deps` explicitly to prepare Config, System Services (including Audio), Qt (including Wayland),
and Shell’s standalone configuration export into `build/deps/prefix`, with provider tests/demos disabled.
Configure, build and test only validate prepared providers and installed prerequisites.

Use C++23 and the checked-in formatting/static-analysis configuration. Preserve the imported provider targets
and configuration-package consumer coverage.

Do not import `QtQuick.Controls.Basic` in QML. Import `QtQuick.Controls` with a namespace alias, such as
`import QtQuick.Controls as Controls`, and qualify controls, enums and attached properties through that alias. Do not import the Holonight style directly.
Keep Core/composites explicit; run `task qml-import-check`. Preserve the embedded overridable style default.

Developer tooling uses `build/debug`, `build/test`, `build/release` and module-owned `build/deps`.
See tooling/README.md; run task tooling:refresh explicitly after configuring/building for editor metadata.

Dependency preparation is explicit: `task deps` uses supplied local provider sources in dependency order.
Qt >=6.11, libpulse and toml++ must be installed; tests require GTest >=1.14 CONFIG with GMock.
`HOLONIGHT_DEPENDENCY_PREFIX` selects an existing internal installation; `HOLONIGHT_EXTERNAL_PREFIX`
selects installed external libraries. Incompatible caches are preserved for inspection.
Run `task ci:prepare` before `task ci`: acquisition happens only during preparation, followed by
four offline qualified Arch/REUSE lanes. See tooling/README.md for source overrides.
