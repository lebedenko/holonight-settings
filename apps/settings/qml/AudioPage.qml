import Holonight.Core
import HolonightSettings
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    required property AudioController audioController

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        HnLabel {
            Layout.fillWidth: true
            role: HnTypographyRole.Body
            rawText: root.audioController.available ? qsTr("Audio service connected") : qsTr("Audio service unavailable")
        }

        RowLayout {
            Layout.fillWidth: true
            enabled: root.audioController.available

            HnLabel {
                Layout.fillWidth: true
                role: HnTypographyRole.Body
                rawText: qsTr("Output volume")
            }

            Controls.Slider {
                id: masterVolume
                objectName: "audioMasterVolume"
                Layout.preferredWidth: 280
                from: 0
                to: 100
                value: root.audioController.volume
                onMoved: root.audioController.setVolume(Math.round(value))
            }

            Controls.Switch {
                objectName: "audioMasterMuted"
                text: qsTr("Mute")
                checked: root.audioController.muted
                onToggled: root.audioController.setDefaultOutputMuted(checked)
            }
        }

        HnLabel {
            Layout.fillWidth: true
            role: HnTypographyRole.Title
            rawText: qsTr("Output devices")
        }

        ListView {
            objectName: "audioOutputDevices"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 180)
            clip: true
            spacing: 8
            model: root.audioController.outputs

            delegate: Controls.RadioButton {
                required property int deviceId
                required property string description
                required property bool isDefault
                width: ListView.view.width
                text: description
                checked: isDefault
                onClicked: root.audioController.setDefaultOutput(deviceId)
            }
        }

        HnLabel {
            Layout.fillWidth: true
            role: HnTypographyRole.Title
            rawText: qsTr("Input devices")
        }

        ListView {
            objectName: "audioInputDevices"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 180)
            clip: true
            spacing: 8
            model: root.audioController.inputs

            delegate: Controls.RadioButton {
                required property int deviceId
                required property string description
                required property bool isDefault
                width: ListView.view.width
                text: description
                checked: isDefault
                onClicked: root.audioController.setDefaultInput(deviceId)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
