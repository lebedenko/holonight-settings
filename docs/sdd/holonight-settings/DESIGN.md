# holonight-settings — Design Document

**Feature:** Standalone settings application for the HoloNight shell.
**Spec:** `docs/sdd/holonight-settings/SPEC.md`
**Status:** Design (pre-implementation)

---

## 1. Overview

`holonight-settings` is a standalone C++23/Qt6 QML application that provides a GUI for editing
`~/.config/holonight/config.toml`. It runs without a Wayland session (X11 is sufficient) and
communicates with the running shell exclusively through file writes: the shell's `ConfigService`
holds a `QFileSystemWatcher` with a 200ms debounce and picks up changes automatically. The binary
lives in `apps/settings/` inside the restructured monorepo, links only `holonight_config` and
Qt6 Core/Gui/Qml/Quick, and does not touch `holonight_services`, Wayland protocol libs, D-Bus, or
theme-token libraries.

The QML UI imports `Holonight` for `HoloniightPalette` tokens. That module is supplied by the
external `holonight-qt` install, usually under `~/.local/lib/qt6/qml/Holonight` or a system Qt QML
path. `holonight-settings` does not embed, resolve, or register a fallback palette; if the external
module is missing at runtime, QML loading should fail visibly.

---

## 2. Monorepo Restructure (scoped to this feature)

The repo root moves from a single-app layout to a multi-app monorepo. Only the directories
directly relevant to this feature are shown in full detail; other dirs are abbreviated.

```
holonight-shell/
├── apps/
│   ├── shell/                   ← existing src/ migrated here (holonight-shell binary)
│   │   ├── CMakeLists.txt
│   │   └── src/  qml/  resources/
│   └── settings/                ← NEW — this feature
│       ├── CMakeLists.txt
│       └── src/
│           ├── main.cpp
│           ├── SettingsApplication.h
│           ├── SettingsApplication.cpp
│           ├── SettingsEditModel.h
│           ├── SettingsEditModel.cpp
│           ├── ConfigFileService.h
│           ├── ConfigFileService.cpp
│           └── FontListModel.h
│               FontListModel.cpp
│       └── qml/
│           ├── qmldir
│           ├── SettingsWindow.qml
│           ├── NavPanel.qml
│           ├── ContentStack.qml
│           ├── FooterBar.qml
│           ├── AppearancePage.qml
│           └── BarPage.qml
│       └── resources/
│           └── (icons, if any)
├── libs/
│   ├── holonight-config/        ← existing include/holonight_config/ + ConfigParsers.cpp
│   │   ├── CMakeLists.txt       │  + ConfigWriter.cpp migrated here
│   │   ├── include/
│   │   │   └── holonight_config/
│   │   │       ├── config_structs.h
│   │   │       ├── config_parsers.h
│   │   │       └── config_writer.h
│   │   └── src/
│   │       ├── ConfigParsers.cpp
│   │       └── ConfigWriter.cpp
│   ├── holonight-services/      ← existing holonight_services target (shell only)
│   └── holonight-theme/         ← skeleton only this cycle
├── qml/
│   └── HoloNight/               ← shared QML components (seed from shell controls)
├── assets/
├── tests/
└── CMakeLists.txt               ← top-level, adds subdirectories
```

### Migration task order

1. **Migrate `libs/holonight-config/`** — move headers and `.cpp` files; update include paths in
   shell's `ConfigService`; verify `task build` is green before touching anything else.
2. **Migrate `apps/shell/`** — move `src/` tree; update top-level CMake; re-run full build.
3. **Write `apps/settings/`** — only after the shell build is clean so regressions are isolated.

This ordering keeps the shell always buildable; settings code never gates a shell green build.

---

## 3. Component Map

