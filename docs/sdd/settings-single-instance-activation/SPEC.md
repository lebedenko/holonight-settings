# Settings Single-Instance Activation — Specification

**Status:** Implemented and verified

## Scope

This cycle gives the standalone settings application one stable session identity and forwards repeat launches to its
existing window. It uses session D-Bus and XDG activation only. Repository extraction is deferred.

## Functional requirements

### REQ-F-001 — Singleton ownership

When settings starts in a graphical user session, it shall claim `org.holonight.Settings` before loading configuration
or QML. If another owner exists, it shall not construct settings state.

**Acceptance:** Two launches leave one settings owner/process; instrumentation or a lifecycle test proves the losing
launch does not load configuration or QML.

### REQ-F-002 — Secondary-launch forwarding

When a launch loses ownership, it shall call `Activate(a{sv})` on the existing owner and exit successfully.

**Acceptance:** An integration test starts two independent bus connections and observes the second arbitration result
as secondary and the existing window receiving activation.

### REQ-F-003 — Standard activation data

When `XDG_ACTIVATION_TOKEN` or `DESKTOP_STARTUP_ID` is supplied, the launch shall forward it under the standardized
`activation-token` or `desktop-startup-id` platform-data key respectively.

**Acceptance:** Automated tests set each environment value and compare the generated `a{sv}` map exactly.

### REQ-F-004 — Window restoration

When activation is received after window creation, settings shall show/restore, raise, and request activation for the
existing `QQuickWindow`.

**Acceptance:** An offscreen window hidden before activation becomes visible and non-minimized; live checks cover
minimized and other-workspace windows.

### REQ-F-005 — Early activation

When activation arrives before the window is ready, the latest activation data shall be queued and applied when the
window is attached.

**Acceptance:** A unit test invokes activation before `setWindow()` and observes restoration afterward.

### REQ-F-006 — Attention fallback

If the restored window remains inactive after a short grace period, settings shall request user attention.

**Acceptance:** Code/test instrumentation confirms the attention request occurs only when the window is visible and
still inactive after the grace period.

### REQ-F-007 — Failure behavior

If the session bus is unavailable, ownership cannot be determined, or forwarding fails, the new launch shall exit with
failure and shall not create a duplicate settings instance.

**Acceptance:** Fault-injection tests cover disconnected bus and forwarding errors and observe an error result with no
settings model or QML engine construction.

### REQ-F-008 — Desktop integration

The installed desktop ID shall be `org.holonight.Settings.desktop`, declare `DBusActivatable=true`, and have a matching
installable `org.holonight.Settings.service` whose executable starts `holonight-settings`.

**Acceptance:** Configure/install metadata inspection confirms matching desktop, bus, object, and executable identity.

## Non-functional requirements

### REQ-NF-001 — Extraction readiness

Activation implementation and metadata shall be local to `apps/settings` and directly movable to a standalone settings
repository without shell, Hyprland, Wayland-protocol, or internal platform-library code.

**Acceptance:** Source/link review finds no shell, Hyprland, or internal platform-library dependency in activation code.

### REQ-NF-002 — Portable behavior

The implementation shall use `org.freedesktop.Application` and public Qt/session-D-Bus mechanisms, with no
window-manager IPC.

**Acceptance:** Architecture check and source search find no compositor API use in the settings activation path.

## Constraints

### REQ-C-001 — Stable public interface

The service shall be `org.holonight.Settings`, object `/org/holonight/Settings`, interface
`org.freedesktop.Application`, exporting `Activate(a{sv})`, `Open(as, a{sv})`, and
`ActivateAction(s, av, a{sv})`.

**Acceptance:** Runtime D-Bus introspection lists all three methods with their exact signatures.

### REQ-C-002 — Compositor policy boundary

Workspace movement, workspace switching, keyboard focus, and urgency presentation are compositor policy. Settings
shall request activation/attention but shall not guarantee or implement these outcomes itself.

**Acceptance:** Requirements and implementation contain no portable guarantee or compositor-specific control for
those outcomes; live results are recorded as observations only.

### REQ-C-003 — Preserve prior SDD

The older `docs/sdd/holonight-settings/` cycle shall remain unchanged.

**Acceptance:** Git diff contains no path below that directory.
