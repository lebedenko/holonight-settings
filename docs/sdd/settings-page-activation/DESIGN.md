# Settings page activation design

Status: Implemented and verified (2026-08-12)

## Flow

`org.freedesktop.Application.ActivateAction` reaches `SettingsActivationService`, which stores an activation request
containing platform data and an optional page key. When a window is available, the service installs the activation
environment, emits `pageRequested(QString)`, and then restores, raises, and requests activation for the window.

`SettingsApplication` connects `pageRequested` to the loaded QML root before calling `setWindow()`. The connection
invokes `SettingsWindow.requestPage(pageKey)` synchronously, so an early queued request is routed before its window is
restored. `SettingsWindow` iterates `NavPanel.pages`; it changes `currentPage` only after finding a matching key.

## Ownership and failure behavior

QML owns page validation because `NavPanel.pages` is the existing navigation source of truth. Empty and unknown keys
produce no navigation state change, while the C++ service continues window activation. Generic `Activate` and `Open`
requests carry no page key and therefore emit no page request.

Only one pending request is retained. Replacing the full request keeps page intent and platform data paired and
preserves the established latest-request-wins behavior.

## Rejected alternatives

- A C++ page allowlist was rejected because it would duplicate the QML navigation model and drift as pages change.
- Encoding the page in platform data was rejected because the existing action-name field expresses the request without
  changing the D-Bus signature.
- Adding a command-line option or a private D-Bus interface was rejected as unnecessary protocol surface.
- Treating invalid keys as activation failures was rejected because activation must remain successful and compatible.
