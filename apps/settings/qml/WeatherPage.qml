pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls

import HolonightSettings

Flickable {
    id: root

    required property ShellSettingsEditModel editModel

    function choices(property: string, labels: var): var {
        return root.editModel.document.metadata(property).choices.map(value => ({ text: labels[value] || value, value: value }));
    }

    readonly property real rowHorizontalPadding: 16
    readonly property real inlineControlWidth: Math.max(180, Math.min(420, (width - 80) * 0.55))

    WeatherLocationSearch { id: locationSearch }
    property string coordinateError: ""

    function findLocation(text: string) {
        root.coordinateError = ""
        const match = text.trim().match(/^([+-]?(?:\d+(?:\.\d*)?|\.\d+))\s*,\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+))$/)
        if (match) {
            locationSearch.search("")
            if (!root.editModel.selectLocation("", "", Number(match[1]), Number(match[2])))
                root.coordinateError = qsTr("Latitude must be −90…90 and longitude −180…180.")
        } else {
            locationSearch.search(text, root.editModel.weatherLanguage)
        }
    }

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

            OverridePanel {
                Layout.fillWidth: true
                document: root.editModel.document
                group: "weather"
            }

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
                        Controls.ComboBox {
                            id: weatherProviderComboBox

                            objectName: "weatherProviderComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: root.choices("weatherProvider", { "open-meteo": qsTr("Open-Meteo"), "openweathermap": qsTr("OpenWeatherMap") })
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
                        Controls.TextField {
                            id: weatherApiKeyTextField

                            objectName: "weatherApiKeyTextField"
                            implicitWidth: root.inlineControlWidth
                            placeholderText: qsTr("Enter OWM API Key...")
                            text: root.editModel.weatherApiKey
                            onTextEdited: root.editModel.weatherApiKey = text
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
                        Controls.ComboBox {
                            id: weatherLocationSourceComboBox

                            objectName: "weatherLocationSourceComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: root.choices("weatherLocationSource", { "manual": qsTr("Manual"), "auto": qsTr("Auto (IP Geolocation)") })
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
                    visible: root.editModel.weatherLocationSource === "manual"
                    titleText: qsTr("City / Coordinates")
                    descriptionText: qsTr("Search for a city or enter coordinates")
                    sizeRole: HnControlSize.Hero
                    stacked: false
                    dividerVisible: false
                    contentHorizontalPadding: root.rowHorizontalPadding
                    Layout.fillWidth: true

                    control: Component {
                        Controls.TextField {
                            id: weatherCityTextField

                            objectName: "weatherCityTextField"
                            implicitWidth: root.inlineControlWidth
                            placeholderText: qsTr("e.g. Warsaw, Poland or 52.2297, 21.0122")
                            text: root.editModel.weatherCity
                            onTextEdited: { locationSearch.search(""); root.coordinateError = "" }
                            onAccepted: root.findLocation(text)
                        }
                    }
                }
            }

            HnSettingsRow {
                visible: root.editModel.weatherLocationSource === "auto"
                titleText: qsTr("IP geolocation API key")
                descriptionText: qsTr("An ipgeolocation.io API key is required for automatic location.")
                Layout.fillWidth: true
                control: Component {
                    Controls.TextField {
                        implicitWidth: root.inlineControlWidth
                        text: root.editModel.weatherGeoApiKey
                        onTextEdited: root.editModel.weatherGeoApiKey = text
                    }
                }
            }
            HnLabel {
                Layout.fillWidth: true
                visible: root.editModel.weatherLocationSource === "manual"
                rawText: locationSearch.loading ? qsTr("Searching…") : root.coordinateError || locationSearch.status || qsTr("Press Enter to search, then select a result. Coordinates are staged on Enter.")
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: root.editModel.weatherLocationSource === "manual" ? locationSearch.results : []
                delegate: Controls.Button {
                    required property var modelData
                    Layout.fillWidth: true
                    text: [modelData.city, modelData.region, modelData.country].filter(value => value.length > 0).join(", ")
                    onClicked: {
                        root.editModel.selectLocation(modelData.city, modelData.country, modelData.latitude, modelData.longitude)
                        locationSearch.search("")
                    }
                }
            }
            HnLabel {
                Layout.fillWidth: true
                visible: root.editModel.weatherLocationSource === "manual" && root.editModel.weatherLatitude !== undefined && root.editModel.weatherLongitude !== undefined
                rawText: qsTr("Selected: %1, %2").arg(root.editModel.weatherLatitude).arg(root.editModel.weatherLongitude)
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
                        Controls.ComboBox {
                            id: weatherTempUnitComboBox

                            objectName: "weatherTempUnitComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: root.choices("weatherTempUnit", { "celsius": qsTr("°C (Celsius)"), "fahrenheit": qsTr("°F (Fahrenheit)"), "kelvin": qsTr("K (Kelvin)") })
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
                        Controls.ComboBox {
                            id: weatherWindUnitComboBox

                            objectName: "weatherWindUnitComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: root.choices("weatherWindUnit", { "kmh": qsTr("km/h"), "ms": qsTr("m/s"), "mph": qsTr("mph"), "knots": qsTr("knots") })
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
                        Controls.ComboBox {
                            id: weatherPressureUnitComboBox

                            objectName: "weatherPressureUnitComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: root.choices("weatherPressureUnit", { "hpa": qsTr("hPa"), "mmhg": qsTr("mmHg"), "inhg": qsTr("inHg"), "bar": qsTr("bar") })
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
                        Controls.Switch {
                            id: weatherShowInBarSwitch

                            objectName: "weatherShowInBarSwitch"
                            checked: root.editModel.weatherShowInBar
                            onClicked: root.editModel.weatherShowInBar = checked
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
                        Controls.Switch {
                            id: weatherCompactModeSwitch

                            objectName: "weatherCompactModeSwitch"
                            checked: root.editModel.weatherCompactMode
                            onClicked: root.editModel.weatherCompactMode = checked
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
                        Controls.Switch {
                            id: weatherShowFeelsLikeSwitch

                            objectName: "weatherShowFeelsLikeSwitch"
                            checked: root.editModel.weatherShowFeelsLike
                            onClicked: root.editModel.weatherShowFeelsLike = checked
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
                        Controls.Switch {
                            id: weatherShowLocationSwitch

                            objectName: "weatherShowLocationSwitch"
                            checked: root.editModel.weatherShowLocation
                            onClicked: root.editModel.weatherShowLocation = checked
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
                        Controls.ComboBox {
                            id: weatherRefreshIntervalComboBox

                            objectName: "weatherRefreshIntervalComboBox"
                            implicitWidth: root.inlineControlWidth
                            model: [
                                { text: qsTr("10 minutes"), value: 600 },
                                { text: qsTr("15 minutes"), value: 900 },
                                { text: qsTr("30 minutes"), value: 1800 },
                                { text: qsTr("1 hour"), value: 3600 }
                            ].concat([{text: qsTr("%1 seconds").arg(root.editModel.weatherRefreshInterval), value: root.editModel.weatherRefreshInterval}]).filter((choice, index, values) => values.findIndex(other => other.value === choice.value) === index)
                            textRole: "text"
                            valueRole: "value"
                            currentIndex: {
                                for (let i = 0; i < model.length; ++i) {
                                    if (model[i].value === root.editModel.weatherRefreshInterval) return i;
                                }
                                return -1;
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
