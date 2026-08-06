# Weather Settings Page — Specification

## 1. Scope & Objectives

This specification defines the functional, interface, configuration, and visual requirements for the **Weather Settings Page** in HoloNight Settings.

---

## 2. Functional Requirements

### F1: Provider Settings
- **REQ-F-1.1**: The user shall be able to select the weather service provider (`provider` property). Supported values: `"open-meteo"`, `"openweathermap"`.
- **REQ-F-1.2**: When `"openweathermap"` is selected, an API key text field (`api_key` property) shall allow user entry.

### F2: Location Settings
- **REQ-F-2.1**: The user shall be able to choose the location resolution strategy (`location_source` property). Supported values: `"manual"`, `"auto"`.
- **REQ-F-2.2**: In manual location mode, the user shall be able to enter a city name or coordinates (`city` property).

### F3: Units Settings
- **REQ-F-3.1**: The user shall be able to select the temperature unit (`temp_unit` property: `"celsius"`, `"fahrenheit"`, `"kelvin"`).
- **REQ-F-3.2**: The user shall be able to select the wind speed unit (`wind_unit` property: `"kmh"`, `"ms"`, `"mph"`, `"knots"`).
- **REQ-F-3.3**: The user shall be able to select the pressure unit (`pressure_unit` property: `"hpa"`, `"mmhg"`, `"inhg"`, `"bar"`).

### F4: Display & Update Settings
- **REQ-F-4.1**: The user shall be able to toggle top bar module visibility (`show_in_bar` property, boolean).
- **REQ-F-4.2**: The user shall be able to toggle compact mode (`compact_mode` property, boolean).
- **REQ-F-4.3**: The user shall be able to toggle feels-like temperature display (`show_feels_like` property, boolean).
- **REQ-F-4.4**: The user shall be able to toggle location label display (`show_location` property, boolean).
- **REQ-F-4.5**: The user shall be able to select update interval (`refresh_interval` property: 600, 900, 1800, 3600 seconds).

### F5: Configuration & Dirty State
- **REQ-F-5.1**: All weather options shall be initialized from `config.toml` `[weather]` section.
- **REQ-F-5.2**: Any difference between the edited model and snapshot shall mark `isDirty` as `true`.
- **REQ-F-5.3**: Saving shall write `[weather]` settings to `config.toml` without dropping comments or unedited fields.

---

## 3. QML Object Names & Element Contracts

To enable automated testing and accessibility, `WeatherPage.qml` shall define the following unique `objectName` identifiers:

- `weatherSectionHeader` (Section headers)
- `providerSectionFrame` (Provider card frame)
- `weatherProviderRow` & `weatherProviderComboBox`
- `weatherApiKeyRow` & `weatherApiKeyTextField`
- `locationSectionFrame` (Location card frame)
- `weatherLocationSourceRow` & `weatherLocationSourceComboBox`
- `weatherCityRow` & `weatherCityTextField`
- `unitsSectionFrame` (Units card frame)
- `weatherTempUnitRow` & `weatherTempUnitComboBox`
- `weatherWindUnitRow` & `weatherWindUnitComboBox`
- `weatherPressureUnitRow` & `weatherPressureUnitComboBox`
- `displaySectionFrame` (Display card frame)
- `weatherShowInBarRow` & `weatherShowInBarSwitch`
- `weatherCompactModeRow` & `weatherCompactModeSwitch`
- `weatherShowFeelsLikeRow` & `weatherShowFeelsLikeSwitch`
- `weatherShowLocationRow` & `weatherShowLocationSwitch`
- `weatherRefreshIntervalRow` & `weatherRefreshIntervalComboBox`

---

## 4. Acceptance Criteria

1. Navigating to the Weather page in HoloNight Settings displays the 4 section cards matching the appearance page 3-column layout.
2. Changing any weather setting updates `SettingsEditModel` and turns the **Apply Changes** button active (`isDirty == true`).
3. Discarding changes reverts all weather controls back to the loaded configuration.
4. Applying changes writes all `[weather]` fields to `config.toml` and preserves all other sections.
5. All automated unit tests in `test_settings_app.cpp` pass cleanly.
