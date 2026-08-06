#include "SettingsEditModel.h"

#include "ThemeConfigFile.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kMinFontSize = 8;
constexpr int kMaxFontSize = 18;
constexpr int kMinWorkspaceCount = 3;
constexpr int kMaxWorkspaceCount = 10;
constexpr int kMinTrayMaxItems = 2;
constexpr int kMaxTrayMaxItems = 5;
}  // namespace

SettingsEditModel::SettingsEditModel(QObject* parent) : QObject(parent) {}

void SettingsEditModel::setFromParsedConfig(const ParsedConfig& config) {
  const ParsedConfig previous = current_;
  current_ = config;
  snapshot_ = config;
  if (previous.appearance.transparency != config.appearance.transparency) {
    emit transparencyChanged();
  }
  if (previous.appearance.blur_strength != config.appearance.blur_strength) {
    emit blurStrengthChanged();
  }
  if (previous.bar_workspaces.count != config.bar_workspaces.count) {
    emit workspaceCountChanged();
  }
  if (previous.bar_system_tray.max_items != config.bar_system_tray.max_items) {
    emit trayMaxItemsChanged();
  }
  if (previous.weather.provider != config.weather.provider) emit weatherProviderChanged();
  if (previous.weather.location_source != config.weather.location_source) emit weatherLocationSourceChanged();
  if (previous.weather.api_key != config.weather.api_key) emit weatherApiKeyChanged();
  if (previous.weather.city != config.weather.city) emit weatherCityChanged();
  if (previous.weather.temp_unit != config.weather.temp_unit) emit weatherTempUnitChanged();
  if (previous.weather.wind_unit != config.weather.wind_unit) emit weatherWindUnitChanged();
  if (previous.weather.pressure_unit != config.weather.pressure_unit) emit weatherPressureUnitChanged();
  if (previous.weather.show_in_bar != config.weather.show_in_bar) emit weatherShowInBarChanged();
  if (previous.weather.compact_mode != config.weather.compact_mode) emit weatherCompactModeChanged();
  if (previous.weather.show_feels_like != config.weather.show_feels_like) emit weatherShowFeelsLikeChanged();
  if (previous.weather.show_location != config.weather.show_location) emit weatherShowLocationChanged();
  if (previous.weather.refresh_interval != config.weather.refresh_interval) emit weatherRefreshIntervalChanged();
  recomputeDirty();
}

void SettingsEditModel::setThemeConfigSnapshot(const Holonight::ThemeConfig& config) {
  const Holonight::ThemeConfig previous = current_typography_;
  current_typography_ = config;
  current_typography_.base_font_size = std::clamp(config.base_font_size, kMinFontSize, kMaxFontSize);
  current_typography_.fixed_font_size = std::clamp(config.fixed_font_size, kMinFontSize, kMaxFontSize);
  snapshot_typography_ = current_typography_;
  if (previous.ui_font != current_typography_.ui_font) emit uiFontChanged();
  if (previous.base_font_size != current_typography_.base_font_size) emit uiFontSizeChanged();
  if (previous.fixed_font != current_typography_.fixed_font) emit fixedFontChanged();
  if (previous.fixed_font_size != current_typography_.fixed_font_size) emit fixedFontSizeChanged();
  setThemeAppearanceSnapshot(ThemeConfigFile::Appearance{.scheme = config.scheme, .accent = config.accent});
  recomputeDirty();
}

void SettingsEditModel::setThemeModeSnapshot(const QString& mode) {
  const QString scheme = ThemeConfigFile::schemeForMode(mode);
  setThemeAppearanceSnapshot(
      ThemeConfigFile::Appearance{.scheme = scheme, .accent = ThemeConfigFile::defaultAccent(), .mode = mode});
}

void SettingsEditModel::setThemeAppearanceSnapshot(const ThemeConfigFile::Appearance& appearance) {
  QString scheme = ThemeConfigFile::normalizeScheme(appearance.scheme);
  if (scheme.isEmpty()) {
    scheme = ThemeConfigFile::defaultScheme();
  }
  const QString accent = ThemeConfigFile::normalizeAccent(appearance.accent);
  const QString mode = ThemeConfigFile::modeForScheme(scheme);
  if (current_theme_scheme_ == scheme && snapshot_theme_scheme_ == scheme && current_theme_accent_ == accent &&
      snapshot_theme_accent_ == accent && current_theme_mode_ == mode && snapshot_theme_mode_ == mode) {
    return;
  }

  current_theme_scheme_ = scheme;
  snapshot_theme_scheme_ = scheme;
  current_theme_accent_ = accent;
  snapshot_theme_accent_ = accent;
  current_theme_mode_ = mode;
  snapshot_theme_mode_ = mode;
  emit themeSchemeChanged();
  emit themeAccentChanged();
  emit themeModeChanged();
  recomputeDirty();
}