```
┌──────────────────────────────────────────────────────────────────────┐
│  holonight-settings  executable                                      │
│                                                                      │
│  C++ layer                                                           │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ SettingsApplication                                          │   │
│  │   QApplication subclass; owns model + service; registers     │   │
│  │   QML singletons; sets up QQuickView; calls load() on start  │   │
│  └──────────────┬───────────────────────────────────────────────┘   │
│                 │ owns                                               │
│  ┌──────────────▼──────────────┐  ┌───────────────────────────┐    │
│  │ SettingsEditModel            │  │ ConfigFileService          │    │
│  │  QML_SINGLETON               │  │  QObject, Q_INVOKABLE     │    │
│  │  Holds ParsedConfig in-mem   │  │  load() → parse → model   │    │
│  │  Q_PROPERTYs per MVP field   │  │  save() → ConfigWriter    │    │
│  │  isDirty computed property   │  │  emits saveFinished/Error │    │
│  └──────────────────────────────┘  └───────────────────────────┘    │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ FontListModel  (QAbstractListModel, QML_ELEMENT)             │   │
│  │   Populated once from QFontDatabase::families()              │   │
│  │   fixedPitchOnly property filters to monospace families      │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  QML layer  (module URI: HolonightSettings, prefix /HolonightSettings/) │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ SettingsWindow.qml  (QQuickView root item)                   │   │
│  │   ┌────────────┐  ┌──────────────────────────────────────┐  │   │
│  │   │ NavPanel   │  │ ContentStack (StackView / Loader)    │  │   │
│  │   │  ~200px    │  │   AppearancePage.qml                 │  │   │
│  │   │  13 items  │  │   BarPage.qml                        │  │   │
│  │   │            │  │   PlaceholderPage (deferred items)   │  │   │
│  │   └────────────┘  └──────────────────────────────────────┘  │   │
│  │   ┌──────────────────────────────────────────────────────┐  │   │
│  │   │ FooterBar  (fixed, does not scroll)                  │  │   │
│  │   │   status text  │  [Discard] [Apply] [Save & Apply]   │  │   │
│  │   └──────────────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────┘   │
└────────────────────────────┬─────────────────────────────────────────┘
                             │ links
┌────────────────────────────▼─────────────────────────────────────────┐
│  holonight_config  (static lib, libs/holonight-config/)              │
│    parseConfigTable()  ConfigWriter::write()  config_structs.h       │
│    deps: Qt6::Core, TOMLPLUSPLUS::TOMLPLUSPLUS (PUBLIC)              │
└──────────────────────────────────────────────────────────────────────┘
```

### Component responsibilities

| Component | File(s) | Responsibility |
|---|---|---|
| `SettingsApplication` | `src/SettingsApplication.{h,cpp}` | Entry point; constructs edit model, file service, shell status service; exposes settings objects to QML; creates `QQuickView`; calls `ConfigFileService::load()` before show |
| `SettingsEditModel` | `src/SettingsEditModel.{h,cpp}` | Holds a `ParsedConfig` in-memory; exposes MVP fields as `Q_PROPERTY` with notify; tracks `isDirty` vs. last-saved snapshot; `setFromParsedConfig()` bulk-sets all fields atomically |
| `ConfigFileService` | `src/ConfigFileService.{h,cpp}` | Resolves `~/.config/holonight/config.toml`; calls `parseConfigTable()` on load; calls `ConfigWriter::write()` on save; emits `saveFinished(bool)` / `saveError(QString)` |
| `FontListModel` | `src/FontListModel.{h,cpp}` | `QAbstractListModel` populated once at construction from `QFontDatabase::families()`; filtered by `QFontDatabase::isFixedPitch()` when `fixedPitchOnly` is true |
| `SettingsWindow.qml` | `qml/SettingsWindow.qml` | Root `Item` hosted by `QQuickView`; lays out `NavPanel`, `ContentStack`, `FooterBar` |
| `NavPanel.qml` | `qml/NavPanel.qml` | Fixed ~200px left column; 13 `Text`/`AbstractButton` items; emits `pageRequested(string)` to parent |
| `ContentStack.qml` | `qml/ContentStack.qml` | `StackView` or `Loader`-based switcher; maps page name to component; deferred pages show placeholder |
| `AppearancePage.qml` | `qml/AppearancePage.qml` | Swatch grid (ThemeConfig.variant), accent dots (ThemeConfig.accent), dark/light toggle (ThemeConfig.mode), UI font dropdown, monospace font dropdown, two font-size sliders |
| `BarPage.qml` | `qml/BarPage.qml` | "General" header; workspace-count slider (3–10); tray-max-items slider (2–5) |
| `FooterBar.qml` | `qml/FooterBar.qml` | Status text (left); Discard, Apply, Save & Apply buttons (right); binds button `enabled` to `!ConfigFileService.isSaving` |

