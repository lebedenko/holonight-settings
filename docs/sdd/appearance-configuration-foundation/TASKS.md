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
- [x] ACF5-09: Add redacted domain-isolation, conflict, partial-failure, QML, package, and clean-break regression tests.
- [x] ACF5-10: Run format, tidy, QML lint/types, full CTest, and manual Settings-to-Qt/Shell Hyprland checks; publish
  the verified commit for umbrella handoff.

## Completion evidence

Record exact commands, versions, automated results, manual observations, redaction checks, and the published commit
before requesting ACF-005 `Done`. A local or unpublished commit is not a handoff.

- 2026-08-08: provider-backed build passed; 13/13 CTest entries passed with one environment-dependent D-Bus skip;
  format check, QML lint, and QML type metadata passed. The tidy workflow now filters GCC's unsupported
  `-mno-direct-extern-access` argument and the complete clang-tidy target passes, including activation-service checks.
  Hyprland manual verification remained open.
- 2026-08-08: ACF5-09 regression coverage completed for canonical value conversion, redacted invalid startup,
  missing/invalid/atomic-write and discard failures, file-identity conflicts with reload/overwrite/recheck,
  isolated and partial multi-domain operations, retry and busy suppression, unavailable-font retention, QML control
  and routing contracts, installed provider consumption, and clean-break searches. The provider-backed build and all
  31 CTest entries passed (the existing environment-dependent D-Bus test skipped); format check, QML lint, QML type
  metadata, and the complete clang-tidy target passed. ACF5-10 Hyprland verification and published handoff remain open.
- 2026-08-08: live Hyprland review found and corrected a QML import collision that selected Basic controls and caused
  Appearance component creation to fail. After rebuilding, the user confirmed Appearance, Bar, Weather, and
  placeholder navigation render independently without console or styling issues. An isolated offscreen launch also
  remained error-free. The save propagation, partial-failure, and external-conflict portions of ACF5-10 remain open.
- 2026-08-08: final local verification at implementation `76504d1` passed all 31 CTest entries, including
  `SettingsSaveCoordinatorTest.AppearanceSuccessAndShellFailureAreIndependentAndRetryable`, the installed-provider
  consumer, and clean-break test; `cmake --build build --target format-check`, `tidy`, and `qml-lint`,
  `bash scripts/check-qmltypes.sh build`, and an application install smoke to
  `/tmp/holonight-settings-install-smoke` passed. An offscreen launch emitted no swatch or navigation QML errors.
- 2026-08-08: live Hyprland verification passed reversible Settings-to-Qt/Shell propagation through
  `appearance.toml`. An external accent edit applied immediately to consumers while Settings retained its dirty
  snapshot; silent overwrite was blocked, and Cancel, Reload, and explicit Overwrite behaved as specified. Before
  and after hashes/metadata confirmed appearance-only operations did not modify `config.toml`, `theme.conf`, or
  `appearance.json`; no configuration contents or secret values were recorded. The isolated two-domain partial-save
  and retry path is covered deterministically by the passing coordinator test above. The conflict dialog was restyled
  with HoloNight surfaces and controls after the live review exposed its Basic appearance.
- 2026-08-08: GitHub CI run `31258908617` passed both `build-test` and `static-checks` for published implementation
  `76504d1`, using exact providers `holonight-config@81b01d3`, `holonight-qt@6f591cb`, and
  `holonight-shell@93e1faf` in Config -> Qt -> Shell-config order. The shared Qt shutdown diagnostic
  `QSocketNotifier: current thread's event dispatcher has already been destroyed` remains observable in both Settings
  and Shell and is not specific to appearance persistence or propagation.
