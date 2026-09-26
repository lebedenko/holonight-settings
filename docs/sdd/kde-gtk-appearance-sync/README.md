# Native appearance status

Settings keeps the existing staged save, adapter apply, and rollback transaction. The Integrations page sends the current canonical appearance path with status refresh, so the adapter can distinguish output edits from canonical changes. Reapply continues through the same adapter client.

## Implementation tasks and files

- [x] `apps/settings/src/AppearanceAdapterClient.h` and `.cpp`: add an optional canonical path to status requests while keeping no-path callers valid.
- [x] `apps/settings/src/SettingsSaveCoordinator.cpp`: send the active appearance file path when refreshing integrations.
- [x] `tests/test_acf005.cpp`: verify the status request carries the canonical path.

Verification on 2026-09-26: focused adapter client and save coordinator tests 10/10; `task test` 54/54; `task format-check`, `task tidy`, `task qml-lint`, `task qmltypes-check`, `task qml-import-check`, and REUSE lint pass.
