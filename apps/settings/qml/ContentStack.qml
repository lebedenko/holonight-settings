import HolonightSettings
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls

Item {
    id: root

    required property SettingsEditModel editModel
    required property string currentPage
    required property string currentPageTitle
    property bool ready: false
    property string displayedPage: ""
    property int displayedPageIndex: -1
    property string pendingPage: ""
    property int transitionDirection: 1

    function pageSource(pageKey) {
        switch (pageKey) {
        case "appearance":
            return Qt.resolvedUrl("AppearancePage.qml");
        case "bar":
            return Qt.resolvedUrl("BarPage.qml");
        case "weather":
            return Qt.resolvedUrl("WeatherPage.qml");
        default:
            return Qt.resolvedUrl("PlaceholderPage.qml");
        }
    }

    function pageIndex(pageKey) {
        const pageKeys = ["appearance", "bar", "sidebar", "launcher", "weather", "notifications", "calendar", "audio", "workspaces", "keybindings", "integrations", "advanced", "about"];
        return pageKeys.indexOf(pageKey);
    }

    function pageProperties(pageKey) {
        const properties = {
            "objectName": "contentPage-" + pageKey
        };
        if (pageKey === "appearance" || pageKey === "bar" || pageKey === "weather")
            properties.editModel = root.editModel;

        return properties;
    }

    function showPage(pageKey) {
        if (!root.ready)
            return ;

        const pageIndex = root.pageIndex(pageKey);
        if (pageIndex < 0)
            return ;

        if (contentStack.busy) {
            if (pageKey === root.displayedPage)
                root.pendingPage = "";
            else
                root.pendingPage = pageKey;
            return ;
        }
        if (pageKey === root.displayedPage)
            return ;

        root.transitionDirection = pageIndex > root.displayedPageIndex ? 1 : -1;
        root.displayedPage = pageKey;
        root.displayedPageIndex = pageIndex;
        contentStack.replace(root.pageSource(pageKey), root.pageProperties(pageKey), StackView.Transition);
    }

    clip: true
    onCurrentPageChanged: root.showPage(root.currentPage)
    Component.onCompleted: {
        contentStack.push(root.pageSource(root.currentPage), root.pageProperties(root.currentPage), StackView.Immediate);
        root.displayedPage = root.currentPage;
        root.displayedPageIndex = root.pageIndex(root.currentPage);
        root.ready = true;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HnHeaderBar {
            objectName: "settingsContentHeader"
            Layout.fillWidth: true
            content: HnPanelHeader {
                objectName: "settingsContentHeaderTitle"
                title: root.currentPageTitle
                dividerVisible: false
            }
        }

        StackView {
            id: contentStack

            objectName: "settingsContentStack"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            onBusyChanged: {
                if (contentStack.busy)
                    return ;

                if (contentStack.currentItem) {
                    contentStack.currentItem.y = 0;
                    contentStack.currentItem.opacity = 1;
                }
                if (root.pendingPage === "")
                    return ;

                const nextPage = root.pendingPage;
                root.pendingPage = "";
                root.showPage(nextPage);
            }

            replaceEnter: Transition {
                YAnimator {
                    from: root.transitionDirection * 8
                    to: 0
                    duration: 160
                    easing.type: Easing.OutCubic
                }

                OpacityAnimator {
                    from: 0
                    to: 1
                    duration: 160
                    easing.type: Easing.OutCubic
                }

            }

            replaceExit: Transition {
                YAnimator {
                    from: 0
                    to: root.transitionDirection * -4
                    duration: 100
                    easing.type: Easing.InCubic
                }

                OpacityAnimator {
                    from: 1
                    to: 0
                    duration: 100
                    easing.type: Easing.InCubic
                }

            }

        }
    }
}
