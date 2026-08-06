# Weather Settings Page — Architecture & Design

## 1. Overview & Objectives

The Weather Settings Page provides a user-configurable management interface for the HoloNight shell weather service and desktop widget integration.
It allows users to configure weather providers, location resolution strategies, unit formats (temperature, wind speed, pressure), display toggles (bar visibility, compact mode, feels-like temperature, location labels), and background update intervals.

This design aligns with the established HoloNight Settings 3-column architecture settled in `AppearancePage.qml` and `SettingsWindow.qml`.

---

## 2. System Architecture & Component Mapping

The settings application follows a decoupled model-view architecture where standard C++ configuration structures (`HolonightConfig::WeatherConfig`) are managed by `SettingsEditModel` and exposed to Qt Quick controls via Q_PROPERTY bindings.

```
┌─────────────────────────────────────────────────────────┐
│                    config.toml                          │
└────────────────────────────┬────────────────────────────┘
                             │ (Parsed via ParsedConfig)
                             ▼
┌─────────────────────────────────────────────────────────┐
│                 libs/holonight-config                   │
│              (WeatherConfig & ParsedConfig)             │
└────────────────────────────┬────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────┐
│                 SettingsEditModel (C++)                 │
│        - weatherProvider, weatherLocationSource, etc.   │
│        - Dirty state tracking & snapshot comparison     │
└────────────────────────────┬────────────────────────────┘
                             │ (Q_PROPERTY Bindings)
                             ▼
┌─────────────────────────────────────────────────────────┐
│                   WeatherPage.qml                       │
│    - Provider Section (Service, API key)                │
│    - Location Section (Manual / Auto, Search/City)      │
│    - Units Section (Temperature, Wind, Pressure)        │
│    - Display Section (Bar toggle, Compact, Interval)    │
└─────────────────────────────────────────────────────────┘
```

---

## 3. Data Schema & TOML Mapping

The `[weather]` section in `config.toml` is extended with full roundtrip serialization support:

```toml
[weather]
provider = "open-meteo"      # "open-meteo" | "openweathermap"
location_source = "manual"   # "manual" | "auto"
api_key = ""                 # API key for OpenWeatherMap (optional for Open-Meteo)
geo_api_key = ""             # IP geolocation API key (optional)
latitude = 52.2297           # Decimal latitude
longitude = 21.0122          # Decimal longitude
city = "Warsaw, Poland"      # Display label for current location
country = "Poland"           # Country label
units = "metric"             # "metric" | "imperial" | "standard"
temp_unit = "celsius"        # "celsius" | "fahrenheit" | "kelvin"
wind_unit = "kmh"            # "kmh" | "ms" | "mph" | "knots"
pressure_unit = "hpa"        # "hpa" | "mmhg" | "inhg" | "bar"
show_in_bar = true           # Show weather widget in top bar
compact_mode = false         # Compact widget display format
show_feels_like = true       # Display "feels like" temperature
show_location = true         # Display location name in tooltip
refresh_interval = 1800      # Update interval in seconds (e.g. 1800 = 30 mins)
```

---

## 4. UI Layout & Component Design

`WeatherPage.qml` uses the standard `Flickable` + `ColumnLayout` design pattern with 4 distinct `SectionGroup` cards:

1. **Provider Section (`providerSectionFrame`)**
   - **Weather Provider**: `HnComboBox` (Open-Meteo, OpenWeatherMap).
   - **API Key**: `HnTextField` (displayed when OpenWeatherMap provider is selected or configured).

2. **Location Section (`locationSectionFrame`)**
   - **Location Source**: `HnComboBox` (Manual, Auto IP-Geolocation).
   - **City / Coordinates**: `HnTextField` (Search or enter city / coordinates).

3. **Units Section (`unitsSectionFrame`)**
   - **Temperature Unit**: `HnComboBox` (°C Celsius, °F Fahrenheit, K Kelvin).
   - **Wind Speed Unit**: `HnComboBox` (km/h, m/s, mph, knots).
   - **Pressure Unit**: `HnComboBox` (hPa, mmHg, inHg, bar).

4. **Display Section (`displaySectionFrame`)**
   - **Show in Bar**: `HnSwitch` (Show module in top bar).
   - **Compact Mode**: `HnSwitch` (Show minimal weather information).
   - **Show Feels-Like**: `HnSwitch` (Display "feels like" temperature).
   - **Show Location**: `HnSwitch` (Display location name in tooltip).
   - **Update Interval**: `HnComboBox` (10 minutes, 15 minutes, 30 minutes, 1 hour).

---

## 5. Dirty State & Save Lifecycle

`SettingsEditModel` maintains `current_.weather` and `snapshot_.weather`.
Modifying any weather setting recomputes `is_dirty_`.
Clicking **Apply Changes** or **Save & Apply** in the footer invokes `toParsedConfig()`, serializes `[weather]` to `config.toml`, and emits `markSaved()`.
