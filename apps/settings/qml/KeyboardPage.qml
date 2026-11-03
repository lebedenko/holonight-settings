// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls
import HolonightSettings

Item {
    id: root
    required property ShellSettingsEditModel editModel

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        HnSettingsRow {
            Layout.fillWidth: true
            titleText: qsTr("Navigation style")
            descriptionText: qsTr("Shared by Files, the file chooser, and Bluetooth. Text editing remains conventional.")
            control: Component {
                Controls.ComboBox {
                    objectName: "navigationStyleSelector"
                    model: [qsTr("Standard"), qsTr("Vim navigation")]
                    currentIndex: root.editModel.navigationStyle === "vim-navigation" ? 1 : 0
                    onActivated: root.editModel.navigationStyle = currentIndex === 1 ? "vim-navigation" : "standard"
                }
            }
        }
        HnLabel {
            Layout.fillWidth: true
            rawText: root.editModel.document.diagnostics
            visible: rawText !== ""
            wrapMode: Text.WordWrap
        }
        Controls.Button {
            objectName: "resetNavigationStyle"
            text: qsTr("Reset navigation to default")
            onClicked: root.editModel.document.reset("navigationStyle")
        }
        Item { Layout.fillHeight: true }
    }
}
