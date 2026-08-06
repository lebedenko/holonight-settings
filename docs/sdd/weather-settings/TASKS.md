# Weather Settings Page — Implementation Tasks

- [x] **Phase 1: Design & SDD Specifications**
  - [x] Create `docs/sdd/weather-settings/DESIGN.md`
  - [x] Create `docs/sdd/weather-settings/SPEC.md`
  - [x] Create `docs/sdd/weather-settings/TASKS.md`

- [x] **Phase 2: Configuration & C++ Model Updates**
  - [x] Extend `WeatherConfig` in `config_structs.h` with `provider`, `location_source`, `temp_unit`, `wind_unit`, `pressure_unit`, `show_in_bar`, `compact_mode`, `show_feels_like`, `show_location`.
  - [x] Update `parseWeather()` and `ConfigParsers.cpp` to read missing/present defaults.
  - [x] Update `ConfigWriter::write()` to serialize new weather fields to TOML.
  - [x] Add weather properties (Q_PROPERTY, getters, setters, NOTIFY signals) to `SettingsEditModel` (.h & .cpp).
  - [x] Update `toParsedConfig()`, `setFromParsedConfig()`, `recomputeDirty()`, `markSaved()` in `SettingsEditModel`.

- [x] **Phase 3: QML User Interface Implementation**
  - [x] Create `apps/settings/qml/WeatherPage.qml` following `AppearancePage.qml` layout (Flickable + SectionGroups + HnSettingsRows + HnControls).
  - [x] Wire `WeatherPage.qml` in `ContentStack.qml` page source & page properties.

- [x] **Phase 4: Verification & Testing**
  - [x] Add unit tests in `tests/test_settings_app.cpp` for `WeatherPage.qml` layout, property bindings, and edit model updates.
  - [x] Add tests in `tests/test_configwriter_weather_preservation.cpp` for full TOML roundtrip of new weather settings.
  - [x] Run `task test`, `task format-check`, `task tidy`, `task qml-lint`.
