# Bluetooth Settings integration

The executable owns its Bluetooth controller and KeyboardDisplay agent and consumes
the installed HoloNightSystem::Bluetooth package. It must work without shell.
Bluetooth is an immediately applied system page outside configuration Save/Apply.
Navigation and external page activation accept "bluetooth"; the generic preview is
hidden. A list/detail layout exposes adapter power/selection, alias, visibility and
scan, then device pairing, connection, alias/reset, trust, block and confirmed Forget.
Details include address/type, friendly service names and expandable raw diagnostics.

Scan only while the page is visible in a visible window. Leaving the page or closing
the window cancels its pairing workflow. A Settings-owned dialog handles all agent
interactions. Explicit Pair chains Trust then Connect and reports partial failures.
Incoming authorization alone never grants trust. Only shell requests default agent.

Reference: ../holonight-shell/docs/mockups/bluetooth-settings.png in the umbrella.
Verification includes page activation, immediate controls, scan/dismissal lifecycle,
QML imports/lint/qmltypes and standalone live pairing with headphones and a keyboard.
