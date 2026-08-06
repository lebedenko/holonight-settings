# holonight-settings — Requirements Specification

## Purpose & Scope

`holonight-settings` is a standalone C++/QML configuration application for the HoloNight Wayland shell. It provides a graphical interface for editing persistent settings stored in `~/.config/holonight/config.toml`, with immediate application to the running shell via ConfigService's file-system watcher (200ms debounce). This document specifies the MVP feature set: Appearance and Bar (General) pages, a two-panel navigation layout, and a three-button footer apply flow (Discard, Apply, Save & Apply).

## Out of Scope (Deferred)

The following features are visible in design mockups or feasible but explicitly deferred beyond MVP:

- **Right panel (live preview)** — no visual preview of theme/appearance changes in real-time
- **Bar sub-tabs** — only "General" tab active (Height, Margins, Border-Radius, Background Style pages deferred)
- **Sidebar, Launcher, Weather, Notifications, Calendar, Audio, Workspaces, Keybindings, Integrations, Advanced, About pages** — nav items exist, content unimplemented
- **Appearance transparency & blur sliders** — no backing struct fields in `AppearanceConfig`; defer until `AppearanceConfig::transparency` and `AppearanceConfig::blur_strength` are added
- **Reset All Settings button** — UI visible in mockup, action deferred
- **Per-section export/import** — settings are file-based only; no backup/restore workflow this cycle
- **Settings sync across displays/profiles** — single-user, single-config-file scope only

## Requirements

### 1. Binary & Launch

**REQ-F-001**: The holonight-settings application shall be a standalone C++ executable (target name `holonight-settings`) that does not require a running Wayland session.

*AC:* The binary starts and displays a window (or runs headless) in a standard X11 or Wayland session without crashing on unavailable Wayland protocols or D-Bus services.

**REQ-F-002**: holonight-settings shall link only against `holonight_config` and Qt6 Core/Gui/Quick; it shall NOT link against `holonight_services`, `holonight_platform`, or Wayland protocol libraries.

*AC:* Build succeeds with `-DCMAKE_VERBOSE_MAKEFILE=ON` and `ldd build/holonight-settings` lists no `libholonight_services.so` or `wayland-*.so` symbols.

**REQ-F-003**: On startup, holonight-settings shall load `~/.config/holonight/config.toml` via `ConfigParser::parse()` into a `ParsedConfig` struct.

*AC:* If the file exists, the in-memory edit model is populated from parsed values. If missing, default values from `ParsedConfig` struct initializers are used.

**REQ-F-004**: The application window shall display with a minimum size of 1000×700 pixels and a default size of 1200×800 pixels.

*AC:* Window geometry on first launch is 1200×800. User can resize below 1000×700 (no enforced minimum after launch).

### 2. Navigation & UI Structure

**REQ-F-005**: The main window shall use a two-panel layout: a fixed-width left navigation panel (~200px) and a resizable center content panel.

*AC:* Left panel is visually distinct (background color or border) and does not scroll; center panel scrolls independently when content overflows.

**REQ-F-006**: The left navigation panel shall display a vertical list of section labels in this order:
- Appearance
- Bar
- Sidebar
- Launcher
- Weather
- Notifications
- Calendar
- Audio
- Workspaces
- Keybindings
- Integrations
- Advanced
- About

*AC:* All 13 labels are visible and clickable (or focusable). Clicking a deferred page (Sidebar onwards, except Bar) shows a "Not yet implemented" placeholder or navigates to an empty page.

**REQ-F-007**: Where a section label is clicked, the center content panel shall load that section's page.

*AC:* Appearance is initially selected on launch. Clicking Bar loads the Bar page. Visual selection indicator (highlight, underline, or color change) appears on the active label.

**REQ-F-008**: The application window shall include a fixed footer (visible at all times) with:
  - Left side: "Shell is running v{version}" text and status indicator
  - Right side: [Discard Changes] [Apply] [Save & Apply] buttons (in that order, left-to-right)

*AC:* Footer does not scroll with content. Version string is read from a version header or CMake variable. All three buttons are clickable.

### 3. Appearance Page

**REQ-F-009**: The Appearance page shall display form fields for the following `AppearanceConfig` and `ThemeConfig` members:

| Field | Control Type | Backing Struct Member | Range / Options |
|-------|--------------|----------------------|-----------------|
| Color Scheme | Swatch grid | `ThemeConfig.variant` | "Storm", "Ocean", "Forest" (and any future variants in `ThemeConfig.h`) |
| Accent Color | Color dots (6 options) | `ThemeConfig.accent` | "cyan", "purple", "green", "orange", "red", "pink" |
| Dark / Light Mode | Toggle or radio | `ThemeConfig.mode` | "dark", "light" |
| Interface Font | Dropdown | `AppearanceConfig.ui_font` | System fonts (installed and configured in system) |
| Monospace Font | Dropdown | `AppearanceConfig.fixed_font` | System fonts (installed and configured in system) |
| Interface Font Size | Slider | `AppearanceConfig.ui_font_size` | 8–18 pt |
| Monospace Font Size | Slider | `AppearanceConfig.fixed_font_size` | 8–18 pt |

