# SDD Tasks — holonight-settings

## Phase 1 — Monorepo Restructure

- [x] T-001: Create libs/holonight-config directory structure
  - REQs: structural
  - Check: `ls -d libs/holonight-config/{include/holonight_config,src}` succeeds; both directories exist and are empty.

- [x] T-002: Migrate holonight_config headers to libs/holonight-config
  - REQs: REQ-C-001
  - Check: `include/holonight_config/*.h` files are copied to `libs/holonight-config/include/holonight_config/`; original include/ dir is unmodified until Phase 1.8.

- [x] T-003: Migrate ConfigParsers and ConfigWriter to libs/holonight-config
  - REQs: REQ-C-001
  - Check: `src/core/ConfigParsers.cpp` and `src/core/ConfigWriter.cpp` are copied to `libs/holonight-config/src/`; original files unmodified.

- [x] T-004: Write libs/holonight-config/CMakeLists.txt build target
  - REQs: structural
  - Check: CMake configures `add_library(holonight_config STATIC ...)` with TOMLPLUSPLUS::TOMLPLUSPLUS PUBLIC; target has `target_include_directories(...  PUBLIC include)`.

- [x] T-005: Update shell's include paths after config lib migration
  - REQs: structural
  - Check: All `#include "holonight_config/..."` statements in `src/services/ConfigService.cpp` resolve without error after changing root CMakeLists to `add_subdirectory(libs/holonight-config)`.

- [x] T-006: Update root CMakeLists to include libs/holonight-config subdirectory
  - REQs: structural
  - Check: `task build` succeeds; `ldd build/holonight-shell` contains `libholonight_config` (symbol resolution test).

- [x] T-007: Remove old include/holonight_config and src/core/Config*.cpp
  - REQs: structural
  - Check: `rm -rf include/holonight_config src/core/ConfigParsers.cpp src/core/ConfigWriter.cpp`; `task build` still succeeds.

- [x] T-008: Create apps/settings directory structure (simplified: apps/settings only, src/ stays in place)
  - REQs: structural
  - Check: `mkdir -p apps/settings/{src,qml}`; both exist.

- [x] T-009: Update apps/settings/CMakeLists.txt — added in T-012
  - REQs: structural

- [x] T-010: Update root CMakeLists.txt to orchestrate libs and apps/settings subdirectories
  - REQs: structural
  - Check: Root `CMakeLists.txt` has `add_subdirectory(libs/holonight-config)` and `add_subdirectory(apps/settings)`; `task configure && task build` is green.

---

## Phase 2 — Settings Binary Scaffold

- [x] T-011: Create apps/settings directory structure
  - REQs: structural
  - Check: `mkdir -p apps/settings/{src,qml,resources}`; `ls -d apps/settings/{src,qml,resources}` all exist and are empty.

- [x] T-012: Write apps/settings/CMakeLists.txt with executable and QML module
  - REQs: REQ-F-002, REQ-C-006, REQ-C-007
  - Check: CMakeLists defines `qt_add_executable(holonight-settings ...)` and `qt_add_qml_module(holonight-settings URI HolonightSettings ...)`; links only `holonight_config Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Widgets`; does NOT link `holonight_services`, `Qt6::DBus`, or Wayland libraries.

- [x] T-013: Create apps/settings/src/main.cpp entry point
  - REQs: REQ-F-001, REQ-F-003
  - Check: `main.cpp` contains `#include <QGuiApplication>` (or `QApplication`), constructs a `SettingsApplication`, and calls `app.exec()`.

- [x] T-014: Create SettingsApplication.h/cpp skeleton
  - REQs: REQ-F-001, REQ-F-003
  - Check: `SettingsApplication : public QGuiApplication` exists with constructor; class is declared in `apps/settings/src/SettingsApplication.{h,cpp}`.

- [x] T-015: Create empty SettingsWindow.qml with 1200×800 default size
  - REQs: REQ-F-004
  - Check: `apps/settings/qml/SettingsWindow.qml` is a `Rectangle` or `Window` with `width: 1200; height: 800;` properties; QML parses without error.

- [x] T-016: Register SettingsWindow.qml in apps/settings/CMakeLists.txt QML_FILES
  - REQs: REQ-C-006
  - Check: CMakeLists has `set(SETTINGS_QML_FILES qml/SettingsWindow.qml)` and passes it to `qt_add_qml_module(...  QML_FILES ${SETTINGS_QML_FILES})`.

