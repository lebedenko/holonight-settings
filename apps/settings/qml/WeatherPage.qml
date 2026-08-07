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

    required property ShellSettingsEditModel editModel

    readonly property real rowHorizontalPadding: 16
    readonly property real inlineControlWidth: Math.max(180, Math.min(420, (width - 80) * 0.55))

    contentHeight: contentItem.implicitHeight
    contentWidth: width
    clip: true

    component SectionGroup: ColumnLayout {
        property alias label: sectionHeader.titleText
        property alias headerObjectName: sectionHeader.objectName
        property alias frameObjectName: sectionFrame.objectName
        default property alias content: sectionRows.data
        spacing: 8

        HnSectionHeader {
            id: sectionHeader

            objectName: "weatherSectionHeader"
            sizeRole: HnControlSize.Compact
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

            // Provider Section
            SectionGroup {
                frameObjectName: "providerSectionFrame"
                label: qsTr("Provider")
                Layout.fillWidth: true

                HnSettingsRow {
                    id: weatherProviderRow

                    objectName: "weatherProviderRow"
                    titleText: qsTr("Weather provider")
                    descriptionText: qsTr("Choose the service used to fetch weather data")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: root.editModel.weatherProvider === "openweathermap"
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        ComboBox {
                            id: weatherProviderComboBox

                            objectName: "weatherProviderComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("Open-Meteo"), value: "open-meteo" },
                                { text: qsTr("OpenWeatherMap"), value: "openweathermap" }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherProvider) return i;
                                }
                                return 0;
                            }
                            onActivated: (index) => {
                                root.editModel.weatherProvider = model[index].value;
                            }
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherApiKeyRow

                    objectName: "weatherApiKeyRow"
                    visible: root.editModel.weatherProvider === "openweathermap"
                    titleText: qsTr("API key")
                    descriptionText: qsTr("OpenWeatherMap API key for weather data retrieval")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        TextField {
                            id: weatherApiKeyTextField

                            objectName: "weatherApiKeyTextField"
                            implicitWidth: root.inlineControlWidth
                            placeholderText: qsTr("Enter OWM API Key...")
                            text: root.editModel.weatherApiKey
                            onTextChanged: root.editModel.weatherApiKey = text
                        }
                    }
                }
            }

            // Location Section
            SectionGroup {
                frameObjectName: "locationSectionFrame"
                label: qsTr("Location")
                Layout.fillWidth: true

                HnSettingsRow {
                    id: weatherLocationSourceRow

                    objectName: "weatherLocationSourceRow"
                    titleText: qsTr("Location source")
                    descriptionText: qsTr("How should HoloNight get your location?")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        ComboBox {
                            id: weatherLocationSourceComboBox

                            objectName: "weatherLocationSourceComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("Manual"), value: "manual" },
                                { text: qsTr("Auto (IP Geolocation)"), value: "auto" }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherLocationSource) return i;
                                }
                                return 0;
                            }
                            onActivated: (index) => {
                                root.editModel.weatherLocationSource = model[index].value;
                            }
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherCityRow

                    objectName: "weatherCityRow"
                    titleText: qsTr("City / Coordinates")
                    descriptionText: qsTr("Search for a city or enter coordinates")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        TextField {
                            id: weatherCityTextField

                            objectName: "weatherCityTextField"
                            implicitWidth: root.inlineControlWidth
                            placeholderText: qsTr("e.g. Warsaw, Poland or 52.2297, 21.0122")
                            text: root.editModel.weatherCity
                            onTextChanged: root.editModel.weatherCity = text
                        }
                    }
                }
            }

            // Units Section
            SectionGroup {
                frameObjectName: "unitsSectionFrame"
                label: qsTr("Units")
                Layout.fillWidth: true

                HnSettingsRow {
                    id: weatherTempUnitRow

                    objectName: "weatherTempUnitRow"
                    titleText: qsTr("Temperature unit")
                    descriptionText: qsTr("Display temperature in")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        ComboBox {
                            id: weatherTempUnitComboBox

                            objectName: "weatherTempUnitComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("°C (Celsius)"), value: "celsius" },
                                { text: qsTr("°F (Fahrenheit)"), value: "fahrenheit" },
                                { text: qsTr("K (Kelvin)"), value: "kelvin" }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherTempUnit) return i;
                                }
                                return 0;
                            }
                            onActivated: (index) => {
                                root.editModel.weatherTempUnit = model[index].value;
                            }
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherWindUnitRow

                    objectName: "weatherWindUnitRow"
                    titleText: qsTr("Wind speed unit")
                    descriptionText: qsTr("Display wind speed in")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        ComboBox {
                            id: weatherWindUnitComboBox

                            objectName: "weatherWindUnitComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("km/h"), value: "kmh" },
                                { text: qsTr("m/s"), value: "ms" },
                                { text: qsTr("mph"), value: "mph" },
                                { text: qsTr("knots"), value: "knots" }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherWindUnit) return i;
                                }
                                return 0;
                            }
                            onActivated: (index) => {
                                root.editModel.weatherWindUnit = model[index].value;
                            }
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherPressureUnitRow

                    objectName: "weatherPressureUnitRow"
                    titleText: qsTr("Pressure unit")
                    descriptionText: qsTr("Display pressure in")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        ComboBox {
                            id: weatherPressureUnitComboBox

                            objectName: "weatherPressureUnitComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("hPa"), value: "hpa" },
                                { text: qsTr("mmHg"), value: "mmhg" },
                                { text: qsTr("inHg"), value: "inhg" },
                                { text: qsTr("bar"), value: "bar" }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherPressureUnit) return i;
                                }
                                return 0;
                            }
                            onActivated: (index) => {
                                root.editModel.weatherPressureUnit = model[index].value;
                            }
                        }
                    }
                }
            }

            // Display Section
            SectionGroup {
                frameObjectName: "displaySectionFrame"
                label: qsTr("Display")
                Layout.fillWidth: true

                HnSettingsRow {
                    id: weatherShowInBarRow

                    objectName: "weatherShowInBarRow"
                    titleText: qsTr("Show in bar")
                    descriptionText: qsTr("Show weather module in the top bar")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Switch {
                            id: weatherShowInBarSwitch

                            objectName: "weatherShowInBarSwitch"
                            checked: root.editModel.weatherShowInBar
                            onCheckedChanged: root.editModel.weatherShowInBar = checked
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherCompactModeRow

                    objectName: "weatherCompactModeRow"
                    titleText: qsTr("Compact mode")
                    descriptionText: qsTr("Show minimal weather information")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Switch {
                            id: weatherCompactModeSwitch

                            objectName: "weatherCompactModeSwitch"
                            checked: root.editModel.weatherCompactMode
                            onCheckedChanged: root.editModel.weatherCompactMode = checked
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherShowFeelsLikeRow

                    objectName: "weatherShowFeelsLikeRow"
                    titleText: qsTr("Show feels like temperature")
                    descriptionText: qsTr("Display \"feels like\" temperature")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Switch {
                            id: weatherShowFeelsLikeSwitch

                            objectName: "weatherShowFeelsLikeSwitch"
                            checked: root.editModel.weatherShowFeelsLike
                            onCheckedChanged: root.editModel.weatherShowFeelsLike = checked
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherShowLocationRow

                    objectName: "weatherShowLocationRow"
                    titleText: qsTr("Show location")
                    descriptionText: qsTr("Show location name in the tooltip")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: true
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Switch {
                            id: weatherShowLocationSwitch

                            objectName: "weatherShowLocationSwitch"
                            checked: root.editModel.weatherShowLocation
                            onCheckedChanged: root.editModel.weatherShowLocation = checked
                        }
                    }
                }

                HnSettingsRow {
                    id: weatherRefreshIntervalRow

                    objectName: "weatherRefreshIntervalRow"
                    titleText: qsTr("Update interval")
                    descriptionText: qsTr("How often to update weather data")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        ComboBox {
                            id: weatherRefreshIntervalComboBox

                            objectName: "weatherRefreshIntervalComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("10 minutes"), value: 600 },
                                { text: qsTr("15 minutes"), value: 900 },
                                { text: qsTr("30 minutes"), value: 1800 },
                                { text: qsTr("1 hour"), value: 3600 }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherRefreshInterval) return i;
                                }
                                return 2;
                            }
                            onActivated: (index) => {
                                root.editModel.weatherRefreshInterval = model[index].value;
                            }
                        }
                    }
                }
            }
        }
    }
}
