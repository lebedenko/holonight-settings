# Settings Single-Instance Activation — Design

**Status:** Implemented and verified

## Architecture

`SettingsActivationService` is an app-local QObject and D-Bus endpoint. `SettingsApplication` constructs it immediately
after establishing application identity. Arbitration happens before configuration models, services, or the QML engine
are created.

Startup registers the object on the launch's unique bus connection, requests `org.holonight.Settings` with no queue and
no replacement, then follows one of three paths:

1. The winner queues initial platform data and continues normal application initialization.
2. A loser unregisters its local object, forwards `Activate(a{sv})` to the owner, and exits without settings state.
3. A bus or forwarding failure reports a fatal startup error and exits without settings state.

## Public D-Bus contract

- Service: `org.holonight.Settings`
- Object: `/org/holonight/Settings`
- Interface: `org.freedesktop.Application`
- Methods: `Activate(a{sv})`, `Open(as, a{sv})`, `ActivateAction(s, av, a{sv})`

`Open` and `ActivateAction` currently normalize to window activation; their URI/action arguments are intentionally
ignored because settings exposes neither document opening nor actions.

## Activation and readiness flow

Launch context maps `XDG_ACTIVATION_TOKEN` to `activation-token` and `DESKTOP_STARTUP_ID` to `desktop-startup-id`.
Incoming activation received before QML completion replaces the single pending request, since only the most recent user
intent matters. Once the root `QQuickWindow` exists, activation restores a hidden/minimized window, raises it, and calls
`requestActivate()`. The standardized values are temporarily restored to the conventional launch environment while Qt
issues the platform request. Once the window is raised and `requestActivate()` is called, if the window remains inactive after 500 ms, `QGuiApplication::alert()` triggers an urgency indicator.

All activation functionality uses standard Qt 6 public APIs, eliminating any dependency on Qt private development headers (`WaylandClientPrivate`).

The compositor decides whether this changes workspace, switches workspace, grants focus, or how urgency is rendered.

## Failure handling

Uniqueness is fail-closed: inability to connect, export, claim, or forward never falls through to constructing another
settings application. Diagnostics go to stderr and the launch returns failure. A successful forward returns success.

## Desktop and build integration

`org.holonight.Settings.desktop` declares `DBusActivatable=true`. CMake configures and installs the matching D-Bus
service file using the final install bindir and links the settings target to Qt DBus. All activation sources remain below
`apps/settings`; metadata is limited to XDG installation data.

## Extraction boundary

The movable unit is the settings executable sources, QML/resources, desktop entry, and D-Bus service template. The
activation service depends only on Qt Core, Gui, Quick, and DBus. It imports no shell services, platform library,
Wayland protocol, or compositor API.

## Alternatives considered

- Lock files/local sockets: rejected because they duplicate activation transport, complicate stale-owner recovery, and
  do not satisfy XDG D-Bus activation.
- Compositor IPC: rejected because focus/workspace handling is compositor policy and would destroy portability.
- Third-party singleton libraries: rejected because Qt DBus already supplies the small required mechanism.
- Extending shell libraries: rejected because settings must be independently extractable and must not acquire shell
  lifecycle dependencies.
