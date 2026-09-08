import Holonight.Controls
import Holonight.Core
import HolonightSettings
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Flickable {
    id: root
    required property AppearanceEditModel appearanceModel
    required property SettingsSaveCoordinator saveCoordinator
    required property AppearanceAdapterClient adapter
    contentWidth: width
    contentHeight: content.implicitHeight + 32
    clip: true

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 24
        spacing: 16

        HnLabel { role: HnTypographyRole.Title; rawText: qsTr("Native Toolkit Appearance") }
        HnLabel {
            Layout.fillWidth: true
            role: HnTypographyRole.Body
            wrapMode: Text.WordWrap
            rawText: root.adapter.resultText === "" ? qsTr("Refresh to inspect native toolkit propagation.") : root.adapter.resultText
        }

        Repeater {
            model: root.adapter.outputs
            delegate: HnSurfaceFrame {
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: details.implicitHeight + 24
                surfaceRole: HnSurfaceRole.Panel
                ColumnLayout {
                    id: details
                    anchors.fill: parent
                    anchors.margins: 12
                    HnLabel { role: HnTypographyRole.Body; rawText: modelData.name }
                    HnLabel { role: HnTypographyRole.Caption; rawText: qsTr("%1 · %2").arg(modelData.status).arg(modelData.applyMode) }
                    HnLabel { visible: modelData.diagnostic !== ""; role: HnTypographyRole.Caption; rawText: modelData.diagnostic; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
            }
        }

        RowLayout {
            Controls.Button {
                objectName: "reapplyAppearanceButton"
                text: qsTr("Reapply HoloNight Appearance")
                enabled: !root.saveCoordinator.isBusy && !root.adapter.busy && !root.appearanceModel.isDirty
                onClicked: root.saveCoordinator.reapplyAppearance()
            }
            Controls.Button {
                objectName: "restoreNativeDefaultsButton"
                text: qsTr("Restore Native Toolkit Defaults")
                enabled: !root.saveCoordinator.isBusy && !root.adapter.busy && !root.appearanceModel.isDirty
                onClicked: root.saveCoordinator.restoreNativeDefaults()
            }
            Controls.Button {
                objectName: "refreshIntegrationsButton"
                text: qsTr("Refresh Status")
                enabled: !root.saveCoordinator.isBusy && !root.adapter.busy
                onClicked: root.saveCoordinator.refreshIntegrations()
            }
        }
    }
}
