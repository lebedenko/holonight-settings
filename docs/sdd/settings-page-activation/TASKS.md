# Settings page activation tasks

Status: Implemented and verified (2026-08-12)

- [x] Document the page-aware activation contract and design.
- [x] Carry an optional page key with activation platform data.
- [x] Interpret `ActivateAction` action names as page keys without changing the D-Bus signature.
- [x] Preserve latest-request-wins behavior for early activation.
- [x] Emit page requests before restoring the settings window.
- [x] Connect activation transport to the QML root before attaching the window.
- [x] Validate requested keys against `NavPanel.pages` and update `SettingsWindow.currentPage` only on a match.
- [x] Cover immediate, queued, generic, runtime D-Bus, platform-data, and QML routing behavior with tests.
- [x] Update the existing single-instance activation design.
- [x] Run and record focused and repository-wide verification.
