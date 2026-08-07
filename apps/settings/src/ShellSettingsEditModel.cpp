#include "ShellSettingsEditModel.h"

#include <algorithm>

ShellSettingsEditModel::ShellSettingsEditModel(QObject* parent) : QObject(parent) {}
int ShellSettingsEditModel::workspaceCount() const { return current_.bar_workspaces.count; }
int ShellSettingsEditModel::trayMaxItems() const { return current_.bar_system_tray.max_items; }
QString ShellSettingsEditModel::weatherProvider() const { return current_.weather.provider; }
QString ShellSettingsEditModel::weatherLocationSource() const { return current_.weather.location_source; }
QString ShellSettingsEditModel::weatherApiKey() const { return current_.weather.api_key; }
QString ShellSettingsEditModel::weatherCity() const { return current_.weather.city; }
QString ShellSettingsEditModel::weatherTempUnit() const { return current_.weather.temp_unit; }
QString ShellSettingsEditModel::weatherWindUnit() const { return current_.weather.wind_unit; }
QString ShellSettingsEditModel::weatherPressureUnit() const { return current_.weather.pressure_unit; }
bool ShellSettingsEditModel::weatherShowInBar() const { return current_.weather.show_in_bar; }
bool ShellSettingsEditModel::weatherCompactMode() const { return current_.weather.compact_mode; }
bool ShellSettingsEditModel::weatherShowFeelsLike() const { return current_.weather.show_feels_like; }
bool ShellSettingsEditModel::weatherShowLocation() const { return current_.weather.show_location; }
int ShellSettingsEditModel::weatherRefreshInterval() const { return current_.weather.refresh_interval; }

void ShellSettingsEditModel::changed(void (ShellSettingsEditModel::*signal)(), bool was_dirty) {
  emit(this->*signal)();
  if (was_dirty != isDirty()) {
    emit isDirtyChanged();
  }
}
void ShellSettingsEditModel::setworkspaceCount(int value) {
  const bool was_dirty = isDirty();
  value = std::clamp(value, 3, 10);
  if (current_.bar_workspaces.count == value) {
    return;
  }
  current_.bar_workspaces.count = value;
  changed(&ShellSettingsEditModel::workspaceCountChanged, was_dirty);
}
void ShellSettingsEditModel::settrayMaxItems(int value) {
  const bool was_dirty = isDirty();
  value = std::clamp(value, 2, 5);
  if (current_.bar_system_tray.max_items == value) {
    return;
  }
  current_.bar_system_tray.max_items = value;
  changed(&ShellSettingsEditModel::trayMaxItemsChanged, was_dirty);
}
#define STRING_SETTER(name, member)                             \
  void ShellSettingsEditModel::set##name(QString value) {       \
    const bool was_dirty = isDirty();                           \
    if (member == value) {                                      \
      return;                                                   \
    }                                                           \
    member = std::move(value);                                  \
    changed(&ShellSettingsEditModel::name##Changed, was_dirty); \
  }
STRING_SETTER(weatherProvider, current_.weather.provider)
STRING_SETTER(weatherLocationSource, current_.weather.location_source)
STRING_SETTER(weatherApiKey, current_.weather.api_key)
STRING_SETTER(weatherCity, current_.weather.city)
STRING_SETTER(weatherTempUnit, current_.weather.temp_unit)
STRING_SETTER(weatherWindUnit, current_.weather.wind_unit)
STRING_SETTER(weatherPressureUnit, current_.weather.pressure_unit)
#undef STRING_SETTER
#define BOOL_SETTER(name, member)                               \
  void ShellSettingsEditModel::set##name(bool value) {          \
    const bool was_dirty = isDirty();                           \
    if (member == value) {                                      \
      return;                                                   \
    }                                                           \
    member = value;                                             \
    changed(&ShellSettingsEditModel::name##Changed, was_dirty); \
  }
BOOL_SETTER(weatherShowInBar, current_.weather.show_in_bar)
BOOL_SETTER(weatherCompactMode, current_.weather.compact_mode)
BOOL_SETTER(weatherShowFeelsLike, current_.weather.show_feels_like)
BOOL_SETTER(weatherShowLocation, current_.weather.show_location)
#undef BOOL_SETTER
void ShellSettingsEditModel::setweatherRefreshInterval(int value) {
  const bool was_dirty = isDirty();
  value = std::max(1, value);
  if (current_.weather.refresh_interval == value) {
    return;
  }
  current_.weather.refresh_interval = value;
  changed(&ShellSettingsEditModel::weatherRefreshIntervalChanged, was_dirty);
}

void ShellSettingsEditModel::load(const HoloNight::ShellConfig::ProductConfig& value) {
  const bool dirty = isDirty();
  current_ = value;
  snapshot_ = value;
  emit workspaceCountChanged();
  emit trayMaxItemsChanged();
  emit weatherProviderChanged();
  emit weatherLocationSourceChanged();
  emit weatherApiKeyChanged();
  emit weatherCityChanged();
  emit weatherTempUnitChanged();
  emit weatherWindUnitChanged();
  emit weatherPressureUnitChanged();
  emit weatherShowInBarChanged();
  emit weatherCompactModeChanged();
  emit weatherShowFeelsLikeChanged();
  emit weatherShowLocationChanged();
  emit weatherRefreshIntervalChanged();
  if (dirty) {
    emit isDirtyChanged();
  }
}
void ShellSettingsEditModel::markSaved() {
  const bool dirty = isDirty();
  snapshot_ = current_;
  if (dirty) {
    emit isDirtyChanged();
  }
}
