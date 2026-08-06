import QtQuick
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HnHeaderBar {
            objectName: "settingsPreviewHeader"
            Layout.fillWidth: true
            content: HnPanelHeader {
                objectName: "settingsPreviewHeaderTitle"
                title: qsTr("Preview")
                dividerVisible: false
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            HnLabel {
                objectName: "previewPlaceholderLabel"
                anchors.centerIn: parent
                role: HnTypographyRole.Caption
                rawText: qsTr("Not yet implemented")
            }
        }
    }
}