- [x] T-017: Wire SettingsApplication to create and show QQuickView
  - REQs: REQ-F-004
  - Check: `SettingsApplication` instantiates `QQuickView`, sets source to `qrc:/HolonightSettings/SettingsWindow.qml`, calls `view.show()`, and returns the view.

- [x] T-018: Configure holonight-settings in root CMakeLists and verify build
  - REQs: structural
  - Check: Root `CMakeLists.txt` has `add_subdirectory(apps/settings)`; `task configure && task build` completes with no errors; `build/holonight-settings` binary exists and is executable.

---

## Phase 3 — C++ Model Layer

- [x] T-019: Implement SettingsEditModel.h with 10 Q_PROPERTYs and isDirty
  - REQs: REQ-F-009, REQ-F-012, REQ-F-015
  - Check: `SettingsEditModel.h` declares `Q_PROPERTY` for `themeVariant`, `themeAccent`, `themeMode`, `uiFont`, `uiFontSize`, `fixedFont`, `fixedFontSize`, `workspaceCount`, `trayMaxItems`, `isDirty` (read-only); each property has READ, WRITE, NOTIFY; isDirty uses custom recompute logic.

- [x] T-020: Implement SettingsEditModel.cpp setFromParsedConfig method
  - REQs: REQ-F-015
  - Check: `setFromParsedConfig(const ParsedConfig&)` takes a `ParsedConfig`, stores it into `current_` and `snapshot_`, then emits all `*Changed()` signals; uses `blockSignals(true/false)` or batch-updates to avoid mid-flight QML states.

- [x] T-021: Implement SettingsEditModel.cpp toParsedConfig method
  - REQs: REQ-F-015, REQ-F-016
  - Check: `toParsedConfig()` starts from `current_`, overwrites only the 10 MVP properties (using the live Q_PROPERTY values), and returns the result; non-MVP fields are preserved from the last-loaded config.

- [x] T-022: Implement SettingsEditModel isDirty comparison
  - REQs: REQ-F-015
  - Check: `recomputeDirty()` compares `current_` against `snapshot_` using `ParsedConfig::operator==`; isDirty recomputes and emits signal when any field changes.

- [x] T-023: Implement ConfigFileService.h with load/save/configPath
  - REQs: REQ-F-003, REQ-F-016, REQ-F-018, REQ-F-019
  - Check: Class declares `Q_INVOKABLE load()`, `save()`, `configPath()`; properties `isSaving`; signals `saveStarted()`, `saveFinished(bool)`, `saveError(QString)`.

- [x] T-024: Implement ConfigFileService load method
  - REQs: REQ-F-003, REQ-F-018
  - Check: `load()` resolves config path via XDG_CONFIG_HOME + "/holonight/config.toml"; calls `toml::parse_file()` with try/catch; calls `parseConfigTable()` on parsed result; calls `model_->setFromParsedConfig()`; returns false on parse error (model gets defaults).

- [x] T-025: Implement ConfigFileService save method
  - REQs: REQ-F-016, REQ-F-017, REQ-F-019, REQ-F-020
  - Check: `save()` sets `isSaving_=true` and emits `saveStarted()`; calls `model_->toParsedConfig()`; calls `ConfigWriter::write(parsed, configPath_)` synchronously; emits `saveFinished(ok)` and optionally `saveError(msg)` on failure; sets `isSaving_=false`.

- [x] T-026: Implement FontListModel.h with fixedPitchOnly property
  - REQs: REQ-F-010
  - Check: Class inherits `QAbstractListModel`; declares `Q_PROPERTY bool fixedPitchOnly` with READ/WRITE/NOTIFY; declares `Q_INVOKABLE int indexOf(const QString& family)`.

- [x] T-027: Implement FontListModel.cpp with QFontDatabase enumeration
  - REQs: REQ-F-010, REQ-C-002
  - Check: Constructor calls `QFontDatabase::families()`; `rebuild()` filters by `QFontDatabase::isFixedPitch()` when `fixedPitchOnly` is true; `rowCount()`, `data()`, `roleNames()` implement list model contract; `indexOf()` returns row index or -1; no hardcoded font names in source.

