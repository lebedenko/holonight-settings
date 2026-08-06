pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Holonight
import Holonight.Core
import Holonight.Controls

import HolonightSettings

Flickable {
    id: root

    required property SettingsEditModel editModel

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

                            Slider {
                                objectName: "workspaceCountSlider"
                                from: 3
                                to: 10
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

                            Slider {
                                objectName: "trayMaxItemsSlider"
                                from: 2
                                to: 5
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

        }

    }

}

