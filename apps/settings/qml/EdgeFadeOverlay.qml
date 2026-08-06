import QtQuick

Item {
    id: root

    required property Flickable flickable
    required property color fadeColor

    readonly property bool showLeft: flickable.contentX > 1
    readonly property bool showRight: flickable.contentX + flickable.width < flickable.contentWidth - 1

    Rectangle {
        width: 32
        visible: root.showLeft
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: root.fadeColor }
            GradientStop { position: 1.0; color: Qt.alpha(root.fadeColor, 0.0) }
        }
    }

    Rectangle {
        width: 32
        visible: root.showRight
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.alpha(root.fadeColor, 0.0) }
            GradientStop { position: 1.0; color: root.fadeColor }
        }
    }
}
