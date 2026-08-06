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

    required property SettingsEditModel editModel
    required property ConfigFileService fileService
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

        }

        trailingContent: RowLayout {
            spacing: 8

            HnStyle.Button {
                objectName: "discardChangesButton"
                text: qsTr("Discard Changes")
                enabled: root.editModel.isDirty && !root.fileService.isSaving
                Layout.preferredHeight: 36
                onClicked: root.fileService.load()
            }

            HnStyle.Button {
                objectName: "applyButton"
                text: qsTr("Apply")
                enabled: root.editModel.isDirty && !root.fileService.isSaving
                highlighted: true
                Layout.preferredHeight: 36
                onClicked: root.fileService.save()
            }

            HnStyle.Button {
                objectName: "saveAndApplyButton"
                text: qsTr("Save & Apply")
                enabled: root.editModel.isDirty && !root.fileService.isSaving
                highlighted: true
                Layout.preferredHeight: 36
                onClicked: root.fileService.save()
            }

        }

    }

    Dialog {
        id: errorDialog

        property string errorMessage: ""

        title: qsTr("Save Failed")
        modal: true
        anchors.centerIn: Overlay.overlay

        HnLabel {
            role: HnTypographyRole.Body
            rawText: errorDialog.errorMessage
            wrapMode: Text.WordWrap
            width: 360
        }

        footer: DialogButtonBox {
            HnStyle.Button {
                text: qsTr("Retry")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: root.fileService.save()
            }

            HnStyle.Button {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }

        }

    }

    Connections {
        function onSaveError(message) {
            errorDialog.errorMessage = message;
            errorDialog.open();
        }

        target: root.fileService
    }

}
