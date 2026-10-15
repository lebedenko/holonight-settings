# Repository Guidelines

Use Conventional Commits for every new commit: `type(scope): imperative summary`, or `type: imperative summary` when a scope adds no clarity.

`apps/settings/` owns the executable and QML UI. The separate Config repository owns document editing and
appearance schema; Shell owns its standalone configuration export. Tests live in `tests/`; feature design
records live under `docs/sdd/`.

Use `task build`, `task test`, `task format-check`, `task tidy`, `task qml-lint`, and `task qmltypes-check` for local
development.

Use C++23 and the checked-in formatting/static-analysis configuration. Preserve the imported provider targets
and configuration-package consumer coverage.

Do not import `QtQuick.Controls.Basic` in QML. Import `QtQuick.Controls` with a namespace alias, such as
`import QtQuick.Controls as Controls`, and qualify controls, enums and attached properties through that alias. Do not import the Holonight style directly.
Keep Core/composites explicit; run `task qml-import-check`. Preserve the embedded overridable style default.




## Development and documentation

Use `task build` and `task test` for incremental native CMake/CTest work. Native builds
consume installed packages; use `CMAKE_PREFIX_PATH` for an explicit development prefix.
Use `task format` for the pinned formatter and `task check` for container acceptance.
Run `task check:integration` for installation/runtime changes. Container builds use
fresh build/prefix directories, separate from `build/dev` and `build/release`.
clangd reads `build/dev/compile_commands.json` directly.

Routine changes need tests and a commit explanation; update user documentation when
behavior changes. Substantial designs need one local SDD describing intent, decisions,
interfaces and acceptance. Cross-project changes need one umbrella note linking local
designs and stating contracts, dependency order and integration checks. Historical SDDs
remain historical. No separate planning, readiness, handoff or closure commits are required.
Keep routine logs and generated evidence in ignored build directories or CI artifacts.
Provider and consumer implementation may proceed together; publish providers before
consumer commits that reference them. Update umbrella pins per coherent milestone.
