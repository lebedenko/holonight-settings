pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls

Item {
    id: root

    required property string currentPage

    property string applicationName: qsTr("Settings")

    signal pageRequested(string pageKey)

    readonly property var pages: [
        {key: "appearance",    label: qsTr("Appearance"),    icon: "appearance"},
        {key: "bar",           label: qsTr("Bar"),           icon: "bar"},
        {key: "sidebar",       label: qsTr("Sidebar"),       icon: "sidebar"},
        {key: "launcher",      label: qsTr("Launcher"),      icon: "launcher"},
        {key: "weather",       label: qsTr("Weather"),       icon: "weather"},
        {key: "notifications", label: qsTr("Notifications"), icon: "notifications"},
        {key: "calendar",      label: qsTr("Calendar"),      icon: "calendar"},
        {key: "audio",         label: qsTr("Audio"),         icon: "audio"},
        {key: "workspaces",    label: qsTr("Workspaces"),    icon: "workspaces"},
        {key: "keybindings",   label: qsTr("Keybindings"),   icon: "keybindings"},
        {key: "integrations",  label: qsTr("Integrations"),  icon: "integrations"},
        {key: "advanced",      label: qsTr("Advanced"),      icon: "advanced"},
        {key: "about",         label: qsTr("About"),         icon: "about"}
    ]
    readonly property int currentPageIndex: {
        for (let pageIndex = 0; pageIndex < root.pages.length; ++pageIndex) {
            if (root.pages[pageIndex].key === root.currentPage)
                return pageIndex
        }
        return -1
    }
    readonly property string currentPageTitle: currentPageIndex >= 0 ? pages[currentPageIndex].label : ""
    readonly property Item appTitleItem: navHeader.contentItem as Item
    readonly property real minimumContentWidth: appTitleItem
                                                ? appTitleItem.implicitWidth + navHeader.horizontalPadding * 2
                                                : 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HnHeaderBar {
            id: navHeader

            objectName: "settingsNavHeader"
            Layout.fillWidth: true
            dividerVisible: false
            content: HnAppTitle {
                id: appTitle

                objectName: "settingsAppTitle"
                applicationName: root.applicationName
                iconSource: "qrc:/HolonightSettings/holonight-settings.svg"
                iconTinted: false
            }
        }

        ListView {
            objectName: "navPageList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 8
            Layout.topMargin: 16
            Layout.rightMargin: 8
            Layout.bottomMargin: 8
            spacing: 2
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.pages
            currentIndex: root.currentPageIndex

            delegate: HnNavigationDelegate {
                id: navDelegate
                required property int index
                required property var modelData

                objectName: "navDelegate-" + navDelegate.modelData.key
                width: ListView.view.width
                sizeRole: HnControlSize.Large
                title: navDelegate.modelData.label
                checked: root.currentPage === navDelegate.modelData.key
                leadingContent: Component {
                    HnIcon {
                        source: "qrc:/HolonightSettings/settings-navigation/" + navDelegate.modelData.icon + ".svg"
                        size: HnControlMetrics.iconSize(navDelegate.resolvedSizeRole)
                        iconState: navDelegate.checked ? HnIcon.Active : HnIcon.Normal
                    }
                }
                onClicked: root.pageRequested(navDelegate.modelData.key)
            }
        }
    }
}
