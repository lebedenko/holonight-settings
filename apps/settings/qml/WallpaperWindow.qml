pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls
import HolonightSettings

HnApplicationWindow {
    id: root
    required property WallpaperController controller
    required property WallpaperModel browser
    required property WallpaperFilter wallpapers
    readonly property var details: {
        browser.loading;
        return browser.details(controller.selectedPath);
    }
    visible: true
    title: qsTr("HoloNight Wallpaper")
    width: 1240
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    onClosing: {
        folderChooser.cancel();
        controller.discard();
    }

    PortalFolderChooser {
        id: folderChooser
        objectName: "wallpaperFolderChooser"
        onFolderSelected: folder => root.browser.addFolder(folder)
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        HnSurfaceFrame {
            surfaceRole: HnSurfaceRole.Panel
            chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomRight
            fillColor: HoloniightPalette.surfaceRaised
            borderColor: HoloniightPalette.borderPassive
            Layout.minimumWidth: 200
            Layout.preferredWidth: Math.max(Layout.minimumWidth, navigation.minimumContentWidth)
            Layout.fillHeight: true
            NavPanel {
                id: navigation
                objectName: "wallpaperNavigation"
                anchors.fill: parent
                applicationName: qsTr("Wallpapers")
                currentPage: root.wallpapers.collection
                pages: [
                    {
                        key: "",
                        label: qsTr("All Wallpapers")
                    },
                    {
                        key: "favorites",
                        label: qsTr("Favorites")
                    },
                    ...root.browser.folders.map(folder => ({
                                key: folder,
                                label: folder.endsWith("/wallpapers/holonight") ? qsTr("HoloNight") : folder.split("/").pop()
                            })),
                    {
                        key: "add-folder",
                        label: qsTr("Add Folder…")
                    }
                ]
                onPageRequested: key => {
                    if (key === "add-folder")
                        folderChooser.open();
                    else
                        root.wallpapers.collection = key;
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            HnLabel {
                rawText: qsTr("Choose a wallpaper")
                role: HnTypographyRole.Heading
                Layout.fillWidth: true
            }
            HnLoadingState {
                titleText: qsTr("Loading wallpapers")
                running: root.browser.loading
                visible: running
                Layout.fillWidth: true
            }
            HnLabel {
                rawText: folderChooser.error
                visible: rawText !== ""
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            HnLabel {
                rawText: root.browser.error
                visible: rawText !== ""
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            GridView {
                id: grid
                objectName: "wallpaperGrid"
                Layout.fillWidth: true
                Layout.fillHeight: true
                cellWidth: width / Math.max(1, Math.floor(width / 180))
                cellHeight: 146
                clip: true
                model: root.wallpapers
                keyNavigationEnabled: true
                activeFocusOnTab: true
                Keys.onReturnPressed: {
                    const tile = currentItem as Controls.ItemDelegate;
                    if (tile)
                        tile.clicked();
                }
                delegate: HnThumbnailDelegate {
                    id: tile
                    required property string imagePath
                    required property string imageTitle
                    required property url thumbnail
                    required property bool favorite
                    required property int index
                    width: grid.cellWidth - 8
                    height: grid.cellHeight - 8
                    highlighted: root.controller.selectedPath === imagePath
                    caption: imageTitle
                    showSelectionMarker: true
                    onClicked: {
                        grid.currentIndex = index;
                        root.controller.select(imagePath);
                    }
                    thumbnailContent: Image {
                        source: tile.thumbnail
                        sourceSize: Qt.size(320, 200)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        HnLabel {
                            anchors.centerIn: parent
                            visible: parent.status === Image.Error
                            rawText: qsTr("Image unavailable")
                        }
                    }
                }
                HnEmptyState {
                    anchors.centerIn: parent
                    visible: grid.count === 0 && !root.browser.loading
                    titleText: root.wallpapers.collection === "favorites" ? qsTr("No favorites yet") : qsTr("No wallpapers found. Add a folder to begin.")
                    width: parent.width
                }
            }
        }
        HnSurfaceFrame {
            surfaceRole: HnSurfaceRole.Panel
            fillColor: HoloniightPalette.surfaceRaised
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                HnLabel {
                    rawText: qsTr("Preview")
                    role: HnTypographyRole.Heading
                    Layout.fillWidth: true
                }
                HnPreviewFrame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    // Keep the image loaded during loading/error presentation.
                    Image {
                        id: wallpaperImage
                        objectName: "wallpaperPreviewImage"
                        anchors.fill: parent
                        anchors.margins: parent.padding
                        source: root.controller.selectedImage
                        sourceSize: Qt.size(720, 440)
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                    }
                    previewState: wallpaperImage.status === Image.Null ? HnPreviewFrame.Empty : wallpaperImage.status === Image.Loading ? HnPreviewFrame.Loading : wallpaperImage.status === Image.Error ? HnPreviewFrame.Error : HnPreviewFrame.Content
                    loadingContent: Item {
                        Controls.BusyIndicator {
                            anchors.centerIn: parent
                            running: true
                            Accessible.name: qsTr("Loading preview")
                        }
                    }
                    errorContent: Item {
                        Accessible.name: qsTr("No preview available")
                        Accessible.description: qsTr("Image cannot be loaded")
                        HoverHandler {
                            id: wallpaperFailureHover
                        }
                        Controls.ToolTip.visible: wallpaperFailureHover.hovered
                        Controls.ToolTip.text: qsTr("Image cannot be loaded")
                        ColumnLayout {
                            anchors.centerIn: parent
                            width: parent.width
                            HnIcon {
                                id: wallpaperFileIcon
                                Layout.alignment: Qt.AlignHCenter
                                size: 64
                                name: "image-x-generic"
                                visible: !hasError
                            }
                            HnIcon {
                                Layout.alignment: Qt.AlignHCenter
                                size: 64
                                visible: wallpaperFileIcon.hasError
                                source: "qrc:/qt/qml/Holonight/Controls/assets/generic-file.svg"
                            }
                            HnLabel {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                role: HnTypographyRole.Caption
                                color: HoloniightPalette.textMuted
                                rawText: qsTr("No preview available")
                            }
                        }
                    }
                }
                HnLabel {
                    rawText: root.details.title || root.controller.selectedPath.split("/").pop()
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                HnLabel {
                    rawText: root.details.dimensions ? qsTr("%1 · %2 MB").arg(root.details.dimensions).arg((root.details.bytes / 1048576).toFixed(1)) : ""
                    Layout.fillWidth: true
                }
                Controls.Button {
                    text: root.details.favorite ? qsTr("Remove from Favorites") : qsTr("Add to Favorites")
                    enabled: root.controller.selectedPath !== ""
                    Layout.fillWidth: true
                    onClicked: root.browser.toggleFavorite(root.controller.selectedPath)
                }
                HnSettingsRow {
                    titleText: qsTr("Display")
                    stacked: true
                    Layout.fillWidth: true
                    Controls.ComboBox {
                        objectName: "wallpaperDisplay"
                        model: root.controller.displays.map((name, index) => name || qsTr("Display %1").arg(index + 1))
                        Accessible.name: qsTr("Target display")
                        currentIndex: root.controller.target
                        Layout.fillWidth: true
                        onActivated: root.controller.target = currentIndex
                    }
                }
                HnSettingsRow {
                    titleText: qsTr("Apply to all displays")
                    Layout.fillWidth: true
                    Controls.CheckBox {
                        Accessible.name: qsTr("Apply to all displays")
                        checked: root.controller.allDisplays
                        onToggled: root.controller.allDisplays = checked
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                HnLabel {
                    rawText: root.controller.error
                    visible: rawText !== ""
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                HnLabel {
                    rawText: qsTr("Wallpaper configuration changed externally.")
                    visible: root.controller.conflict
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                RowLayout {
                    visible: root.controller.conflict
                    Controls.Button {
                        text: qsTr("Reload")
                        onClicked: root.controller.resolve(false)
                    }
                    Controls.Button {
                        text: qsTr("Keep pending")
                        onClicked: root.controller.resolve(true)
                    }
                }
                HnActionBar {
                    Layout.fillWidth: true
                    trailingContent: Controls.Button {
                        objectName: "wallpaperApply"
                        text: qsTr("Apply")
                        enabled: root.controller.canApply
                        Layout.fillWidth: true
                        onClicked: root.controller.apply()
                    }
                }
            }
        }
    }
}