ParsedConfig SettingsEditModel::toParsedConfig() const { return current_; }

Holonight::ThemeConfig SettingsEditModel::toThemeConfig() const {
  Holonight::ThemeConfig config = current_typography_;
  config.scheme = current_theme_scheme_;
  config.accent = current_theme_accent_;
  config.appearance_mode = current_theme_mode_ == QLatin1String("light") ? Holonight::AppearanceMode::Light
                                                                         : Holonight::AppearanceMode::Dark;
  return config;
}

Holonight::AppearanceConfig SettingsEditModel::toShapeAppearanceConfig() const { return current_shape_appearance_; }

void SettingsEditModel::markSaved(const ParsedConfig& config, const Holonight::ThemeConfig& theme_config,
                                  const Holonight::AppearanceConfig& shape_appearance) {
  snapshot_ = config;
  snapshot_typography_ = current_typography_;
  snapshot_theme_scheme_ = ThemeConfigFile::normalizeScheme(theme_config.scheme);
  if (snapshot_theme_scheme_.isEmpty()) {
    snapshot_theme_scheme_ = ThemeConfigFile::defaultScheme();
  }
  snapshot_theme_accent_ = ThemeConfigFile::normalizeAccent(theme_config.accent);
  snapshot_theme_mode_ = ThemeConfigFile::modeForScheme(snapshot_theme_scheme_);
  snapshot_shape_appearance_ = shape_appearance.normalized();
  recomputeDirty();
}

void SettingsEditModel::recomputeDirty() {
  bool dirty = !(current_ == snapshot_) || current_theme_scheme_ != snapshot_theme_scheme_ ||
               current_theme_accent_ != snapshot_theme_accent_ || current_theme_mode_ != snapshot_theme_mode_ ||
               current_typography_.ui_font != snapshot_typography_.ui_font ||
               current_typography_.fixed_font != snapshot_typography_.fixed_font ||
               current_typography_.base_font_size != snapshot_typography_.base_font_size ||
               current_typography_.fixed_font_size != snapshot_typography_.fixed_font_size ||
               current_shape_appearance_ != snapshot_shape_appearance_;
  if (dirty != is_dirty_) {
    is_dirty_ = dirty;
    emit isDirtyChanged();
  }
}

void SettingsEditModel::updateModeFromScheme() {
  const QString mode = ThemeConfigFile::modeForScheme(current_theme_scheme_);
  if (current_theme_mode_ == mode) {
    return;
  }
  current_theme_mode_ = mode;
  emit themeModeChanged();
}

void SettingsEditModel::setThemeScheme(const QString& value) {
  QString normalized = ThemeConfigFile::normalizeScheme(value);
  if (normalized.isEmpty()) {
    normalized = ThemeConfigFile::defaultScheme();
  }
  if (current_theme_scheme_ == normalized) {
    return;
  }
  current_theme_scheme_ = normalized;
  emit themeSchemeChanged();
  updateModeFromScheme();
  recomputeDirty();
}

void SettingsEditModel::setThemeAccent(const QString& value) {
  const QString normalized = ThemeConfigFile::normalizeAccent(value);
  if (current_theme_accent_ == normalized) {
    return;
  }
  current_theme_accent_ = normalized;
  emit themeAccentChanged();
  recomputeDirty();
}

void SettingsEditModel::setThemeMode(const QString& value) {
  setThemeScheme(ThemeConfigFile::siblingSchemeForMode(current_theme_scheme_, ThemeConfigFile::normalizeMode(value)));
}

void SettingsEditModel::setUiFont(const QString& value) {
  if (current_typography_.ui_font == value) {
    return;
  }
  current_typography_.ui_font = value;
  emit uiFontChanged();
  recomputeDirty();
}

void SettingsEditModel::setUiFontSize(int value) {
  const int clamped_value = std::clamp(value, kMinFontSize, kMaxFontSize);
  if (current_typography_.base_font_size == clamped_value) {
    return;
  }
  current_typography_.base_font_size = clamped_value;
  emit uiFontSizeChanged();
  recomputeDirty();
}

void SettingsEditModel::setFixedFont(const QString& value) {
  if (current_typography_.fixed_font == value) {
    return;
  }
  current_typography_.fixed_font = value;
  emit fixedFontChanged();
  recomputeDirty();
}