- [x] T-028: Register SettingsEditModel as QML element with QML_UNCREATABLE
  - REQs: structural
  - Check: `SettingsEditModel` class has `Q_OBJECT`, `QML_ELEMENT`, `QML_UNCREATABLE` macros; CMakeLists includes it in `qt_add_qml_module(... SOURCES SettingsEditModel.h SettingsEditModel.cpp)`.

- [x] T-029: Register ConfigFileService and FontListModel as QML elements
  - REQs: structural
  - Check: Both classes have `QML_ELEMENT` macro; CMakeLists includes both in `SOURCES` list; `qmllint` can resolve types `ConfigFileService` and `FontListModel` in QML files.

- [x] T-030: Integrate models into SettingsApplication and call load on startup
  - REQs: REQ-F-003
  - Check: `SettingsApplication::SettingsApplication()` constructs `SettingsEditModel` and `ConfigFileService`; exposes both via rootContext properties; calls `ConfigFileService::load()` before `QQuickView::show()`.

- [x] T-031: Verify no Wayland/D-Bus/services deps in link graph
  - REQs: REQ-F-002, REQ-C-008
  - Check: `task build` completes; `ldd build/holonight-settings | grep -E 'wayland|dbus|services'` returns empty (no unexpected symbols).

---

## Phase 4 — QML Skeleton

- [x] T-032: Write NavPanel.qml with 13 selectable section items
  - REQs: REQ-F-005, REQ-F-006, REQ-F-007
  - Check: NavPanel is a ~200px-wide left column; contains 13 `Text` or `Button` items (Appearance, Bar, Sidebar, Launcher, Weather, Notifications, Calendar, Audio, Workspaces, Keybindings, Integrations, Advanced, About); each is clickable/focusable; visual selection indicator (highlight, underline, or color) shows active item.

- [x] T-033: Implement NavPanel signal and state for page switching
  - REQs: REQ-F-007
  - Check: NavPanel emits `pageRequested(string pageKey)` signal when an item is clicked; tracks `currentPage` property that updates on click; styling reflects the active selection.

- [x] T-034: Write ContentStack.qml with page routing
  - REQs: REQ-F-007
  - Check: ContentStack uses `StackView` or `Loader` to switch between pages; receives `pageRequested(key)` signal from NavPanel; maps page keys to QML components (e.g., "appearance" → AppearancePage, "bar" → BarPage); deferred pages resolve to PlaceholderPage.

- [x] T-035: Write PlaceholderPage.qml for deferred nav items
  - REQs: REQ-F-006
  - Check: PlaceholderPage displays "Not yet implemented" text; used for all nav items except Appearance and Bar (Sidebar, Launcher, Weather, etc.).

- [x] T-036: Write FooterBar.qml with status and action buttons
  - REQs: REQ-F-008, REQ-F-019
  - Check: FooterBar is a fixed bottom bar (not scrolling); left side shows "Shell is running v{version}" with status indicator; right side has [Discard Changes], [Apply], [Save & Apply] buttons in that order; buttons have `enabled: !ConfigFileService.isSaving` binding.

- [x] T-037: Update SettingsWindow.qml to compose all three panels
  - REQs: REQ-F-005, REQ-F-007, REQ-F-008
  - Check: SettingsWindow is a `ColumnLayout` containing NavPanel (left, fixed width), ContentStack (center, resizable), and FooterBar (bottom, fixed height); left panel does not scroll; center content scrolls when overflow occurs.

- [x] T-038: Wire NavPanel → ContentStack page switching
  - REQs: REQ-F-007
  - Check: NavPanel emits `pageRequested(key)` → SettingsWindow connects signal → ContentStack switches to the corresponding page; clicking Appearance then Bar then Appearance again works correctly.

- [x] T-039: Implement window minimum and default sizes
  - REQs: REQ-F-004
  - Check: `SettingsApplication` sets `view.setMinimumSize(1000, 700)` and `view.resize(1200, 800)` before `show()`.

---

## Phase 5 — Appearance Page

- [x] T-040: Write AppearancePage.qml with field containers
  - REQs: REQ-F-009
  - Check: AppearancePage contains 7 groups/rows for: Color Scheme, Accent Color, Dark/Light Mode, Interface Font, Monospace Font, Interface Font Size, Monospace Font Size; layout is scrollable if content exceeds viewport.