---

## 4. Data Flow

### 4.1 Startup

```
main()
  └─ SettingsApplication::SettingsApplication()
       ├─ construct SettingsEditModel  (holds ParsedConfig{} defaults initially)
       ├─ construct ConfigFileService(editModel)
       ├─ expose SettingsEditModel, ConfigFileService, ShellStatusService as context properties
       ├─ set up QQuickView (source: qrc:/HolonightSettings/SettingsWindow.qml)
       │    window size: default 1200×800, minimum 1000×700
       └─ ConfigFileService::load()
            ├─ resolve path: QStandardPaths::writableLocation(AppConfigLocation)
            │    + "/holonight/config.toml"
            ├─ if file absent → use ParsedConfig{} defaults, log info
            ├─ toml::parse(file)  [try/catch; on error → defaults + qCWarning]
            ├─ parseConfigTable(table, missing)
            ├─ SettingsEditModel::setFromParsedConfig(parsed)
            └─ save snapshot_ = parsed  (baseline for isDirty)
  └─ QQuickView::show()
```

### 4.2 User edits a field

```
QML field (e.g., Slider, ComboBox)
  └─ binding: SettingsEditModel.workspaceCount = newValue
       └─ SettingsEditModel::setWorkspaceCount(int value)
            ├─ if value == workspace_count_ → return (no-op)
            ├─ workspace_count_ = value
            ├─ emit workspaceCountChanged()
            └─ recompute isDirty = (current != snapshot_)
                 └─ emit isDirtyChanged() if changed
```

All page fields follow this same setter pattern. No file I/O occurs during editing.

### 4.3 Apply / Save & Apply

Both buttons invoke the same action (the distinct label is cosmetic per REQ-F-017):

```
FooterBar: [Apply] / [Save & Apply] clicked
  └─ ConfigFileService::save()
       ├─ emit saveStarted()   (FooterBar binds buttons' enabled to !isSaving_)
       ├─ isSaving_ = true
       ├─ parsed = editModel_->toParsedConfig()
       ├─ ok = ConfigWriter::write(parsed, configPath_)  [synchronous, QSaveFile, ~1ms]
       ├─ isSaving_ = false
       ├─ if ok:
       │    snapshot_ = parsed  (isDirty resets to false)
       │    emit saveFinished(true)
       └─ if !ok:
            emit saveFinished(false)
            emit saveError(errorMessage)
            → QML shows error dialog  (edit model left unchanged, user can retry)

shell's ConfigService (separate process)
  └─ QFileSystemWatcher fires within 200ms
       └─ parseConfigTable() → applyParsedConfig() → emit signals
```

### 4.4 Discard Changes

```
FooterBar: [Discard Changes] clicked
  └─ ConfigFileService::load()   [identical to startup load path]
       └─ SettingsEditModel::setFromParsedConfig(snapshot_from_file)
            └─ all Q_PROPERTYs batch-updated, isDirty → false
```

`setFromParsedConfig` sets every field in one method call and emits signals at the end, so QML
receives a consistent batch of updates rather than individual mid-flight states.

---

## 5. SettingsEditModel — Q_PROPERTY List

File: `apps/settings/src/SettingsEditModel.h`

