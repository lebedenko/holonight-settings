# SDD Tasks — appearance-configuration-foundation

All tasks remain unchecked until umbrella ACF-005 is `Ready`. Implement only after the exact appearance, Qt, and
Shell product provider revisions are published and pinned.

- [x] ACF5-01: Adopt `HoloNight::Config` and `HoloNightShellConfig::Config` packages in CMake and Task workflows.
- [x] ACF5-02: Add `AppearanceEditModel` covering every canonical field, catalog projections, validation, snapshots,
  and dirty state.
- [x] ACF5-03: Add `AppearanceFileService` with missing/invalid load, atomic save, discard, diagnostics, and
  external-change conflict handling.
- [x] ACF5-04: Convert the existing edit/file service to the Shell product domain using its published package.
- [x] ACF5-05: Add a save coordinator with isolated dirty-domain dispatch, partial-success reporting, and retry.
- [x] ACF5-06: Update the Appearance page for canonical typography, icons, cursor, layout, and optional shape values;
  remove transparency, blur, and persisted mode semantics.
- [x] ACF5-07: Consolidate duplicate Apply/Save behavior or define and test a real behavioral distinction.
- [x] ACF5-08: Delete `ThemeConfigFile`, legacy INI/JSON integration, the local exported config library, forwarding
  APIs, duplicated defaults, obsolete tests, and documentation.
- [ ] ACF5-09: Add redacted domain-isolation, conflict, partial-failure, QML, package, and clean-break regression tests.
- [ ] ACF5-10: Run format, tidy, QML lint/types, full CTest, and manual Settings-to-Qt/Shell Hyprland checks; publish
  the verified commit for umbrella handoff.

## Completion evidence

Record exact commands, versions, automated results, manual observations, redaction checks, and the published commit
before requesting ACF-005 `Done`. A local or unpublished commit is not a handoff.

- 2026-08-08: provider-backed build passed; 13/13 CTest entries passed with one environment-dependent D-Bus skip;
  format check, QML lint, and QML type metadata passed. The tidy workflow now filters GCC's unsupported
  `-mno-direct-extern-access` argument and the complete clang-tidy target passes, including activation-service checks.
  Hyprland manual verification and the complete ACF5-09 matrix remain open.
