# HoloNight Settings

For current build, test, container acceptance and provider workflows, see [DEVELOPMENT.md](DEVELOPMENT.md).

The standalone Qt 6 settings application edits shared appearance and Shell preferences. Shell owns its exported
configuration schema; the toolkit-neutral Config library owns document editing and appearance compatibility.
Each other application retains its own configuration and settings UI.

## Build

Qt >=6.11 (Core, Gui, Quick, QML, D-Bus), CMake 3.25+, Ninja, libpulse and toml++ are required.
Install Python 3 and GTest >=1.14 CONFIG with GMock for verification. Prepare the sibling Config,
System Services (with Audio), Qt (with Wayland) and Shell standalone configuration packages explicitly.

See [DEVELOPMENT.md](DEVELOPMENT.md) for current native and container commands.

Appearance persistence is provided by `HoloNight::Config`, while Shell product settings use
`HoloNightShellConfig::Config`. Settings owns only edit state and user-facing save coordination.

## Quick Controls style and discovery

Standard application controls use `import QtQuick.Controls as Controls`. Explicit `Holonight.Core` and
`Holonight.Controls` imports retain the shared design system. Run `task qml-import-check` to check this policy.
The executable embeds `:/qtquickcontrols2.conf` with `Style=Holonight`. Qt supports an environment override
(`QT_QUICK_CONTROLS_STYLE=Fusion`), `-style Fusion`, or an external `QT_QUICK_CONTROLS_CONF` file; there is no
imperative style selection.

See [DEVELOPMENT.md](DEVELOPMENT.md) for current native and container commands.

For isolated acceptance, configure with `BUILD_TESTS=ON`, a staged `CMAKE_PREFIX_PATH`, and a disposable
`CMAKE_INSTALL_PREFIX`. CTest runs the real window under HoloNight/Fusion and actual build launches in four
style-selection modes using private D-Bus sessions and temporary configuration. After installing settings and
its dependencies into the configured prefix, run `tests/check_settings_startup.py` against its `bin/holonight-settings`
and `lib/qt6/qml` in `default`, `environment`, `command-line`, and `configuration` modes.
Install at the configure-time prefix: the D-Bus activation service records that exact executable path.
See the [UQC-103 verification record](docs/sdd/unified-qtquick-controls/IMPLEMENTATION.md).

## Configuration editing

Save writes pending value patches and retains unrelated text, comments and unknown fields. Reset removes the selected
override; absent values use domain defaults. Valid appearance v1 files upgrade to sparse v2 on their first successful
save while retaining other explicit values. Unsupported versions and invalid external documents cannot be overwritten.

External changes update untouched controls. Pending edits remain visible; review conflicts per preference and either
keep the pending value or accept the external value. Save checks the latest disk values again. Discard loads the latest
valid document; a failed reload retains edits. Defaults and override status, sparse resets and diagnostics appear on
the existing Appearance, Bar and Weather pages. API key conflict values are masked.

Appearance and Shell saves report separate outcomes. Appearance is staged before native application; failures restore
the exact prior document only while both its staged revision and physical target remain current. An intervening editor
change is preserved and reported as an application failure. Cooperating writers lock the target's sibling lock file;
arbitrary editors retain a race between the final revision check and replacement. No daemon or running Shell is needed
for document editing. Files, Viewer and other applications are outside this settings migration.

Weather settings initially show only “Choose a provider…” when no provider or nonempty
legacy weather API key is saved. A saved provider remains selected even if setup is
incomplete; legacy API keys select OpenWeatherMap. Selecting a provider immediately
reveals its settings and stages the choice through Save/Discard. First selection
uses manual location unless a location source override exists. Existing units,
display preferences, credentials and location values are preserved when switching
providers. OpenWeatherMap shows its API key field; automatic location shows the
IP geolocation credentials.

## Wallpaper picker

Run `holonight-settings --wallpaper` to open the dedicated Wallpaper window.
The **Change Wallpaper** launcher, Appearance's **Change…** button, and the shell
desktop menu open the same picker. Settings and Wallpaper share one process and
close independently. Choose a folder or Favorites, select an image and display,
and press **Apply**. **Apply to all displays** keeps the shell's last-image
fallback. Closing discards unapplied assignments; added folders and favorites
persist in `$XDG_CONFIG_HOME/holonight/settings-wallpapers.ini`.

See [the feature record](docs/sdd/dedicated-wallpaper-picker.md) for activation,
conflict resolution, and display assignment behavior.

See [DEVELOPMENT.md](DEVELOPMENT.md) for current native and container commands.

Settings detects the session compositor once at startup using the installed System Services
selector. The Bar page shows **Visible workspace count** on Hyprland, Sway and labwc;
it limits the number of workspace labels shown in the bar. Taskbar, grouping, window
overview and desktop-menu controls appear only on labwc. Unknown or ambiguous sessions
show common controls only. Defaults and overrides follows the same filtering.

All sessions share the existing configuration file and schema. Hidden settings retain
their saved values and are preserved when saving other settings. Temporary compositor
IPC failures do not change which controls are available.

Bluetooth is an immediately applied device-management page, independent of Save.
It uses BlueZ through installed `HoloNightSystem::Bluetooth`, works without shell,
and owns its pairing dialog. Scanning stops when the page/window is hidden, and
leaving cancels pairing. Visibility expires after three minutes. The application
never powers adapters at startup or changes rfkill, host services, or Blueman.

## Navigation style

Choose **Settings → Keyboard → Navigation style** to use Standard (the default) or
Vim navigation in Files, the standalone file chooser, and the Bluetooth popup. The preference
applies live, independently of Shell; text editing remains conventional. Standard
keeps arrow, Home/End, Page Up/Down and Tab navigation. Vim adds h/j/k/l, gg/G,
counts, Ctrl+U/D half pages and Ctrl+H/J/K/L panel focus where supported. In file
lists h/l go to parent/open; in grids they move horizontally. `.` toggles hidden
files; Alt+D edits the location. Text fields and modal dialogs isolate navigation.

The shared preference is in `$XDG_CONFIG_HOME/holonight/config.toml`, or
`~/.config/holonight/config.toml` when XDG_CONFIG_HOME is unset:

```toml
[input]
navigation = "vim-navigation"
```

Use `navigation = "standard"` or remove the key to restore Standard. Missing files
are never created by readers. Valid replacement/deletion applies live; invalid
updates retain the last valid setting and expose reader diagnostics. Settings Save,
Discard, reset and conflict resolution apply to this preference. Edits preserve
Shell sections, comments and unknown keys; appearance stays in its own document.

For manual migration from the unreleased `input.toml` contract, copy its `profile`
value into `[input].navigation` in `config.toml`. The old `version` and `profile`
keys are no longer read. Old files are never deleted automatically. There are no
application-specific navigation overrides.

Save and Reapply recover native appearance changes, including manual font and
color edits, after checking adapter replacement support. Integrations provides
backup access and **Restore Previous Native Appearance**; later manual edits remain.
See [recoverable appearance](docs/sdd/recoverable-appearance/DESIGN.md).
