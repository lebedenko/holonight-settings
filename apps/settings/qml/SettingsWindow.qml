import Holonight.Controls
import Holonight.Core
import HolonightSettings
import QtQuick
import QtQuick.Layouts
import "PreviewPanel"

HnApplicationWindow {
    id: root

    required property AppearanceEditModel appearanceModel
    required property ShellSettingsEditModel shellModel
    required property SettingsSaveCoordinator saveCoordinator
    required property ShellStatusService shellStatus
    required property string appVersion
    property string currentPage: "appearance"

    visible: true
    minimumWidth: 1244
    minimumHeight: 480
    width: 1244
    height: 800
    title: qsTr("HoloNight Settings")

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 8
            Layout.topMargin: 8
            Layout.rightMargin: 8
            spacing: 8

            HnSurfaceFrame {
                id: navFrame

                objectName: "settingsNavFrame"
                surfaceRole: HnSurfaceRole.Panel
                chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomRight
                fillColor: HoloniightPalette.surfaceRaised
                borderColor: HoloniightPalette.borderPassive
                Layout.minimumWidth: 280
                Layout.preferredWidth: Math.max(Layout.minimumWidth, navPanel.minimumContentWidth)
                Layout.fillWidth: false
                Layout.fillHeight: true

                NavPanel {
                    id: navPanel

                    objectName: "settingsNavPanel"
                    anchors.fill: parent
                    currentPage: root.currentPage
                    onPageRequested: (key) => {
                        if (key === root.currentPage)
                            return ;

                        root.currentPage = key;
                    }
                }

            }

            HnSurfaceFrame {
                id: contentFrame

                surfaceRole: HnSurfaceRole.Window
                fillColor: HoloniightPalette.surface
                borderColor: HoloniightPalette.borderPassive
                Layout.fillWidth: true
                Layout.fillHeight: true

                ContentStack {
                    anchors.fill: parent
                    appearanceModel: root.appearanceModel
                    shellModel: root.shellModel
                    currentPage: root.currentPage
                    currentPageTitle: navPanel.currentPageTitle
                }

            }

            HnSurfaceFrame {
                id: previewFrame

                surfaceRole: HnSurfaceRole.Panel
                chamferedCornersOverride: HnCornerMask.TopLeft | HnCornerMask.BottomLeft
                fillColor: HoloniightPalette.surfaceRaised
                borderColor: HoloniightPalette.borderPassive
                Layout.preferredWidth: 320
                Layout.fillWidth: false
                Layout.fillHeight: true

                PreviewPanel {
                    anchors.fill: parent
                }

            }

        }

        FooterBar {
            saveCoordinator: root.saveCoordinator
            shellStatus: root.shellStatus
            appVersion: root.appVersion
            Layout.fillWidth: true
        }

    }

}
