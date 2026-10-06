pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls
import HolonightSettings

ColumnLayout {
    id: root
    required property DocumentEditSession document
    property string group: ""
    property bool expanded: false
    spacing: 8

    HnLabel {
        Layout.fillWidth: true
        visible: root.document.diagnostics !== ""
        rawText: root.document.diagnostics
        wrapMode: Text.WordWrap
    }
    Controls.Button {
        text: root.expanded ? qsTr("Hide overrides") : qsTr("Defaults and overrides")
        onClicked: root.expanded = !root.expanded
    }
    Repeater {
        model: root.expanded ? root.document.fields : []
        delegate: RowLayout {
            id: row
            required property var modelData
            visible: root.group === "" || (root.group === "weather") === row.modelData.property.startsWith("weather")
            Layout.fillWidth: true
            HnLabel {
                Layout.fillWidth: true
                rawText: row.modelData.label + (row.modelData.overridden ? qsTr(" — Override") : qsTr(" — Default")) + (row.modelData.pending ? qsTr(" (pending)") : "")
                wrapMode: Text.WordWrap
            }
            Controls.Button {
                text: qsTr("Reset")
                enabled: row.modelData.overridden
                onClicked: root.document.reset(row.modelData.property)
            }
        }
    }
}
