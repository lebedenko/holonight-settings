pragma ComponentBehavior: Bound

import Holonight as HnStyle
import Holonight.Controls
import Holonight.Core
import HolonightSettings
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root

    required property SettingsSaveCoordinator saveCoordinator
    required property ShellStatusService shellStatus
    required property string appVersion

    implicitHeight: 56
    color: HoloniightPalette.surfaceRaised

    HnActionBar {
        objectName: "footerActionBar"
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        dividerVisible: false

        leadingContent: RowLayout {
            spacing: 10

            HnStatusIndicator {
                objectName: "shellStatusIndicator"
                Layout.preferredHeight: 36
                status: root.shellStatus.shellRunning ? HnStatusIndicator.Success : HnStatusIndicator.Warning
                text: root.shellStatus.statusText
            }

            HnLabel {
                role: HnTypographyRole.Caption
                rawText: qsTr("v%1").arg(root.appVersion)
            }

            HnLabel {
                objectName: "saveResultText"
                role: HnTypographyRole.Caption
                rawText: root.saveCoordinator.resultText
            }

        }

        trailingContent: RowLayout {
            spacing: 8

            HnStyle.Button {
                objectName: "discardChangesButton"
                text: qsTr("Discard Changes")
                enabled: root.saveCoordinator.isDirty && !root.saveCoordinator.isBusy
                Layout.preferredHeight: 36
                onClicked: root.saveCoordinator.discard()
            }

            HnStyle.Button {
                objectName: "saveButton"
                text: qsTr("Save")
                enabled: root.saveCoordinator.isDirty && !root.saveCoordinator.isBusy
                highlighted: true
                Layout.preferredHeight: 36
                onClicked: root.saveCoordinator.save()
            }

        }

    }

    Dialog {
        id: errorDialog

        property string errorMessage: ""

        title: qsTr("External Change Detected")
        modal: true
        anchors.centerIn: Overlay.overlay

        HnLabel {
            role: HnTypographyRole.Body
            rawText: qsTr("%1 changed outside Settings.").arg(root.saveCoordinator.conflictDomain)
            wrapMode: Text.WordWrap
            width: 360
        }

        footer: DialogButtonBox {
            HnStyle.Button {
                text: qsTr("Reload")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: root.saveCoordinator.reloadConflict()
            }

            HnStyle.Button {
                text: qsTr("Overwrite")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: root.saveCoordinator.overwriteConflict()
            }

            HnStyle.Button {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: root.saveCoordinator.cancelConflict()
            }

        }

    }

    Connections {
        function onConflictDomainChanged() {
            if (root.saveCoordinator.conflictDomain !== "")
                errorDialog.open();
        }

        target: root.saveCoordinator
    }

}
