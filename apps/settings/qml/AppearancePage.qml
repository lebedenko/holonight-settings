pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import Holonight
import Holonight.Core
import Holonight.Controls

import HolonightSettings

Flickable {
    id: root

    required property SettingsEditModel editModel

    readonly property real rowHorizontalPadding: 16
    readonly property real inlineControlWidth: Math.max(180, Math.min(420, (width - 80) * 0.55))

    contentHeight: contentItem.implicitHeight
    contentWidth: width
    clip: true

    function familyIdForScheme(schemeId: string): string {
        for (const family of HolonightTheme.themeFamilies) {
            for (const variant of family.variants) {
                if (variant.id === schemeId)
                    return family.id
            }
        }
        return ""
    }

    function variantIdForFamilyAndMode(family: var, mode: string): string {
        const target = family.variants.find(variant => variant.mode === mode)
        return target ? target.id : family.variants[0].id
    }

    component SectionGroup: ColumnLayout {
        property alias label: sectionHeader.titleText
        property alias frameObjectName: sectionFrame.objectName
        default property alias content: sectionRows.data
        spacing: 8

        HnSectionHeader {
            id: sectionHeader

            objectName: "appearanceSectionHeader"
            isCategoryMode: true
            showPrefix: true
            dividerVisible: false
            Layout.fillWidth: true
        }

        HnSurfaceFrame {
            id: sectionFrame

            surfaceRole: HnSurfaceRole.Card
            implicitHeight: sectionRows.implicitHeight + normalizedBorderWidth * 2
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight

            ColumnLayout {
                id: sectionRows

                anchors.fill: parent
                anchors.margins: sectionFrame.normalizedBorderWidth
                spacing: 0
            }
        }
    }

    Item {
        id: contentItem

        width: root.width
        implicitHeight: content.implicitHeight + 48

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 24
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 20

            SectionGroup {
                frameObjectName: "themeSectionFrame"
                label: qsTr("Theme")
                Layout.fillWidth: true

                HnSettingsRow {
                    objectName: "colorSchemeRow"
                    titleText: qsTr("Color scheme")
                    descriptionText: qsTr("Choose your preferred color scheme")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Item {
                            implicitWidth: root.inlineControlWidth
                            implicitHeight: 76.8

                            Flickable {
                                id: swatchFlickable

                                objectName: "colorSchemeSwatchFlickable"
                                anchors.fill: parent
                                contentWidth: swatchRow.implicitWidth
                                contentHeight: height
                                flickableDirection: Flickable.HorizontalFlick
                                boundsBehavior: Flickable.StopAtBounds
                                clip: true

                                ButtonGroup {
                                    id: themeFamilyGroup
                                }

                                Row {
                                    id: swatchRow

                                    spacing: 8

                                    Repeater {
                                        model: HolonightTheme.themeFamilies

                                        delegate: ColorSchemeSwatchCard {
                                            id: themeFamilyDelegate

                                            required property var modelData
                                            required property int index
                                            familyId: modelData.id
                                            title: modelData.name
                                            schemeId: root.variantIdForFamilyAndMode(modelData, editModel.themeMode)
                                            checked: root.familyIdForScheme(editModel.themeScheme) === familyId
                                            ButtonGroup.group: themeFamilyGroup
                                            onClicked: {
                                                editModel.themeScheme = root.variantIdForFamilyAndMode(
                                                    themeFamilyDelegate.modelData, editModel.themeMode)
                                            }
                                        }
                                    }
                                }
                            }

                            EdgeFadeOverlay {
                                anchors.fill: parent
                                flickable: swatchFlickable
                                fadeColor: HoloniightPalette.surfaceElevated
                            }
                        }
                    }
                }

                HnSettingsRow {
                    objectName: "accentColorRow"
                    titleText: qsTr("Accent color")
                    descriptionText: qsTr("Used for highlights and interactive elements")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        HnColorPicker {
                            id: accentColorPicker
                            objectName: "accentColorPicker"

                            readonly property var accentOptions: HolonightTheme.accentOptionsForScheme(editModel.themeScheme)

                            autoUpdateSelectedColor: false
                            colors: accentOptions.map(opt => opt.color)

                            selectedColor: {
                                const opt = accentOptions.find(o => o.id === editModel.themeAccent)
                                if (opt)
                                    return opt.color
                                const variant = HolonightTheme.themeVariants.find(v => v.id === editModel.themeScheme)
                                return variant ? variant.defaultAccentColor : (accentOptions.length > 0 ? accentOptions[0].color : Qt.color("transparent"))
                            }

                            onColorSelected: (color) => {
                                const opt = accentOptions.find(o => o.color.toString() === color.toString())
                                if (opt)
                                    editModel.themeAccent = opt.id
                            }
                        }
                    }
                }

                HnSettingsRow {
                    objectName: "darkModeRow"
                    titleText: qsTr("Dark mode")
                    descriptionText: qsTr("Use dark theme for all components")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Switch {
                            sizeRole: HnControlSize.Large
                            objectName: "darkModeSwitch"
                            checked: editModel.themeMode === "dark"
                            onToggled: editModel.themeMode = checked ? "dark" : "light"
                        }
                    }
                }

                HnSettingsRow {
                    objectName: "transparencyRow"
                    titleText: qsTr("Transparency")
                    descriptionText: qsTr("Control the transparency of shell components")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "transparencyControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Slider {
                                objectName: "transparencySlider"
                                from: 0
                                to: 100
                                stepSize: 1
                                value: editModel.transparency
                                onMoved: editModel.transparency = Math.round(value)
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                objectName: "transparencyValue"
                                role: HnTypographyRole.Body
                                rawText: qsTr("%1%").arg(editModel.transparency)
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 40
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

                HnSettingsRow {
                    objectName: "blurStrengthRow"
                    titleText: qsTr("Blur strength")
                    descriptionText: qsTr("Adjust background blur intensity")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "blurStrengthControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Slider {
                                objectName: "blurStrengthSlider"
                                from: 0
                                to: 64
                                stepSize: 1
                                value: editModel.blurStrength
                                onMoved: editModel.blurStrength = Math.round(value)
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                objectName: "blurStrengthValue"
                                role: HnTypographyRole.Body
                                rawText: qsTr("%1 px").arg(editModel.blurStrength)
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 40
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }
            }

            SectionGroup {
                frameObjectName: "typographySectionFrame"
                label: qsTr("Typography")
                Layout.fillWidth: true

                HnSettingsRow {
                    objectName: "uiFontRow"
                    titleText: qsTr("Interface font")
                    descriptionText: qsTr("Used throughout shell components and settings")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        HnIconComboBox {
                            id: uiFontCombo

                            objectName: "uiFontCombo"
                            implicitWidth: root.inlineControlWidth
                            model: FontListModel {
                                id: uiFontModel
                                fixedPitchOnly: false
                            }
                            textRole: "display"
                            currentIndex: uiFontModel.indexOf(editModel.uiFont)
                            onActivated: editModel.uiFont = currentText
                        }
                    }
                }

                HnSettingsRow {
                    id: uiFontSizeRow

                    objectName: "uiFontSizeRow"
                    titleText: qsTr("Interface font size")
                    descriptionText: qsTr("Adjust the size of interface text")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "uiFontSizeControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Slider {
                                objectName: "uiFontSizeSlider"
                                from: 8
                                to: 18
                                stepSize: 1
                                value: editModel.uiFontSize
                                activeFocusOnTab: true
                                onMoved: editModel.uiFontSize = Math.round(value)
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                objectName: "uiFontSizeValue"
                                role: HnTypographyRole.Body
                                rawText: qsTr("%1 pt").arg(editModel.uiFontSize)
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 40
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

                HnSettingsRow {
                    objectName: "fixedFontRow"
                    titleText: qsTr("Monospace font")
                    descriptionText: qsTr("Used for fixed-width text and technical content")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        HnIconComboBox {
                            id: fixedFontCombo

                            objectName: "fixedFontCombo"
                            implicitWidth: root.inlineControlWidth
                            model: FontListModel {
                                id: fixedFontModel
                                fixedPitchOnly: true
                            }
                            textRole: "display"
                            currentIndex: fixedFontModel.indexOf(editModel.fixedFont)
                            onActivated: editModel.fixedFont = currentText
                        }
                    }
                }

                HnSettingsRow {
                    id: fixedFontSizeRow

                    objectName: "fixedFontSizeRow"
                    titleText: qsTr("Monospace font size")
                    descriptionText: qsTr("Adjust the size of monospace text")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "fixedFontSizeControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Slider {
                                objectName: "fixedFontSizeSlider"
                                from: 8
                                to: 18
                                stepSize: 1
                                value: editModel.fixedFontSize
                                activeFocusOnTab: true
                                onMoved: editModel.fixedFontSize = Math.round(value)
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                objectName: "fixedFontSizeValue"
                                role: HnTypographyRole.Body
                                rawText: qsTr("%1 pt").arg(editModel.fixedFontSize)
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 40
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }
            }

            SectionGroup {
                frameObjectName: "globalShapeSectionFrame"
                label: qsTr("Global Shape")
                Layout.fillWidth: true

                HnSettingsRow {
                    objectName: "cornerStyleRow"
                    titleText: qsTr("Corner style")
                    descriptionText: qsTr("Applies to all HoloNight applications.")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        HnSegmentedControl {
                            id: shapeStyleControl

                            objectName: "shapeStyleControl"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { value: "inherit", text: qsTr("Inherit") },
                                { value: "hybrid", text: qsTr("Hybrid") },
                                { value: "rounded", text: qsTr("Rounded") },
                                { value: "chamfered", text: qsTr("Chamfered") }
                            ]
                            Component.onCompleted: currentIndex = indexForValue(editModel.shapeCornerStyle)
                            onActivated: (_, value) => editModel.shapeCornerStyle = String(value)

                            function indexForValue(value: string): int {
                                for (let index = 0; index < model.length; ++index) {
                                    if (model[index].value === value)
                                        return index
                                }
                                return 0
                            }

                            Connections {
                                target: editModel

                                function onShapeCornerStyleChanged(): void {
                                    shapeStyleControl.currentIndex =
                                        shapeStyleControl.indexForValue(editModel.shapeCornerStyle)
                                }
                            }
                        }
                    }
                }

                HnSettingsRow {
                    id: shapeScaleRow

                    objectName: "shapeScaleRow"
                    titleText: qsTr("Shape scale")
                    descriptionText: qsTr("Scale shared corner radii and chamfers")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "shapeScaleControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Slider {
                                objectName: "shapeScaleSlider"
                                from: 0.25
                                to: 4.0
                                stepSize: 0.05
                                value: editModel.shapeScale
                                onMoved: editModel.shapeScale = value
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                role: HnTypographyRole.Body
                                rawText: Number(editModel.shapeScale).toFixed(2) + qsTr("×")
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 52
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }
            }

            SectionGroup {
                frameObjectName: "advancedShapeSectionFrame"
                label: qsTr("Advanced Shape")
                Layout.fillWidth: true

                HnSettingsRow {
                    id: baseRadiusRow

                    objectName: "baseRadiusRow"
                    titleText: qsTr("Override base radius")
                    descriptionText: qsTr("Flatten the rounded-corner size family to this radius")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "baseRadiusControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Switch {
                                objectName: "baseRadiusSwitch"
                                checked: editModel.baseRadiusEnabled
                                onToggled: editModel.baseRadiusEnabled = checked
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Slider {
                                objectName: "baseRadiusSlider"
                                from: 0
                                to: 128
                                stepSize: 1
                                enabled: editModel.baseRadiusEnabled
                                value: editModel.baseRadius
                                onMoved: editModel.baseRadius = value
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                role: HnTypographyRole.Body
                                rawText: qsTr("%1 px").arg(Math.round(editModel.baseRadius))
                                color: editModel.baseRadiusEnabled
                                    ? HoloniightPalette.textPrimary
                                    : HoloniightPalette.textDisabled
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 52
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

                HnSettingsRow {
                    id: baseChamferRow

                    objectName: "baseChamferRow"
                    titleText: qsTr("Override base chamfer")
                    descriptionText: qsTr("Flatten the chamfered-corner size family to this chamfer")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        RowLayout {
                            objectName: "baseChamferControls"
                            implicitWidth: root.inlineControlWidth
                            spacing: 8

                            Switch {
                                objectName: "baseChamferSwitch"
                                checked: editModel.baseChamferEnabled
                                onToggled: editModel.baseChamferEnabled = checked
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Slider {
                                objectName: "baseChamferSlider"
                                from: 0
                                to: 128
                                stepSize: 1
                                enabled: editModel.baseChamferEnabled
                                value: editModel.baseChamfer
                                onMoved: editModel.baseChamfer = value
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                            HnLabel {
                                role: HnTypographyRole.Body
                                rawText: qsTr("%1 px").arg(Math.round(editModel.baseChamfer))
                                color: editModel.baseChamferEnabled
                                    ? HoloniightPalette.textPrimary
                                    : HoloniightPalette.textDisabled
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 52
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
    }
}