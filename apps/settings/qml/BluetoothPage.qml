import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import Holonight.Core
import HolonightSettings

Item {
    id: root
    required property BluetoothController bluetoothController
    property string selectedDeviceId: ""
    readonly property var selected: bluetoothController.devices.find(device => device.id === root.selectedDeviceId) || ({})

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Controls.Switch {
                objectName: "bluetoothPower"
                text: qsTr("Bluetooth")
                enabled: root.bluetoothController.adapters.length > 0 && !root.bluetoothController.selectedAdapter.busy
                checked: root.bluetoothController.selectedAdapter.powered || false
                onToggled: root.bluetoothController.power(checked)
            }
            Controls.ComboBox {
                objectName: "bluetoothAdapterSelector"
                visible: root.bluetoothController.adapters.length > 1
                model: root.bluetoothController.adapters
                textRole: "alias"
                valueRole: "id"
                currentIndex: indexOfValue(root.bluetoothController.selectedId)
                onActivated: root.bluetoothController.selectAdapter(currentValue)
            }
            Controls.Button {
                text: root.bluetoothController.scanning ? qsTr("Stop scan") : qsTr("Start scan")
                enabled: root.bluetoothController.selectedAdapter.powered || false
                onClicked: root.bluetoothController.scan(!root.bluetoothController.scanning)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            enabled: root.bluetoothController.adapters.length > 0 && !root.bluetoothController.selectedAdapter.busy
            HnLabel {
                rawText: qsTr("Computer name")
            }
            Controls.TextField {
                Layout.fillWidth: true
                text: root.bluetoothController.selectedAdapter.alias || ""
                onEditingFinished: root.bluetoothController.renameAdapter(text)
            }
            Controls.Switch {
                text: qsTr("Visible for 3 minutes")
                checked: root.bluetoothController.selectedAdapter.discoverable || false
                onToggled: root.bluetoothController.discoverable(checked)
            }
        }
        HnLabel {
            Layout.fillWidth: true
            visible: rawText.length > 0
            wrapMode: Text.WordWrap
            rawText: !root.bluetoothController.available ? qsTr("Bluetooth service unavailable") : root.bluetoothController.adapters.length === 0 ? qsTr("No Bluetooth adapter") : root.bluetoothController.errorMessage
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Controls.ScrollView {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                ColumnLayout {
                    width: parent.width
                    Repeater {
                        model: [qsTr("Connected"), qsTr("Paired"), qsTr("Available")]
                        delegate: ColumnLayout {
                            id: section
                            required property string modelData
                            Layout.fillWidth: true

                            function syncDevices() {
                                const desired = root.bluetoothController.devices.filter(device => device.section === section.modelData);
                                for (let i = deviceRows.count - 1; i >= 0; --i) {
                                    if (!desired.some(device => device.id === deviceRows.get(i).record.id))
                                        deviceRows.remove(i);
                                }
                                for (let i = 0; i < desired.length; ++i) {
                                    let found = -1;
                                    for (let j = i; j < deviceRows.count; ++j)
                                        if (deviceRows.get(j).record.id === desired[i].id) {
                                            found = j;
                                            break;
                                        }
                                    if (found < 0)
                                        deviceRows.insert(i, {
                                            record: desired[i]
                                        });
                                    else {
                                        if (found !== i)
                                            deviceRows.move(found, i, 1);
                                        deviceRows.set(i, {
                                            record: desired[i]
                                        });
                                    }
                                }
                            }
                            ListModel {
                                id: deviceRows
                                dynamicRoles: true
                            }
                            Component.onCompleted: syncDevices()
                            Connections {
                                target: root.bluetoothController
                                function onChanged() {
                                    section.syncDevices();
                                }
                            }
                            HnLabel {
                                rawText: section.modelData
                                role: HnTypographyRole.Title
                            }
                            Repeater {
                                model: deviceRows
                                delegate: Controls.ItemDelegate {
                                    required property var record
                                    Layout.fillWidth: true
                                    icon.name: record.icon
                                    text: record.name + (record.busy ? qsTr(" · Working…") : "")
                                    highlighted: record.id === root.selectedDeviceId
                                    onClicked: root.selectedDeviceId = record.id
                                }
                            }
                        }
                    }
                }
            }
            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    width: parent.width
                    visible: !!root.selected.id
                    spacing: 12
                    HnLabel {
                        Layout.fillWidth: true
                        role: HnTypographyRole.Title
                        rawText: root.selected.name || ""
                    }
                    HnLabel {
                        rawText: root.selected.address || ""
                    }
                    HnLabel {
                        rawText: qsTr("Address type: %1").arg(root.selected.addressType || "")
                    }
                    HnLabel {
                        visible: root.selected.battery !== undefined && root.selected.battery !== null
                        rawText: qsTr("Battery: %1%").arg(root.selected.battery || 0)
                    }
                    Controls.Button {
                        objectName: "bluetoothDeviceAction"
                        enabled: !root.selected.busy
                        text: root.selected.connected ? qsTr("Disconnect") : root.selected.paired ? qsTr("Connect") : qsTr("Pair")
                        onClicked: root.bluetoothController.action(root.selected.id, root.selected.connected ? "disconnect" : root.selected.paired ? "connect" : "pair")
                    }
                    RowLayout {
                        Controls.TextField {
                            id: alias
                            text: root.selected.alias || ""
                            placeholderText: qsTr("Device name")
                        }
                        Controls.Button {
                            text: qsTr("Rename")
                            enabled: !root.selected.busy
                            onClicked: root.bluetoothController.action(root.selected.id, "rename", alias.text)
                        }
                        Controls.Button {
                            text: qsTr("Reset")
                            enabled: !root.selected.busy
                            onClicked: root.bluetoothController.action(root.selected.id, "rename", "")
                        }
                    }
                    Controls.Switch {
                        text: qsTr("Trusted")
                        enabled: !root.selected.busy
                        checked: root.selected.trusted || false
                        onToggled: root.bluetoothController.action(root.selected.id, "trust", checked)
                    }
                    Controls.Switch {
                        text: qsTr("Blocked")
                        enabled: !root.selected.busy
                        checked: root.selected.blocked || false
                        onToggled: root.bluetoothController.action(root.selected.id, "block", checked)
                    }
                    Controls.Button {
                        text: qsTr("Forget device…")
                        enabled: !root.selected.busy
                        onClicked: {
                            forget.targetId = root.selected.id;
                            forget.open();
                        }
                    }
                    HnLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        rawText: qsTr("Services: %1").arg((root.selected.services || []).join(", "))
                    }
                    Controls.CheckBox {
                        id: advanced
                        text: qsTr("Advanced information")
                    }
                    HnLabel {
                        Layout.fillWidth: true
                        visible: advanced.checked
                        wrapMode: Text.WrapAnywhere
                        rawText: qsTr("UUIDs: %1\nClass: %2 · Appearance: %3\nRSSI: %4 · Bonded: %5").arg((root.selected.uuids || []).join("\n")).arg(root.selected.deviceClass || 0).arg(root.selected.appearance || 0).arg(root.selected.rssi ?? qsTr("Unknown")).arg(root.selected.bonded || false)
                    }
                }
            }
        }
    }
    Controls.Dialog {
        id: forget
        property string targetId: ""
        title: qsTr("Forget this Bluetooth device?")
        modal: true
        anchors.centerIn: parent
        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
        onAccepted: root.bluetoothController.action(targetId, "forget")
    }
    Controls.Dialog {
        id: pairing
        title: qsTr("Bluetooth pairing")
        modal: true
        anchors.centerIn: parent
        width: Math.min(root.width, 420)
        closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
        onClosed: if (root.bluetoothController.interaction.id)
            root.bluetoothController.cancelPairing()
        contentItem: BluetoothPairing {
            controller: root.bluetoothController
        }
    }
    Connections {
        target: root.bluetoothController
        function onPairingRequested() {
            pairing.open();
        }
        function onChanged() {
            if (!root.bluetoothController.interaction.id)
                pairing.close();
        }
    }
}
