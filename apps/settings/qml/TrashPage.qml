pragma ComponentBehavior: Bound

import HolonightSettings
import Holonight.Core
import Holonight.Controls
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Controls.ScrollView {
    id: root
    required property TrashSettingsModel editModel
    padding: HnMetrics.internalSpacing(HnControlSize.Normal)
    contentWidth: availableWidth
    Component.onCompleted: root.editModel?.refreshShares()

    ColumnLayout {
        width: root.availableWidth
        spacing: HnMetrics.internalSpacing(HnControlSize.Normal)

        Controls.CheckBox {
            objectName: "trashRetentionEnabled"
            text: qsTr("Automatically delete old Trash items")
            checked: root.editModel?.enabled ?? false
            onToggled: if (root.editModel) root.editModel.enabled = checked
        }

        RowLayout {
            HnLabel { rawText: qsTr("Retention in days") }
            Controls.SpinBox {
                objectName: "trashRetentionDays"
                from: 1
                to: 365
                editable: true
                value: root.editModel?.retentionDays ?? 7
                onValueModified: if (root.editModel) root.editModel.retentionDays = value
                Accessible.name: qsTr("Retention in days")
            }
        }

        HnLabel { rawText: qsTr("NFS shares included in automatic cleanup") }
        HnLabel {
            visible: (root.editModel?.shares.length ?? 0) === 0
            rawText: qsTr("No NFS shares are currently mounted.")
        }
        Repeater {
            model: root.editModel?.shares ?? []
            delegate: Controls.CheckBox {
                required property var modelData
                checked: modelData.enabled
                text: modelData.online ? qsTr("%1 (%2)").arg(modelData.mount).arg(modelData.source)
                                       : qsTr("%1 (%2, offline)").arg(modelData.mount).arg(modelData.source)
                onToggled: root.editModel?.setShare(modelData.mount, modelData.source, checked)
            }
        }
        HnLabel {
            Layout.fillWidth: true
            visible: rawText.length > 0
            wrapMode: Text.Wrap
            rawText: root.editModel?.error ?? ""
        }
    }
}
