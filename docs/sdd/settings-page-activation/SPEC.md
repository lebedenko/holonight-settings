# Settings page activation specification

Status: Implemented and verified (2026-08-12)

## Scope

Settings supports page-aware activation through its existing `org.freedesktop.Application` D-Bus endpoint. This
change does not add a command-line option, a Network page, or shell-side integration.

## Requirements

- `Activate(a{sv})` shall raise/restore settings without changing the selected page.
- `Open(as, a{sv})` shall remain page-neutral and ignore its URI list.
- `ActivateAction(s, av, a{sv})` shall interpret its action name as a page key, ignore its parameter list, raise/restore
  settings, and select the page when the key exists in the current navigation model.
- Empty or unknown page keys shall still activate the window and shall leave the current/default page unchanged.
- Activation received before QML is attached shall retain its platform data and optional page key. The latest pending
  request shall replace earlier pending requests.
- The supported keys are exactly the keys already exposed by `NavPanel.pages`, including the existing `audio`
  placeholder.
- The service name, object path, interface, method signatures, and generic activation behavior shall remain compatible.

## Acceptance

Automated coverage verifies immediate and queued page activation, page-neutral methods, platform-data preservation,
latest-request-wins behavior, runtime D-Bus delivery, and QML validation against the navigation model.