```cpp
class SettingsEditModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_ELEMENT

 public:
  // --- ThemeConfig ---
  Q_PROPERTY(QString themeVariant  READ themeVariant  WRITE setThemeVariant  NOTIFY themeVariantChanged)
  Q_PROPERTY(QString themeAccent   READ themeAccent   WRITE setThemeAccent   NOTIFY themeAccentChanged)
  Q_PROPERTY(QString themeMode     READ themeMode     WRITE setThemeMode     NOTIFY themeModeChanged)

  // --- AppearanceConfig (MVP subset; clock_font/title_font deferred) ---
  Q_PROPERTY(QString uiFont        READ uiFont        WRITE setUiFont        NOTIFY uiFontChanged)
  Q_PROPERTY(int     uiFontSize    READ uiFontSize    WRITE setUiFontSize    NOTIFY uiFontSizeChanged)
  Q_PROPERTY(QString fixedFont     READ fixedFont     WRITE setFixedFont     NOTIFY fixedFontChanged)
  Q_PROPERTY(int     fixedFontSize READ fixedFontSize WRITE setFixedFontSize NOTIFY fixedFontSizeChanged)

  // --- BarWorkspacesConfig ---
  Q_PROPERTY(int workspaceCount    READ workspaceCount  WRITE setWorkspaceCount  NOTIFY workspaceCountChanged)

  // --- BarSystemTrayConfig ---
  Q_PROPERTY(int trayMaxItems      READ trayMaxItems    WRITE setTrayMaxItems    NOTIFY trayMaxItemsChanged)

  // --- Edit state ---
  // true when any field differs from the last snapshot_ (i.e., last-loaded or last-saved state)
  Q_PROPERTY(bool isDirty          READ isDirty                              NOTIFY isDirtyChanged)

  explicit SettingsEditModel(QObject* parent = nullptr);

  void setFromParsedConfig(const ParsedConfig& config);
  [[nodiscard]] ParsedConfig toParsedConfig() const;

  // ... getters, setters, signals omitted for brevity

 private:
  ParsedConfig current_{};   // live edit state
  ParsedConfig snapshot_{};  // last-saved/loaded baseline for isDirty

  void recomputeDirty();
};
```

`isDirty` is computed by comparing `current_` vs `snapshot_` using `ParsedConfig`'s
`operator==` members — no manual field-by-field comparison needed (all sub-structs already
carry `= default`).

`setFromParsedConfig()` stores both `current_` and `snapshot_` from the loaded config so that
`isDirty` returns false immediately after a load or a successful save.

---

## 6. ConfigFileService

File: `apps/settings/src/ConfigFileService.{h,cpp}`

```cpp
class ConfigFileService : public QObject {
  Q_OBJECT
  QML_ELEMENT

 public:
  explicit ConfigFileService(SettingsEditModel* model, QObject* parent = nullptr);

  // Loads config.toml, populates model. Returns false on parse error (model gets defaults).
  // Safe to call multiple times (Discard flow).
  Q_INVOKABLE bool load();

  // Serializes model → ParsedConfig → ConfigWriter::write(). Synchronous.
  // Emits saveStarted() before write, saveFinished(ok) after.
  Q_INVOKABLE bool save();

  // Returns resolved absolute path (XDG_CONFIG_HOME/holonight/config.toml).
  [[nodiscard]] Q_INVOKABLE QString configPath() const;

  // True between saveStarted() and saveFinished(); QML binds button enabled to !isSaving.
  Q_PROPERTY(bool isSaving READ isSaving NOTIFY isSavingChanged)

 Q_SIGNALS:
  void saveStarted();
  void saveFinished(bool success);
  void saveError(const QString& message);
  void isSavingChanged();

 private:
  SettingsEditModel* model_{nullptr};
  QString config_path_;
  bool is_saving_{false};

  [[nodiscard]] bool isSaving() const { return is_saving_; }
};
```

**Path resolution** (in constructor): `QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)`
returns `~/.config` under XDG; append `/holonight/config.toml`. This mirrors the shell's
`ConfigService::resolveConfigPath()` but is duplicated (acceptable: settings must not link
`holonight_services`).

**TOML parse entry point**: `ConfigFileService::load()` owns the `try { toml::parse_file() }
catch (const toml::parse_error& err) { ... }` block. `parseConfigTable()` receives an
already-parsed `toml::table&` and never throws — consistent with how `ConfigService` in the shell
currently wraps the parser.

---

## 7. FontListModel

File: `apps/settings/src/FontListModel.{h,cpp}`

