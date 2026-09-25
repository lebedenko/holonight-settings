# Repository Guidelines

Use Conventional Commits for every new commit: `type(scope): imperative summary`, or `type: imperative summary` when a scope adds no clarity.

`apps/settings/` owns the executable and QML UI. `libs/holonight-config/` owns the public configuration schema,
parser, writer, and exported `HolonightConfig::Config` package. Tests live in `tests/`; feature design records live
under `docs/sdd/`.

Use `task build`, `task test`, `task format-check`, `task tidy`, `task qml-lint`, and `task qmltypes-check` for local
development. These workflows build and install the sibling `../holonight-qt` dependency into `build-dependencies/prefix` without provider tests/examples.

Use C++23 and the checked-in formatting/static-analysis configuration. Keep the public headers under
`holonight_config/` source-compatible because the shell consumes this package directly.

Do not import `QtQuick.Controls.Basic` in QML. Import `QtQuick.Controls` with a namespace alias, such as
`import QtQuick.Controls as Controls`, and qualify controls, enums and attached properties through that alias. Do not import the Holonight style directly.
Keep Core/composites explicit; run `task qml-import-check`. Preserve the embedded overridable style default.