*AC:* Clicking a swatch/dot/toggle updates the in-memory edit model. Font dropdowns list available system fonts. Sliders snap to integer values and display the current point size. Legacy persisted font sizes from 19–24 pt load as 18 pt and leave the edit model dirty until the user explicitly applies the migration.

**REQ-F-010**: The Appearance page font dropdowns (Interface Font, Monospace Font) shall list system fonts available on the host. Font enumeration shall use Qt's `QFontDatabase::families()`.

*AC:* Dropdown contains at least "Inter" and "JetBrains Mono" if available on the system. Selected value is readable from the dropdown after selection.

**REQ-F-011**: Changes to any Appearance page field shall update the in-memory edit model but NOT persist to disk until [Apply] or [Save & Apply] is clicked.

*AC:* Switching to Bar page and back to Appearance shows unsaved changes. Clicking [Discard Changes] reverts the fields to the last-persisted values.

### 4. Bar Page

**REQ-F-012**: The Bar page shall display form fields for the following `BarWorkspacesConfig` and `BarSystemTrayConfig` members:

| Field | Control Type | Backing Struct Member | Min–Max |
|-------|--------------|----------------------|---------|
| Workspace Count | Slider | `BarWorkspacesConfig.count` | 3–10 |
| System Tray Max Items | Slider | `BarSystemTrayConfig.max_items` | 2–5 |

*AC:* Sliders snap to integer values and display the current count. Dragging the Workspace Count slider between 3 and 10 is possible. Dragging System Tray Max Items slider between 2 and 5 is possible.

**REQ-F-013**: Changes to Bar page fields shall update the in-memory edit model but NOT persist to disk until [Apply] or [Save & Apply] is clicked.

*AC:* Switching to Appearance and back to Bar shows unsaved changes. Clicking [Discard Changes] reverts sliders to last-persisted values.

**REQ-F-014**: The Bar page shall display a "General" tab label or indicator; no other tabs (Height, Margins, Border-Radius, Background) are implemented this cycle.

*AC:* Text or UI element reading "General" is present. Clicking other tab names (if visible) shows a "Not yet implemented" placeholder.

### 5. Edit Model & Apply Flow

**REQ-F-015**: The application shall maintain an in-memory `ParsedConfig` edit model that is independent of the persisted config file.

*AC:* Changing a field updates the edit model. Clicking [Discard Changes] reloads the edit model from the persisted file without writing. Navigating between pages does not auto-save.

**REQ-F-016**: Clicking [Apply] shall call `ConfigWriter::write(editModel, "~/.config/holonight/config.toml")` and persist all in-memory changes to disk.

*AC:* After clicking [Apply], the file at `~/.config/holonight/config.toml` is updated (verified by `ls -la` or reading the file). The shell's ConfigService detects the change within 200ms and applies it.

**REQ-F-017**: Clicking [Save & Apply] shall call `ConfigWriter::write()` identically to [Apply]; the distinct label is for user clarity.

*AC:* [Save & Apply] produces the same file output as [Apply]. Both buttons have the same effect on the persisted config.

**REQ-F-018**: Clicking [Discard Changes] shall reload the edit model from the persisted config file (via `ConfigParser::parse()`) without writing and without closing the window.

*AC:* After clicking [Discard Changes], all in-memory edits are lost. Form fields display the values from the file on disk. The edit model is ready for new changes.

**REQ-F-019**: The [Apply] and [Save & Apply] buttons shall be disabled while `ConfigWriter::write()` is executing (to prevent concurrent writes).

*AC:* Clicking [Apply] rapidly twice does not trigger two writes. Button remains disabled (with visual feedback, e.g., grayed out or a spinner) during the write.

**REQ-F-020**: If `ConfigWriter::write()` encounters an error, the application shall display an error dialog and leave the edit model unchanged for user retry.

*AC:* Attempting to write to a read-only `~/.config/holonight/` directory shows an error dialog. Clicking "Retry" in the dialog re-attempts the write.

### 6. Persistence

**REQ-F-021**: All changes persisted via [Apply] or [Save & Apply] shall be written in TOML format to `~/.config/holonight/config.toml` using `ConfigWriter::write()`.

*AC:* Reading the persisted file with a TOML parser (e.g., `toml++`) deserializes the same values that were written.

**REQ-F-022**: The persisted config file shall be atomic (all-or-nothing write); if the write fails partway, the file shall remain unchanged from its prior state.

*AC:* Killing the holonight-settings process mid-write does not corrupt the config file. The file is written to a temporary location first, then renamed.

**REQ-F-023**: After [Apply] or [Save & Apply], the shell's ConfigService shall detect the file change via `QFileSystemWatcher` and apply the new configuration within 200ms.

