import Holonight.Controls
import Holonight.Core
import HolonightSettings
import QtQuick
import QtQuick.Layouts
import "PreviewPanel"

HnApplicationWindow {
    id: root

    required property SettingsSessionContext sessionContext
    required property AppearanceEditModel appearanceModel
    required property ShellSettingsEditModel shellModel
    required property SettingsSaveCoordinator saveCoordinator
    required property AppearanceAdapterClient appearanceAdapter
    required property ShellStatusService shellStatus
    required property BluetoothController bluetoothController
    required property AudioController audioController
    required property string appVersion
    property WallpaperController wallpaperController: null
    property string currentPage: "appearance"

    onCurrentPageChanged: bluetoothController.setActive(root.visible && root.visibility !== Window.Minimized && currentPage === "bluetooth")
    onVisibleChanged: bluetoothController.setActive(root.visible && root.visibility !== Window.Minimized && currentPage === "bluetooth")
    onVisibilityChanged: bluetoothController.setActive(root.visible && root.visibility !== Window.Minimized && currentPage === "bluetooth")
    onClosing: bluetoothController.setActive(false)

    visible: true
    minimumWidth: 1244
    minimumHeight: 480
    width: 1244
    height: 800
    title: qsTr("HoloNight Settings")

    function requestPage(pageKey: string): void {
        for (const page of navPanel.pages) {
            if (page.key === pageKey) {
                root.currentPage = pageKey;
                return;
            }
        }
    }

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
                    anchors.margins: contentFrame.normalizedBorderWidth
                    appearanceModel: root.appearanceModel
                    wallpaperController: root.wallpaperController
                    sessionContext: root.sessionContext
                    shellModel: root.shellModel
                    saveCoordinator: root.saveCoordinator
                    appearanceAdapter: root.appearanceAdapter
                    bluetoothController: root.bluetoothController
                    audioController: root.audioController
                    currentPage: root.currentPage
                    currentPageTitle: navPanel.currentPageTitle
                }

            }

            HnSurfaceFrame {
                id: previewFrame
                objectName: "settingsPreviewFrame"
                visible: root.currentPage !== "bluetooth"

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