void SettingsEditModel::setFixedFontSize(int value) {
  const int clamped_value = std::clamp(value, kMinFontSize, kMaxFontSize);
  if (current_typography_.fixed_font_size == clamped_value) {
    return;
  }
  current_typography_.fixed_font_size = clamped_value;
  emit fixedFontSizeChanged();
  recomputeDirty();
}

void SettingsEditModel::setTransparency(int value) {
  const int clamped_value = std::clamp(value, AppearanceConfig::kMinTransparency, AppearanceConfig::kMaxTransparency);
  if (current_.appearance.transparency == clamped_value) {
    return;
  }
  current_.appearance.transparency = clamped_value;
  emit transparencyChanged();
  recomputeDirty();
}

void SettingsEditModel::setBlurStrength(int value) {
  const int clamped_value = std::clamp(value, AppearanceConfig::kMinBlurStrength, AppearanceConfig::kMaxBlurStrength);
  if (current_.appearance.blur_strength == clamped_value) {
    return;
  }
  current_.appearance.blur_strength = clamped_value;
  emit blurStrengthChanged();
  recomputeDirty();
}

void SettingsEditModel::setWorkspaceCount(int value) {
  const int clamped_value = std::clamp(value, kMinWorkspaceCount, kMaxWorkspaceCount);
  if (current_.bar_workspaces.count == clamped_value) {
    return;
  }
  current_.bar_workspaces.count = clamped_value;
  emit workspaceCountChanged();
  recomputeDirty();
}

void SettingsEditModel::setTrayMaxItems(int value) {
  const int clamped_value = std::clamp(value, kMinTrayMaxItems, kMaxTrayMaxItems);
  if (current_.bar_system_tray.max_items == clamped_value) {
    return;
  }
  current_.bar_system_tray.max_items = clamped_value;
  emit trayMaxItemsChanged();
  recomputeDirty();
}

QString SettingsEditModel::shapeCornerStyle() const {
  return Holonight::AppearanceConfig::cornerStyleName(current_shape_appearance_.corner_style);
}

bool SettingsEditModel::baseRadiusEnabled() const { return std::isfinite(current_shape_appearance_.base_radius); }

qreal SettingsEditModel::baseRadius() const {
  return baseRadiusEnabled() ? current_shape_appearance_.base_radius : 0.0;
}

bool SettingsEditModel::baseChamferEnabled() const { return std::isfinite(current_shape_appearance_.base_chamfer); }

qreal SettingsEditModel::baseChamfer() const {
  return baseChamferEnabled() ? current_shape_appearance_.base_chamfer : 0.0;
}

void SettingsEditModel::setShapeAppearanceSnapshot(const Holonight::AppearanceConfig& appearance) {
  const Holonight::AppearanceConfig normalized = appearance.normalized();
  const Holonight::AppearanceConfig previous = current_shape_appearance_;
  current_shape_appearance_ = normalized;
  snapshot_shape_appearance_ = normalized;

  if (previous.corner_style != normalized.corner_style) {
    emit shapeCornerStyleChanged();
  }
  if (previous.shape_scale != normalized.shape_scale) {
    emit shapeScaleChanged();
  }
  if (std::isfinite(previous.base_radius) != std::isfinite(normalized.base_radius)) {
    emit baseRadiusEnabledChanged();
  }
  if ((std::isnan(previous.base_radius) != std::isnan(normalized.base_radius)) ||
      (std::isfinite(previous.base_radius) && previous.base_radius != normalized.base_radius)) {
    emit baseRadiusChanged();
  }
  if (std::isfinite(previous.base_chamfer) != std::isfinite(normalized.base_chamfer)) {
    emit baseChamferEnabledChanged();
  }
  if ((std::isnan(previous.base_chamfer) != std::isnan(normalized.base_chamfer)) ||
      (std::isfinite(previous.base_chamfer) && previous.base_chamfer != normalized.base_chamfer)) {
    emit baseChamferChanged();
  }
  recomputeDirty();
}

void SettingsEditModel::setShapeCornerStyle(const QString& value) {
  const Holonight::CornerStyle style = Holonight::AppearanceConfig::cornerStyleFromName(value);
  if (current_shape_appearance_.corner_style == style) {
    return;
  }
  current_shape_appearance_.corner_style = style;
  emit shapeCornerStyleChanged();
  recomputeDirty();
}