```cpp
class FontListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool fixedPitchOnly READ fixedPitchOnly WRITE setFixedPitchOnly
             NOTIFY fixedPitchOnlyChanged)

 public:
  enum Roles { DisplayRole = Qt::DisplayRole };
  Q_ENUM(Roles)

  explicit FontListModel(bool fixed_pitch_only = false, QObject* parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] bool fixedPitchOnly() const;
  void setFixedPitchOnly(bool value);

  // Returns the row index for a given family name, or -1.
  Q_INVOKABLE int indexOf(const QString& family) const;

 Q_SIGNALS:
  void fixedPitchOnlyChanged();

 private:
  void rebuild();

  QStringList families_;
  bool fixed_pitch_only_{false};
};
```

**Instantiation**: two instances are created in `SettingsApplication` and exposed to QML as
context properties or `QML_SINGLETON`-adjacent singletons:
- `FontListModel allFonts{false}` — for the UI font dropdown
- `FontListModel monoFonts{true}` — for the monospace font dropdown

`rebuild()` is called once at construction (and again if `fixedPitchOnly` is changed at runtime,
though the MVP does not do this). `QFontDatabase::families()` returns an alphabetically sorted list
on Qt6. `QFontDatabase::isFixedPitch(family)` is fast (no font loading, reads metrics cache).

`indexOf(family)` is used by QML `ComboBox.currentIndex: monoFonts.indexOf(SettingsEditModel.fixedFont)`
to initialize the dropdown to the saved value.

---

## 8. CMake Structure

File: `apps/settings/CMakeLists.txt`

```cmake
qt_add_executable(holonight-settings
  src/main.cpp
  src/SettingsApplication.cpp
  src/SettingsEditModel.cpp
  src/ConfigFileService.cpp
  src/FontListModel.cpp
)

set(SETTINGS_QML_FILES
  qml/SettingsWindow.qml
  qml/NavPanel.qml
  qml/ContentStack.qml
  qml/FooterBar.qml
  qml/AppearancePage.qml
  qml/BarPage.qml
)

qt_add_qml_module(holonight-settings
  URI HolonightSettings
  VERSION 1.0
  RESOURCE_PREFIX /HolonightSettings
  QML_FILES ${SETTINGS_QML_FILES}
  SOURCES
    src/SettingsEditModel.h src/SettingsEditModel.cpp
    src/ConfigFileService.h src/ConfigFileService.cpp
    src/FontListModel.h     src/FontListModel.cpp
    src/ShellStatusService.h src/ShellStatusService.cpp
)

target_link_libraries(holonight-settings
  PRIVATE
    holonight_config         # ParsedConfig, parseConfigTable, ConfigWriter
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets             # required only for QApplication (font dialog future use)
)

target_include_directories(holonight-settings
  PRIVATE src/
)
```

**QRC prefix** `/HolonightSettings/` is distinct from the shell's `/HolonightShell/` — both
resource trees can coexist in a future combined-process test harness without collision.

**`Qt6::Widgets`** is included because `QApplication` (rather than `QGuiApplication`) may be
needed for proper system font rendering on X11. If `QGuiApplication` proves sufficient, the
`Widgets` dep can be dropped — but it carries no Wayland/D-Bus transitive baggage.

**Not linked**: `holonight_services`, `holonight_platform`, `Qt6::DBus`, `Qt6::WaylandClient`,
any `wlr-layer-shell` or `ext-workspace` generated headers, or `holonight-qt` C++ libraries. The
`Holonight` QML module remains an installed runtime/tooling dependency only.

---

## 9. Key Architectural Decisions

### 9.1 Edit model: single QObject singleton vs. sub-QObjects vs. pure QML state

**Option A — single `SettingsEditModel` singleton with flat Q_PROPERTYs** (chosen)
All MVP fields live on one QObject. QML references them as
`SettingsEditModel.workspaceCount`, etc. `isDirty` is a single computed property.

**Option B — sub-QObjects** (`AppearanceEditModel`, `BarEditModel` each a QML singleton)
Cleaner domain grouping; sub-objects can be individually replaced in tests. However, each
sub-model needs its own `isDirty` or a shared coordinator, and QML must `import` more types.
The MVP has only 10 properties — premature decomposition.

