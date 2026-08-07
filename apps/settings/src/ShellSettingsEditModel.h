#pragma once

#include <QObject>
#include <QtQml/qqml.h>

#include <holonight_shell_config/config_parsers.h>

class ShellSettingsEditModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("ShellSettingsEditModel is created by SettingsApplication")

#define SHELL_PROPERTY(type, name) Q_PROPERTY(type name READ name WRITE set##name NOTIFY name##Changed)
  SHELL_PROPERTY(int, workspaceCount)
  SHELL_PROPERTY(int, trayMaxItems)
  SHELL_PROPERTY(QString, weatherProvider)
  SHELL_PROPERTY(QString, weatherLocationSource)
  SHELL_PROPERTY(QString, weatherApiKey)
  SHELL_PROPERTY(QString, weatherCity)
  SHELL_PROPERTY(QString, weatherTempUnit)
  SHELL_PROPERTY(QString, weatherWindUnit)
  SHELL_PROPERTY(QString, weatherPressureUnit)
  SHELL_PROPERTY(bool, weatherShowInBar)
  SHELL_PROPERTY(bool, weatherCompactMode)
  SHELL_PROPERTY(bool, weatherShowFeelsLike)
  SHELL_PROPERTY(bool, weatherShowLocation)
  SHELL_PROPERTY(int, weatherRefreshInterval)
#undef SHELL_PROPERTY
  Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)

 public:
  explicit ShellSettingsEditModel(QObject* parent = nullptr);
#define SHELL_ACCESSORS(type, name) \
  type name() const;                \
  void set##name(type value);
  SHELL_ACCESSORS(int, workspaceCount)
  SHELL_ACCESSORS(int, trayMaxItems)
  SHELL_ACCESSORS(QString, weatherProvider)
  SHELL_ACCESSORS(QString, weatherLocationSource)
  SHELL_ACCESSORS(QString, weatherApiKey)
  SHELL_ACCESSORS(QString, weatherCity)
  SHELL_ACCESSORS(QString, weatherTempUnit)
  SHELL_ACCESSORS(QString, weatherWindUnit)
  SHELL_ACCESSORS(QString, weatherPressureUnit)
  SHELL_ACCESSORS(bool, weatherShowInBar)
  SHELL_ACCESSORS(bool, weatherCompactMode)
  SHELL_ACCESSORS(bool, weatherShowFeelsLike)
  SHELL_ACCESSORS(bool, weatherShowLocation)
  SHELL_ACCESSORS(int, weatherRefreshInterval)
#undef SHELL_ACCESSORS
  [[nodiscard]] bool isDirty() const { return current_ != snapshot_; }
  [[nodiscard]] const HoloNight::ShellConfig::ProductConfig& value() const { return current_; }
  void load(const HoloNight::ShellConfig::ProductConfig& value);
  void markSaved();

 Q_SIGNALS:
#define SHELL_SIGNAL(name) void name##Changed();
  SHELL_SIGNAL(workspaceCount)
  SHELL_SIGNAL(trayMaxItems)
  SHELL_SIGNAL(weatherProvider)
  SHELL_SIGNAL(weatherLocationSource)
  SHELL_SIGNAL(weatherApiKey)
  SHELL_SIGNAL(weatherCity)
  SHELL_SIGNAL(weatherTempUnit)
  SHELL_SIGNAL(weatherWindUnit)
  SHELL_SIGNAL(weatherPressureUnit)
  SHELL_SIGNAL(weatherShowInBar)
  SHELL_SIGNAL(weatherCompactMode)
  SHELL_SIGNAL(weatherShowFeelsLike)
  SHELL_SIGNAL(weatherShowLocation)
  SHELL_SIGNAL(weatherRefreshInterval)
#undef SHELL_SIGNAL
  void isDirtyChanged();

 private:
  void changed(void (ShellSettingsEditModel::*signal)(), bool was_dirty);
  HoloNight::ShellConfig::ProductConfig current_;
  HoloNight::ShellConfig::ProductConfig snapshot_;
};