void SettingsEditModel::setShapeScale(qreal value) {
  const qreal clamped_value = std::isfinite(value) ? std::clamp(value, Holonight::AppearanceConfig::minimumShapeScale(),
                                                                Holonight::AppearanceConfig::maximumShapeScale())
                                                   : Holonight::AppearanceConfig::defaults().shape_scale;
  if (current_shape_appearance_.shape_scale == clamped_value) {
    return;
  }
  current_shape_appearance_.shape_scale = clamped_value;
  emit shapeScaleChanged();
  recomputeDirty();
}

void SettingsEditModel::setBaseRadiusEnabled(bool enabled) {
  if (baseRadiusEnabled() == enabled) {
    return;
  }
  current_shape_appearance_.base_radius = enabled ? 0.0 : qQNaN();
  emit baseRadiusEnabledChanged();
  emit baseRadiusChanged();
  recomputeDirty();
}

void SettingsEditModel::setBaseRadius(qreal value) {
  const qreal clamped_value =
      std::isfinite(value) ? std::clamp(value, 0.0, Holonight::AppearanceConfig::maximumBaseExtent()) : qQNaN();
  const bool was_enabled = baseRadiusEnabled();
  if ((std::isnan(current_shape_appearance_.base_radius) && std::isnan(clamped_value)) ||
      (was_enabled && current_shape_appearance_.base_radius == clamped_value)) {
    return;
  }
  current_shape_appearance_.base_radius = clamped_value;
  if (was_enabled != std::isfinite(clamped_value)) {
    emit baseRadiusEnabledChanged();
  }
  emit baseRadiusChanged();
  recomputeDirty();
}

void SettingsEditModel::setBaseChamferEnabled(bool enabled) {
  if (baseChamferEnabled() == enabled) {
    return;
  }
  current_shape_appearance_.base_chamfer = enabled ? 0.0 : qQNaN();
  emit baseChamferEnabledChanged();
  emit baseChamferChanged();
  recomputeDirty();
}

void SettingsEditModel::setBaseChamfer(qreal value) {
  const qreal clamped_value =
      std::isfinite(value) ? std::clamp(value, 0.0, Holonight::AppearanceConfig::maximumBaseExtent()) : qQNaN();
  const bool was_enabled = baseChamferEnabled();
  if ((std::isnan(current_shape_appearance_.base_chamfer) && std::isnan(clamped_value)) ||
      (was_enabled && current_shape_appearance_.base_chamfer == clamped_value)) {
    return;
  }
  current_shape_appearance_.base_chamfer = clamped_value;
  if (was_enabled != std::isfinite(clamped_value)) {
    emit baseChamferEnabledChanged();
  }
  emit baseChamferChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherProvider(const QString& value) {
  if (current_.weather.provider == value) return;
  current_.weather.provider = value;
  emit weatherProviderChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherLocationSource(const QString& value) {
  if (current_.weather.location_source == value) return;
  current_.weather.location_source = value;
  emit weatherLocationSourceChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherApiKey(const QString& value) {
  if (current_.weather.api_key == value) return;
  current_.weather.api_key = value;
  emit weatherApiKeyChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherCity(const QString& value) {
  if (current_.weather.city == value) return;
  current_.weather.city = value;
  emit weatherCityChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherTempUnit(const QString& value) {
  if (current_.weather.temp_unit == value) return;
  current_.weather.temp_unit = value;
  emit weatherTempUnitChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherWindUnit(const QString& value) {
  if (current_.weather.wind_unit == value) return;
  current_.weather.wind_unit = value;
  emit weatherWindUnitChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherPressureUnit(const QString& value) {
  if (current_.weather.pressure_unit == value) return;
  current_.weather.pressure_unit = value;
  emit weatherPressureUnitChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherShowInBar(bool value) {
  if (current_.weather.show_in_bar == value) return;
  current_.weather.show_in_bar = value;
  emit weatherShowInBarChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherCompactMode(bool value) {
  if (current_.weather.compact_mode == value) return;
  current_.weather.compact_mode = value;
  emit weatherCompactModeChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherShowFeelsLike(bool value) {
  if (current_.weather.show_feels_like == value) return;
  current_.weather.show_feels_like = value;
  emit weatherShowFeelsLikeChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherShowLocation(bool value) {
  if (current_.weather.show_location == value) return;
  current_.weather.show_location = value;
  emit weatherShowLocationChanged();
  recomputeDirty();
}

void SettingsEditModel::setWeatherRefreshInterval(int value) {
  if (current_.weather.refresh_interval == value) return;
  current_.weather.refresh_interval = value;
  emit weatherRefreshIntervalChanged();
  recomputeDirty();
}