- [x] T-041: Implement Theme Variant swatch grid (Storm, Ocean, Forest)
  - REQs: REQ-F-009
  - Check: Swatch grid has 3 clickable squares (or rounded rects) labeled "Storm", "Ocean", "Forest"; clicking a swatch sets `editModel.themeVariant = variantName`; active swatch is visually highlighted.

- [x] T-042: Implement Accent Color dots (6 options)
  - REQs: REQ-F-009
  - Check: 6 colored dots (cyan, purple, green, orange, red, pink) are displayed; clicking a dot sets `editModel.themeAccent = colorName`; active dot shows a selection indicator (e.g., border, ring, or checkmark).

- [x] T-043: Implement Dark/Light Mode toggle
  - REQs: REQ-F-009
  - Check: Toggle switch or radio buttons show "Dark" and "Light"; toggling sets `editModel.themeMode` to "dark" or "light"; visual state reflects current mode.

- [x] T-044: Implement UI Font dropdown with FontListModel
  - REQs: REQ-F-009, REQ-F-010
  - Check: ComboBox binds `model: FontListModel { fixedPitchOnly: false }`; `currentIndex: uiFontModel.indexOf(editModel.uiFont)`; selecting a font sets `editModel.uiFont = selectedFamily`.

- [x] T-045: Implement Monospace Font dropdown with fixedPitchOnly filter
  - REQs: REQ-F-009, REQ-F-010
  - Check: ComboBox binds `model: FontListModel { fixedPitchOnly: true }`; `currentIndex` resolves to saved `fixedFont` value; only monospace families are listed; selecting a font sets `editModel.fixedFont`.

- [x] T-046: Implement UI Font Size slider (8–18 pt)
  - REQs: REQ-F-009
  - Check: Slider has `from: 8; to: 18; stepSize: 1`; dragging updates `editModel.uiFontSize`; current value displays as a label; slider snaps to integer values; legacy values above 18 normalize to 18 and remain dirty until Apply.

- [x] T-047: Implement Monospace Font Size slider (8–18 pt)
  - REQs: REQ-F-009
  - Check: Slider has `from: 8; to: 18; stepSize: 1`; dragging updates `editModel.fixedFontSize`; current value displays; snaps to integers; legacy values above 18 normalize to 18 and remain dirty until Apply.

- [x] T-048: Bind all Appearance fields and verify isDirty updates
  - REQs: REQ-F-011, REQ-F-015
  - Check: Edit any field (e.g., drag a slider), then navigate to Bar page and back to Appearance — the edited field value persists; `SettingsEditModel.isDirty` is true; clicking [Discard Changes] reverts all fields to last-persisted values.

---

## Phase 6 — Bar Page

- [x] T-049: Write BarPage.qml with "General" tab label
  - REQs: REQ-F-012, REQ-F-014
  - Check: BarPage displays "General" as a tab label or section header; two sliders below.

- [x] T-050: Implement Workspace Count slider (3–10)
  - REQs: REQ-F-012
  - Check: Slider has `from: 3; to: 10; stepSize: 1`; dragging updates `editModel.workspaceCount`; current value displays; snaps to integers.

- [x] T-051: Implement System Tray Max Items slider (2–5)
  - REQs: REQ-F-012
  - Check: Slider has `from: 2; to: 5; stepSize: 1`; dragging updates `editModel.trayMaxItems`; current value displays; snaps to integers.

- [x] T-052: Bind Bar fields and verify isDirty/Discard flow
  - REQs: REQ-F-013, REQ-F-015, REQ-F-018
  - Check: Edit either slider, navigate away and back to Bar — change persists; `isDirty` is true; clicking [Discard Changes] reverts sliders to last-saved values and sets `isDirty` to false.

---

## Phase 7 — Apply Flow

- [x] T-053: Wire [Apply] button to ConfigFileService.save()
  - REQs: REQ-F-016, REQ-F-019
  - Check: FooterBar [Apply] button has `onClicked: fileService.save()`; clicking triggers `save()` method; buttons become disabled (grayed out) during the write.