**Option C — pure QML JS state**, converted to `ParsedConfig` at Apply time
Removes all C++ property boilerplate. Breaks down when: (a) non-QML code needs to read edit
state, (b) `isDirty` comparison becomes a JS deepEqual, (c) test injection is impossible.

**Decision**: Option A for MVP. When the property count exceeds ~25 or pages are added that
require independent dirty tracking, migrate to Option B by splitting per sub-struct.

### 9.2 Apply flow threading: synchronous vs. async

**Option A — synchronous `ConfigWriter::write()` on the main thread** (chosen)
`QSaveFile::commit()` on a small TOML file (~2 KB) takes under 1ms on any modern SSD. Button
disabling during write (via `isSaving`) is purely defensive — the UI will not visibly block.

**Option B — `QtConcurrent::run()` with `QFutureWatcher`**
Adds state machine complexity (pending write, cancel-on-discard edge case, concurrent double-click)
for zero user-visible benefit given the write duration.

**Decision**: Option A. Revisit only if profiling shows write latency >16ms (e.g., network FS).

### 9.3 Font filtering: unfiltered vs. fixed-pitch filter for monospace dropdown

**Option A — list all families in both dropdowns**
Simple; user can type to search. Risk: users see "Wingdings" as a UI font choice for monospace.

**Option B — filter monospace dropdown by `QFontDatabase::isFixedPitch()`** (chosen)
The monospace dropdown is specifically for `fixed_font` (terminal/code). Fixed-pitch filtering
is cheap (no font loading) and meaningful. The UI font dropdown remains unfiltered (all families).

**Decision**: Option B. `isFixedPitch()` is documented to check the metrics cache only.

### 9.4 Window type: QQuickView vs. QMainWindow

**QQuickView** (chosen): single-file root, no widget embedding, no `QMainWindow` layout manager.
The settings UI is entirely QML — there is no reason for a widget hierarchy.

**QMainWindow + `QQuickWidget`**: adds a widget dependency and an extra embedding boundary.
Useful only when mixing QWidget and QML in the same window. Not needed here.

### 9.5 Shell reload IPC: file watch vs. explicit D-Bus signal

**File watch** (chosen): zero new IPC; shell already has `QFileSystemWatcher` with 200ms debounce
via `ConfigService`. Settings writes to the same file path. No IPC protocol to design or version.

**D-Bus signal from settings to shell**: would require a named bus service in settings (violating
REQ-C-008) and a corresponding listener in the shell. REQ-F-023 explicitly names file watch as
the mechanism.

### 9.6 Config path resolution: shared helper vs. duplicated logic

The shell's `ConfigService::resolveConfigPath()` lives in `holonight_services`, which settings
must not link. The resolution logic (`QStandardPaths::AppConfigLocation + "/holonight/config.toml"`)
is trivial (two lines). Duplicating it in `ConfigFileService` is acceptable and explicitly called
out here. If it grows (e.g., `$HOLONIGHT_CONFIG` env override), extract to `holonight_config`.

---

## 10. Known Risks

### 10.1 CMake migration breakage

Moving `libs/holonight-config/` changes the `target_include_directories` path consumed by
`holonight_services` and the shell. Every `#include "ConfigParsers.h"` or `#include "ConfigService.h"`
that previously relied on transitive include propagation must be audited.

**Mitigation**: grep all `#include` lines before starting the migration:
```bash
grep -r '#include.*Config' apps/shell/src/
```
Run `task build` after each migration step (lib → shell → settings); never batch all three.

### 10.2 `holonight_services` leaking into the settings link graph

A careless `target_link_libraries(holonight-settings ... holonight_services)` — even if added
transitively by a future shared CMake utility — would pull in Wayland/D-Bus deps and violate
REQ-F-002.

