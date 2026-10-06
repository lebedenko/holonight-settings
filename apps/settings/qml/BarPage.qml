pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls

import HolonightSettings

Flickable {
    id: root

    required property ShellSettingsEditModel editModel

    readonly property real rowHorizontalPadding: 16
    readonly property real inlineControlWidth: Math.max(180, Math.min(420, (width - 80) * 0.55))

    contentHeight: contentItem.implicitHeight
    contentWidth: width
    clip: true

    component SectionGroup: ColumnLayout {
        property alias label: sectionHeader.titleText
        property alias headerObjectName: sectionHeader.objectName
        property alias frameObjectName: sectionFrame.objectName
        default property alias content: sectionRows.data
        spacing: 8

        HnSectionHeader {
            id: sectionHeader

            objectName: "barSectionHeader"
            sizeRole: HnControlSize.Compact
            isCategoryMode: true
            showPrefix: true
            dividerVisible: false
            Layout.fillWidth: true
        }

        HnSurfaceFrame {
            id: sectionFrame

            surfaceRole: HnSurfaceRole.Card
            implicitHeight: sectionRows.implicitHeight + normalizedBorderWidth * 2
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight

            ColumnLayout {
                id: sectionRows

                anchors.fill: parent
                anchors.margins: sectionFrame.normalizedBorderWidth
                spacing: 0
            }
        }
    }

    Item {
        id: contentItem

        width: root.width
        implicitHeight: content.implicitHeight + 48

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 20

            OverridePanel {
                Layout.fillWidth: true
                document: root.editModel.document
                group: "bar"
            }

            SectionGroup {
                frameObjectName: "generalSectionFrame"
                label: qsTr("General")
                Layout.fillWidth: true

                HnSettingsRow {
                    id: workspaceCountRow

                    objectName: "workspaceCountRow"
                    titleText: qsTr("Workspace Count")
                    descriptionText: qsTr("Set the total number of available workspaces")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "workspaceCountControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Controls.Slider {
                                objectName: "workspaceCountSlider"
                                from: root.editModel.document.metadata("workspaceCount").minimum
                                to: root.editModel.document.metadata("workspaceCount").maximum
                                stepSize: 1
                                value: root.editModel.workspaceCount
                                onMoved: root.editModel.workspaceCount = Math.round(value)
                                activeFocusOnTab: true
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            HnLabel {
                                objectName: "workspaceCountValue"
                                role: HnTypographyRole.Body
                                rawText: String(root.editModel.workspaceCount)
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 24
                                Layout.alignment: Qt.AlignVCenter
                            }

                        }

                    }

                }

                HnSettingsRow {
                    id: trayMaxItemsRow

                    objectName: "trayMaxItemsRow"
                    titleText: qsTr("System Tray Max Items")
                    descriptionText: qsTr("Maximum number of icons to display in the system tray")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "trayMaxItemsControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Controls.Slider {
                                objectName: "trayMaxItemsSlider"
                                from: root.editModel.document.metadata("trayMaxItems").minimum
                                to: root.editModel.document.metadata("trayMaxItems").maximum
                                stepSize: 1
                                value: root.editModel.trayMaxItems
                                onMoved: root.editModel.trayMaxItems = Math.round(value)
                                activeFocusOnTab: true
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            HnLabel {
                                objectName: "trayMaxItemsValue"
                                role: HnTypographyRole.Body
                                rawText: String(root.editModel.trayMaxItems)
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 24
                                Layout.alignment: Qt.AlignVCenter
                            }

                        }

                    }

                }

            }

            SectionGroup {
                frameObjectName: "taskbarSectionFrame"
                label: qsTr("Window management (labwc)")
                Layout.fillWidth: true
                HnSettingsRow {
                    titleText: qsTr("Taskbar")
                    descriptionText: qsTr("Show advertised windows on every bar")
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true
                    control: Component {
                        Controls.Switch {
                            objectName: "taskbarEnabledSwitch"
                            checked: root.editModel.taskbarEnabled
                            onToggled: root.editModel.taskbarEnabled = checked
                        }
                    }
                }
                HnSettingsRow {
                    titleText: qsTr("Group by application")
                    descriptionText: qsTr("Turn off to show one button per window")
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true
                    control: Component {
                        Controls.Switch {
                            objectName: "taskbarGroupedSwitch"
                            checked: root.editModel.taskbarGrouped
                            onToggled: root.editModel.taskbarGrouped = checked
                        }
                    }
                }
                HnSettingsRow {
                    titleText: qsTr("Window overview")
                    descriptionText: qsTr("Enable the overview button and optional keyboard binding")
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true
                    control: Component {
                        Controls.Switch {
                            objectName: "windowOverviewAccessSwitch"
                            checked: root.editModel.windowOverviewAccess
                            onToggled: root.editModel.windowOverviewAccess = checked
                        }
                    }
                }
                HnSettingsRow {
                    titleText: qsTr("Desktop menu")
                    descriptionText: qsTr("Let HoloNight own desktop input; disabled by default")
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true
                    control: Component {
                        Controls.Switch {
                            objectName: "desktopMenuEnabledSwitch"
                            checked: root.editModel.desktopMenuEnabled
                            onToggled: root.editModel.desktopMenuEnabled = checked
                        }
                    }
                }
            }

        }

    }

}
