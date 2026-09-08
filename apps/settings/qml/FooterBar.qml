pragma ComponentBehavior: Bound

import Holonight.Controls
import Holonight.Core
import HolonightSettings
import QtQuick
import QtQuick.Controls as Controls
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

            Controls.Button {
                objectName: "discardChangesButton"
                text: qsTr("Discard Changes")
                enabled: root.saveCoordinator.isDirty && !root.saveCoordinator.isBusy
                Layout.preferredHeight: 36
                onClicked: root.saveCoordinator.discard()
            }

            Controls.Button {
                objectName: "saveButton"
                text: qsTr("Save")
                enabled: root.saveCoordinator.isDirty && !root.saveCoordinator.isBusy
                highlighted: true
                Layout.preferredHeight: 36
                onClicked: root.saveCoordinator.save()
            }

        }

    }

    Controls.Dialog {
        id: errorDialog

        property string errorMessage: ""

        modal: true
        width: 440
        padding: 20
        anchors.centerIn: Controls.Overlay.overlay

        background: HnSurfaceFrame {
            surfaceRole: HnSurfaceRole.Popup
            fillColor: HoloniightPalette.surfaceRaised
            borderColor: HoloniightPalette.borderFocus
        }

        header: Item {
            implicitHeight: 56

            HnLabel {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                verticalAlignment: Text.AlignVCenter
                role: HnTypographyRole.Title
                rawText: qsTr("External Change Detected")
            }
        }

        contentItem: HnLabel {
            role: HnTypographyRole.Body
            rawText: qsTr("%1 changed outside Settings.").arg(root.saveCoordinator.conflictDomain)
            wrapMode: Text.WordWrap
        }

        footer: Item {
            implicitHeight: 64

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.bottomMargin: 12
                spacing: 8

                Item { Layout.fillWidth: true }

                Controls.Button {
                    text: qsTr("Reload")
                    onClicked: {
                        root.saveCoordinator.reloadConflict();
                        errorDialog.close();
                    }
                }

                Controls.Button {
                    text: qsTr("Overwrite")
                    highlighted: true
                    onClicked: {
                        root.saveCoordinator.overwriteConflict();
                        errorDialog.close();
                    }
                }

                Controls.Button {
                    text: qsTr("Cancel")
                    onClicked: {
                        root.saveCoordinator.cancelConflict();
                        errorDialog.close();
                    }
                }
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