- [x] T-054: Wire [Save & Apply] button (identical action to Apply)
  - REQs: REQ-F-017
  - Check: [Save & Apply] button invokes the same `fileService.save()` call; produces identical file output as [Apply].

- [x] T-055: Wire [Discard Changes] button to ConfigFileService.load()
  - REQs: REQ-F-018
  - Check: [Discard Changes] button calls `fileService.load()`; all in-memory edits are reverted to persisted values; window remains open.

- [x] T-056: Implement error dialog on save failure
  - REQs: REQ-F-020
  - Check: When `fileService.saveError(msg)` is emitted, a dialog appears with the error message and a [Retry] button; clicking Retry re-attempts the save.

- [x] T-057: Disable buttons during save and show loading feedback
  - REQs: REQ-F-019
  - Check: [Apply] and [Save & Apply] have `enabled: !fileService.isSaving` binding; buttons appear grayed out or disabled while a write is in progress; rapid double-clicks do not trigger two writes.

- [x] T-058: Display version string in footer
  - REQs: REQ-F-008
  - Check: FooterBar left side displays "Shell is running v{version}" where {version} comes from `appVersion` context property set in SettingsApplication to `PROJECT_VERSION`; version displays correctly.

- [ ] T-059: Verify shell ConfigService detects written changes
  - REQs: REQ-F-023
  - Check: Manually edit a field (e.g., workspace count to 7), click [Apply], verify config.toml is updated (via `cat ~/.config/holonight/config.toml | grep workspace`); observe shell's bar/workspace indicators update within 200ms.

---

## Phase 8 — Polish & Validation

- [x] T-060: Add i18n tr() markers to all user-facing strings
  - REQs: REQ-C-003
  - Check: All QML strings (labels, button text, error messages, placeholder text) are wrapped in `qsTr("...")` or `tr(...)`; running `lupdate` generates a `.ts` file with all strings extracted.

- [x] T-061: Run qml-lint and fix violations
  - REQs: REQ-NF-004
  - Check: `task qml-lint` on all `.qml` files in `apps/settings/qml/` returns zero errors; no warnings about unqualified access, unused imports, or type mismatches.

- [x] T-062: Run clang-format and verify C++ code style
  - REQs: REQ-NF-003
  - Check: `task format-check` on all files in `apps/settings/src/` shows no formatting violations; all files comply with `.clang-format` rules.

- [x] T-063: Run clang-tidy and fix violations
  - REQs: REQ-NF-003
  - Check: `task tidy` on `apps/settings/src/` returns zero new warnings; variable names ≥3 characters; modernize-return-braced-init-list, readability-use-anyofallof, designated initializers all pass.

- [x] T-064: Verify full project build with no errors
  - REQs: structural
  - Check: `task build` completes with exit code 0; no errors in CMake, C++ compilation, or QML loading; `build/holonight-settings` binary is created and runnable.

- [ ] T-065: Final acceptance smoke test
  - REQs: REQ-F-001, REQ-F-004, REQ-F-009, REQ-F-012, REQ-F-016, REQ-F-023, REQ-NF-001
  - Check: Launch `build/holonight-settings`, edit an Appearance field and a Bar field, click [Apply], verify config.toml updated within 2 seconds of startup; shell's visible config updates within 200ms; no crashes; startup to visible window ≤2 seconds.

---

## Summary

- **Total tasks**: 65
- **Phase 1** (Monorepo Restructure): T-001 through T-010 (10 tasks)
- **Phase 2** (Settings Binary Scaffold): T-011 through T-018 (8 tasks)
- **Phase 3** (C++ Model Layer): T-019 through T-031 (13 tasks)
- **Phase 4** (QML Skeleton): T-032 through T-039 (8 tasks)
- **Phase 5** (Appearance Page): T-040 through T-048 (9 tasks)
- **Phase 6** (Bar Page): T-049 through T-052 (4 tasks)
- **Phase 7** (Apply Flow): T-053 through T-059 (7 tasks)
- **Phase 8** (Polish & Validation): T-060 through T-065 (6 tasks)

Each task is:
- Independently completable and verifiable
- Ordered to respect dependencies (earlier phases must complete before later ones)
- Tied to one or more REQ-IDs from SPEC.md
- Marked with "structural" for tasks with no direct REQ (e.g., build integration, directory creation)
- Equipped with a single, falsifiable acceptance criterion
