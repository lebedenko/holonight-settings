import Holonight.Core
import QtQuick

Item {
    id: root

    HnLabel {
        objectName: "settingsPlaceholderLabel"
        anchors.centerIn: parent
        role: HnTypographyRole.Caption
        rawText: qsTr("Not yet implemented")
    }

}
