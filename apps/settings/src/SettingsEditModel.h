#pragma once

#include "ThemeConfigFile.h"

#include <QObject>
#include <QtQml/qqml.h>

#include <appearanceconfig.h>
#include <holonight/config.h>
#include <holonight_config/config_parsers.h>

class SettingsEditModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("SettingsEditModel is created by SettingsApplication")

  Q_PROPERTY(QString themeScheme READ themeScheme WRITE setThemeScheme NOTIFY themeSchemeChanged)
  Q_PROPERTY(QString themeAccent READ themeAccent WRITE setThemeAccent NOTIFY themeAccentChanged)
  Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
  Q_PROPERTY(QString uiFont READ uiFont WRITE setUiFont NOTIFY uiFontChanged)
  Q_PROPERTY(int uiFontSize READ uiFontSize WRITE setUiFontSize NOTIFY uiFontSizeChanged)
  Q_PROPERTY(QString fixedFont READ fixedFont WRITE setFixedFont NOTIFY fixedFontChanged)
  Q_PROPERTY(int fixedFontSize READ fixedFontSize WRITE setFixedFontSize NOTIFY fixedFontSizeChanged)
  Q_PROPERTY(int transparency READ transparency WRITE setTransparency NOTIFY transparencyChanged)
  Q_PROPERTY(int blurStrength READ blurStrength WRITE setBlurStrength NOTIFY blurStrengthChanged)
  Q_PROPERTY(int workspaceCount READ workspaceCount WRITE setWorkspaceCount NOTIFY workspaceCountChanged)
  Q_PROPERTY(int trayMaxItems READ trayMaxItems WRITE setTrayMaxItems NOTIFY trayMaxItemsChanged)
  Q_PROPERTY(QString shapeCornerStyle READ shapeCornerStyle WRITE setShapeCornerStyle NOTIFY shapeCornerStyleChanged)
  Q_PROPERTY(qreal shapeScale READ shapeScale WRITE setShapeScale NOTIFY shapeScaleChanged)
  Q_PROPERTY(bool baseRadiusEnabled READ baseRadiusEnabled WRITE setBaseRadiusEnabled NOTIFY baseRadiusEnabledChanged)
  Q_PROPERTY(qreal baseRadius READ baseRadius WRITE setBaseRadius NOTIFY baseRadiusChanged)
  Q_PROPERTY(
      bool baseChamferEnabled READ baseChamferEnabled WRITE setBaseChamferEnabled NOTIFY baseChamferEnabledChanged)
  Q_PROPERTY(qreal baseChamfer READ baseChamfer WRITE setBaseChamfer NOTIFY baseChamferChanged)
  Q_PROPERTY(QString weatherProvider READ weatherProvider WRITE setWeatherProvider NOTIFY weatherProviderChanged)
  Q_PROPERTY(QString weatherLocationSource READ weatherLocationSource WRITE setWeatherLocationSource NOTIFY
                 weatherLocationSourceChanged)
  Q_PROPERTY(QString weatherApiKey READ weatherApiKey WRITE setWeatherApiKey NOTIFY weatherApiKeyChanged)
  Q_PROPERTY(QString weatherCity READ weatherCity WRITE setWeatherCity NOTIFY weatherCityChanged)
  Q_PROPERTY(QString weatherTempUnit READ weatherTempUnit WRITE setWeatherTempUnit NOTIFY weatherTempUnitChanged)
  Q_PROPERTY(QString weatherWindUnit READ weatherWindUnit WRITE setWeatherWindUnit NOTIFY weatherWindUnitChanged)
  Q_PROPERTY(QString weatherPressureUnit READ weatherPressureUnit WRITE setWeatherPressureUnit NOTIFY
                 weatherPressureUnitChanged)
  Q_PROPERTY(bool weatherShowInBar READ weatherShowInBar WRITE setWeatherShowInBar NOTIFY weatherShowInBarChanged)
  Q_PROPERTY(
      bool weatherCompactMode READ weatherCompactMode WRITE setWeatherCompactMode NOTIFY weatherCompactModeChanged)
  Q_PROPERTY(bool weatherShowFeelsLike READ weatherShowFeelsLike WRITE setWeatherShowFeelsLike NOTIFY
                 weatherShowFeelsLikeChanged)
  Q_PROPERTY(
      bool weatherShowLocation READ weatherShowLocation WRITE setWeatherShowLocation NOTIFY weatherShowLocationChanged)
  Q_PROPERTY(int weatherRefreshInterval READ weatherRefreshInterval WRITE setWeatherRefreshInterval NOTIFY
                 weatherRefreshIntervalChanged)
  Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)

 public:
  explicit SettingsEditModel(QObject* parent = nullptr);

  [[nodiscard]] QString themeScheme() const { return current_theme_scheme_; }
  [[nodiscard]] QString themeAccent() const { return current_theme_accent_; }
  [[nodiscard]] QString themeMode() const { return current_theme_mode_; }
  [[nodiscard]] QString uiFont() const { return current_typography_.ui_font; }
  [[nodiscard]] int uiFontSize() const { return current_typography_.base_font_size; }
  [[nodiscard]] QString fixedFont() const { return current_typography_.fixed_font; }
  [[nodiscard]] int fixedFontSize() const { return current_typography_.fixed_font_size; }
  [[nodiscard]] int transparency() const { return current_.appearance.transparency; }
  [[nodiscard]] int blurStrength() const { return current_.appearance.blur_strength; }
  [[nodiscard]] int workspaceCount() const { return current_.bar_workspaces.count; }
  [[nodiscard]] int trayMaxItems() const { return current_.bar_system_tray.max_items; }
  [[nodiscard]] QString shapeCornerStyle() const;
  [[nodiscard]] qreal shapeScale() const { return current_shape_appearance_.shape_scale; }
  [[nodiscard]] bool baseRadiusEnabled() const;
  [[nodiscard]] qreal baseRadius() const;
  [[nodiscard]] bool baseChamferEnabled() const;
  [[nodiscard]] qreal baseChamfer() const;
  [[nodiscard]] QString weatherProvider() const { return current_.weather.provider; }
  [[nodiscard]] QString weatherLocationSource() const { return current_.weather.location_source; }
  [[nodiscard]] QString weatherApiKey() const { return current_.weather.api_key; }
  [[nodiscard]] QString weatherCity() const { return current_.weather.city; }
  [[nodiscard]] QString weatherTempUnit() const { return current_.weather.temp_unit; }
  [[nodiscard]] QString weatherWindUnit() const { return current_.weather.wind_unit; }
  [[nodiscard]] QString weatherPressureUnit() const { return current_.weather.pressure_unit; }
  [[nodiscard]] bool weatherShowInBar() const { return current_.weather.show_in_bar; }
  [[nodiscard]] bool weatherCompactMode() const { return current_.weather.compact_mode; }
  [[nodiscard]] bool weatherShowFeelsLike() const { return current_.weather.show_feels_like; }
  [[nodiscard]] bool weatherShowLocation() const { return current_.weather.show_location; }
  [[nodiscard]] int weatherRefreshInterval() const { return current_.weather.refresh_interval; }
  [[nodiscard]] bool isDirty() const { return is_dirty_; }

  void setThemeScheme(const QString& value);
  void setThemeAccent(const QString& value);
  void setThemeMode(const QString& value);
  void setUiFont(const QString& value);
  void setUiFontSize(int value);
  void setFixedFont(const QString& value);
  void setFixedFontSize(int value);
  void setTransparency(int value);
  void setBlurStrength(int value);
  void setWorkspaceCount(int value);
  void setTrayMaxItems(int value);
  void setShapeCornerStyle(const QString& value);
  void setShapeScale(qreal value);
  void setBaseRadiusEnabled(bool enabled);
  void setBaseRadius(qreal value);
  void setBaseChamferEnabled(bool enabled);
  void setBaseChamfer(qreal value);
  void setWeatherProvider(const QString& value);
  void setWeatherLocationSource(const QString& value);
  void setWeatherApiKey(const QString& value);
  void setWeatherCity(const QString& value);
  void setWeatherTempUnit(const QString& value);
  void setWeatherWindUnit(const QString& value);
  void setWeatherPressureUnit(const QString& value);
  void setWeatherShowInBar(bool value);
  void setWeatherCompactMode(bool value);
  void setWeatherShowFeelsLike(bool value);
  void setWeatherShowLocation(bool value);
  void setWeatherRefreshInterval(int value);

  void setFromParsedConfig(const ParsedConfig& config);
  void setThemeConfigSnapshot(const Holonight::ThemeConfig& config);
  void setThemeAppearanceSnapshot(const ThemeConfigFile::Appearance& appearance);
  void setShapeAppearanceSnapshot(const Holonight::AppearanceConfig& appearance);
  void setThemeModeSnapshot(const QString& mode);
  void markSaved(const ParsedConfig& config, const Holonight::ThemeConfig& theme_config,
                 const Holonight::AppearanceConfig& shape_appearance);
  [[nodiscard]] ParsedConfig toParsedConfig() const;
  [[nodiscard]] Holonight::ThemeConfig toThemeConfig() const;
  [[nodiscard]] Holonight::AppearanceConfig toShapeAppearanceConfig() const;

 Q_SIGNALS:
  void themeSchemeChanged();
  void themeAccentChanged();
  void themeModeChanged();
  void uiFontChanged();
  void uiFontSizeChanged();
  void fixedFontChanged();
  void fixedFontSizeChanged();
  void transparencyChanged();
  void blurStrengthChanged();
  void workspaceCountChanged();
  void trayMaxItemsChanged();
  void shapeCornerStyleChanged();
  void shapeScaleChanged();
  void baseRadiusEnabledChanged();
  void baseRadiusChanged();
  void baseChamferEnabledChanged();
  void baseChamferChanged();
  void weatherProviderChanged();
  void weatherLocationSourceChanged();
  void weatherApiKeyChanged();
  void weatherCityChanged();
  void weatherTempUnitChanged();
  void weatherWindUnitChanged();
  void weatherPressureUnitChanged();
  void weatherShowInBarChanged();
  void weatherCompactModeChanged();
  void weatherShowFeelsLikeChanged();
  void weatherShowLocationChanged();
  void weatherRefreshIntervalChanged();
  void isDirtyChanged();

 private:
  void recomputeDirty();
  void updateModeFromScheme();

  ParsedConfig current_;
  ParsedConfig snapshot_;
  Holonight::ThemeConfig current_typography_{Holonight::ThemeConfig::defaults()};
  Holonight::ThemeConfig snapshot_typography_{Holonight::ThemeConfig::defaults()};
  QString current_theme_scheme_{QStringLiteral("holonight-dark")};
  QString snapshot_theme_scheme_{QStringLiteral("holonight-dark")};
  QString current_theme_accent_{ThemeConfigFile::defaultAccent()};
  QString snapshot_theme_accent_{ThemeConfigFile::defaultAccent()};
  QString current_theme_mode_{QStringLiteral("dark")};
  QString snapshot_theme_mode_{QStringLiteral("dark")};
  Holonight::AppearanceConfig current_shape_appearance_;
  Holonight::AppearanceConfig snapshot_shape_appearance_;
  bool is_dirty_{false};
};