*AC:* Changing the workspace count via holonight-settings and clicking [Apply], then observing `hyprctl dispatch workspace` or the top bar workspace indicators updates to the new count within 200ms.

### 7. Non-Functional Requirements

**REQ-NF-001**: holonight-settings shall load and display the UI within 2 seconds of binary startup on a typical desktop system.

*AC:* Startup time measured from process creation to window visibility is ≤2 seconds.

**REQ-NF-002**: Form field changes (e.g., slider drag, swatch click) shall update the UI immediately with no perceptible lag.

*AC:* Sliding the Workspace Count slider and releasing shows the new value displayed in real-time with ≤50ms latency.

**REQ-NF-003**: The application shall compile without warnings under `clang-tidy` and `clang-format` checks (`task tidy`, `task format-check`).

*AC:* Running `task tidy` in the project root on the holonight-settings target reports zero new warnings.

**REQ-NF-004**: All QML components shall pass `qmllint` validation (`task qml-lint`).

*AC:* Running `task qml-lint` on the holonight-settings QML files returns no errors.

**REQ-NF-005**: holonight-settings shall not require a running Wayland compositor and shall degrade gracefully if one is unavailable (e.g., running under X11).

*AC:* The application launches in an X11 session without crashing. Configuration is editable and persistable even without a Wayland session.

### 8. Constraints

**REQ-C-001**: holonight-settings shall use the existing `holonight_config` library for parsing and writing configuration; no duplicate parsing logic is permitted.

*AC:* Code review verifies that all config I/O flows through `ConfigParser::parse()` and `ConfigWriter::write()`.

**REQ-C-002**: Font lists shall be enumerated using Qt's `QFontDatabase::families()` API only; no hardcoded font lists are permitted.

*AC:* Source code contains no hardcoded array or list of font names (e.g., `QStringList fonts = {"Arial", "Helvetica", …}`).

**REQ-C-003**: All user-facing strings (labels, buttons, placeholders, error messages) shall be extracted into QT_TRANSLATE_NOOP or `tr()` calls for future localization, even if translation is not active this cycle.

*AC:* Running `lupdate` on the holonight-settings target generates a `.ts` file with all UI strings.

**REQ-C-004**: The application shall not poll the config file for external changes; only writes initiated by the user (via [Apply] or [Save & Apply]) shall trigger persistence.

*AC:* No `QTimer` or `QFileSystemWatcher` is used within holonight-settings to reload the config. External changes (e.g., editing `config.toml` in another process) are not reflected in holonight-settings unless the user clicks [Discard Changes] to manually reload.

**REQ-C-005**: holonight-settings shall respect the `HoloniightPalette` theme tokens (via `import Holonight`) for all UI colors and styling; no hardcoded hex color values are permitted.

*AC:* Code review verifies all QML colors reference palette tokens. A color comparison check on rendered widgets confirms they match the active HoloNight theme.

**REQ-C-006**: The CMake build integration shall add each `.qml` source file to `HOLONIGHT_QML_FILES` in `CMakeLists.txt` and prefix QRC aliases with `/HolonightSettings/` (distinct from the shell's `/HolonightShell/` prefix).

*AC:* Build command `task build` succeeds and `ldd build/holonight-settings` shows no undefined symbol errors for QML image/data resources.

**REQ-C-007**: The QML module URI shall be `HolonightSettings` (not `HolonightShell`); all QML imports use `import HolonightSettings`.

*AC:* Running `qmldir` lookup or `qml` debugging on the holonight-settings process lists the module as `HolonightSettings`.

**REQ-C-008**: No D-Bus introspection, service registration, or IPC shall occur within holonight-settings; all communication with the shell is mediated implicitly via config file writes and the shell's ConfigService watcher.

*AC:* `dbus-monitor` or `gdbus intrect` shows no new service or method calls from the holonight-settings process.

**REQ-C-009**: The application window shall be a standard QQuickView (or QMainWindow with QML view); no custom OpenGL rendering or platform-specific window role bindings are permitted.

*AC:* Running under a window manager shows the window as a standard application window (no special layer-shell, overlay, or subsurface role).

---

## Acceptance Checklist (MVP Completion)

- [ ] Binary `holonight-settings` builds and runs standalone
- [ ] Two-panel layout (left nav, center content) displays correctly
- [ ] All 13 nav labels are visible and selectable
- [ ] Appearance page loads all 7 fields (scheme, accent, mode, ui font, mono font, ui size, mono size)
- [ ] Bar page loads 2 sliders (workspace count, tray max items)
- [ ] Unsaved changes can be discarded and changes revert
- [ ] [Apply] and [Save & Apply] write to config.toml via `ConfigWriter::write()`
- [ ] Shell's ConfigService detects write and applies within 200ms (manual test)
- [ ] No `holonight_services`, Wayland, or D-Bus deps in link graph
- [ ] `task tidy`, `task format-check`, `task qml-lint` all pass
- [ ] Version string displays in footer
- [ ] Buttons are disabled during write
- [ ] Error dialog appears if write fails (e.g., read-only directory)