**Mitigation**: `apps/settings/CMakeLists.txt` explicitly lists only `holonight_config` and
Qt6 modules. Add a CI `ldd build/holonight-settings | grep -E 'wayland|dbus'` check that
fails on unexpected symbols (similar to the shell's architecture check script).

### 10.3 Large font list degrading dropdown UX

`QFontDatabase::families()` may return 300–600 families on a typical desktop with many fonts
installed. A plain `ComboBox` with 500 items is scrollable but not searchable.

**Mitigation for MVP**: accept the unfiltered (all fonts) and filtered (fixed-pitch only) lists
as-is; the MVP acceptance checklist does not include typeahead. **Deferred**: add a
`filterString` property to `FontListModel` that applies `QString::contains(filter, Qt::CaseInsensitive)`
on the family list — a one-property addition with no API breakage.

### 10.4 `ThemeConfig.variant` is an open string; QML hardcodes swatches

`ThemeConfig` stores `variant` as a `QString` with no enum constraint. `AppearancePage.qml`
renders swatches for `"Storm"`, `"Ocean"`, `"Forest"` only. Adding a new variant in the future
requires a QML update in addition to any struct/theme changes.

**Mitigation**: comment at the swatch grid in `AppearancePage.qml`:
```qml
// NOTE: Swatch list is manually maintained. When a new ThemeConfig variant is added,
// add a corresponding swatch item here (see docs/sdd/holonight-settings/DESIGN.md §10.4).
```

### 10.5 `setFromParsedConfig` mid-flight signal cascade

`setFromParsedConfig` sets multiple Q_PROPERTYs sequentially, each emitting a `*Changed` signal.
If any QML binding reacts to an intermediate state (e.g., `uiFont` updated but `uiFontSize` not
yet), it may briefly display an inconsistent UI before the last property fires.

**Mitigation**: `setFromParsedConfig` sets all fields into `current_` directly (bypassing setters'
signal emission), then emits all `*Changed` signals in a single pass at the end. This requires
explicitly listing each `emit` but avoids cascade. Alternatively, `blockSignals(true)` / `false`
around the batch update achieves the same effect with less boilerplate — either approach is
acceptable; document the choice in the implementation.

### 10.6 `isDirty` false positive on `ParsedConfig` fields not in MVP

`toParsedConfig()` assembles a `ParsedConfig` from the MVP properties; all other fields (weather,
calendar, widgets, etc.) are populated from `snapshot_` so they round-trip unchanged. If `current_`
is naively default-initialized and `snapshot_` holds a loaded calendar config, the comparison
`current_ != snapshot_` will always be true for non-MVP fields.

**Mitigation**: `setFromParsedConfig()` must store the full `ParsedConfig` into `current_` (not
just MVP fields). `toParsedConfig()` then merges: start from `current_` (which is a copy of the
last-loaded full config), overwrite only the MVP Q_PROPERTY-backed fields, return the result.
This ensures non-MVP fields are preserved across saves and do not pollute `isDirty`.

---

## 11. File Index

| Path | Description |
|---|---|
| `apps/settings/CMakeLists.txt` | Build target, QML module, link graph |
| `apps/settings/src/main.cpp` | Entry point; constructs `SettingsApplication` |
| `apps/settings/src/SettingsApplication.{h,cpp}` | Owns all C++ objects; registers QML types; creates `QQuickView` |
| `apps/settings/src/SettingsEditModel.{h,cpp}` | In-memory edit state; all Q_PROPERTYs |
| `apps/settings/src/ConfigFileService.{h,cpp}` | File load/save; path resolution |
| `apps/settings/src/FontListModel.{h,cpp}` | Font enumeration model for QML dropdowns |
| `apps/settings/qml/SettingsWindow.qml` | Root QML item (1200×800 window) |
| `apps/settings/qml/NavPanel.qml` | Left nav; 13 section items |
| `apps/settings/qml/ContentStack.qml` | Page switcher |
| `apps/settings/qml/FooterBar.qml` | Status + 3 action buttons |
| `apps/settings/qml/AppearancePage.qml` | Theme variant/accent/mode + font controls |
| `apps/settings/qml/BarPage.qml` | Workspace count + tray max items sliders |
| `libs/holonight-config/` | Migrated from `include/holonight_config/` + `src/core/Config{Parsers,Writer}.cpp` |
