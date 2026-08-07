#include "AppearanceEditModel.h"

#include <algorithm>
#include <holonight/appearance.h>
#include <holonight/theme_catalog.h>

namespace {
QString fromStd(const std::string& value) { return QString::fromStdString(value); }
std::string toStd(const QString& value) { return value.toStdString(); }
}  // namespace

AppearanceEditModel::AppearanceEditModel(QObject* parent) : QObject(parent) {}
QString AppearanceEditModel::themeScheme() const { return fromStd(current_.theme.scheme); }
QString AppearanceEditModel::themeAccent() const { return fromStd(current_.theme.accent); }
QString AppearanceEditModel::themeMode() const { return Holonight::modeNameForScheme(themeScheme()); }
QString AppearanceEditModel::uiFont() const { return fromStd(current_.typography.ui_family); }
int AppearanceEditModel::uiFontSize() const { return static_cast<int>(current_.typography.ui_size); }
QString AppearanceEditModel::monospaceFont() const { return fromStd(current_.typography.monospace_family); }
int AppearanceEditModel::monospaceFontSize() const { return static_cast<int>(current_.typography.monospace_size); }
QString AppearanceEditModel::titleFont() const { return fromStd(current_.typography.title_family); }
int AppearanceEditModel::titleFontSize() const { return static_cast<int>(current_.typography.title_size); }
QString AppearanceEditModel::displayFont() const { return fromStd(current_.typography.display_family); }
int AppearanceEditModel::displayFontSize() const { return static_cast<int>(current_.typography.display_size); }
QString AppearanceEditModel::iconTheme() const { return fromStd(current_.icons.theme); }
QString AppearanceEditModel::fallbackIconTheme() const { return fromStd(current_.icons.fallback); }
QString AppearanceEditModel::cursorTheme() const { return fromStd(current_.icons.cursor); }
qreal AppearanceEditModel::layoutScale() const { return current_.layout.scale; }
QString AppearanceEditModel::shapeStyle() const {
  return QString::fromUtf8(HoloNight::Config::shapeStyleName(current_.shape.style));
}
qreal AppearanceEditModel::shapeScale() const { return current_.shape.scale; }
bool AppearanceEditModel::baseRadiusEnabled() const { return current_.shape.base_radius.has_value(); }
qreal AppearanceEditModel::baseRadius() const { return current_.shape.base_radius.value_or(0.0); }
bool AppearanceEditModel::baseChamferEnabled() const { return current_.shape.base_chamfer.has_value(); }
qreal AppearanceEditModel::baseChamfer() const { return current_.shape.base_chamfer.value_or(0.0); }

QString AppearanceEditModel::sibling(const QString& mode) const {
  const auto* selected = Holonight::themeVariantForSchemeId(themeScheme());
  if (selected == nullptr) {
    return {};
  }
  for (const auto& variant : Holonight::themeVariants()) {
    if (variant.family_id == selected->family_id && Holonight::modeNameForScheme(variant.id) == mode) {
      return variant.id;
    }
  }
  return {};
}
bool AppearanceEditModel::lightModeAvailable() const { return !sibling(QStringLiteral("light")).isEmpty(); }
bool AppearanceEditModel::darkModeAvailable() const { return !sibling(QStringLiteral("dark")).isEmpty(); }

void AppearanceEditModel::changed(void (AppearanceEditModel::*signal)(), bool was_dirty) {
  emit(this->*signal)();
  if (was_dirty != isDirty()) {
    emit isDirtyChanged();
  }
  setValidationError({});
}
#define STRING_SETTER(method, member, signal)              \
  void AppearanceEditModel::method(const QString& value) { \
    const bool dirty = isDirty();                          \
    if (member == toStd(value)) {                          \
      return;                                              \
    }                                                      \
    member = toStd(value);                                 \
    changed(&AppearanceEditModel::signal##Changed, dirty); \
  }
STRING_SETTER(setThemeAccent, current_.theme.accent, themeAccent)
STRING_SETTER(setUiFont, current_.typography.ui_family, uiFont)
STRING_SETTER(setMonospaceFont, current_.typography.monospace_family, monospaceFont)
STRING_SETTER(setTitleFont, current_.typography.title_family, titleFont)
STRING_SETTER(setDisplayFont, current_.typography.display_family, displayFont)
STRING_SETTER(setIconTheme, current_.icons.theme, iconTheme)
STRING_SETTER(setFallbackIconTheme, current_.icons.fallback, fallbackIconTheme)
STRING_SETTER(setCursorTheme, current_.icons.cursor, cursorTheme)
#undef STRING_SETTER

void AppearanceEditModel::setThemeScheme(const QString& value) {
  const bool dirty = isDirty();
  if (current_.theme.scheme == toStd(value)) {
    return;
  }
  current_.theme.scheme = toStd(value);
  emit themeSchemeChanged();
  emit themeModeChanged();
  if (dirty != isDirty()) {
    emit isDirtyChanged();
  }
  setValidationError({});
}
void AppearanceEditModel::setThemeMode(const QString& value) {
  const QString target = sibling(value);
  if (!target.isEmpty()) {
    setThemeScheme(target);
  }
}
#define INT_SETTER(method, member, signal)                 \
  void AppearanceEditModel::method(int value) {            \
    const bool dirty = isDirty();                          \
    value = std::clamp(value, 6, 48);                      \
    if (member == value) {                                 \
      return;                                              \
    }                                                      \
    member = value;                                        \
    changed(&AppearanceEditModel::signal##Changed, dirty); \
  }
INT_SETTER(setUiFontSize, current_.typography.ui_size, uiFontSize)
INT_SETTER(setMonospaceFontSize, current_.typography.monospace_size, monospaceFontSize)
INT_SETTER(setTitleFontSize, current_.typography.title_size, titleFontSize)
INT_SETTER(setDisplayFontSize, current_.typography.display_size, displayFontSize)
#undef INT_SETTER

void AppearanceEditModel::setLayoutScale(qreal value) {
  const bool dirty = isDirty();
  value = std::clamp(value, 0.5, 3.0);
  if (current_.layout.scale == value) {
    return;
  }
  current_.layout.scale = value;
  changed(&AppearanceEditModel::layoutScaleChanged, dirty);
}
void AppearanceEditModel::setShapeStyle(const QString& value) {
  const auto style = HoloNight::Config::shapeStyleFromName(value.toStdString());
  if (!style || current_.shape.style == *style) {
    return;
  }
  const bool dirty = isDirty();
  current_.shape.style = *style;
  changed(&AppearanceEditModel::shapeStyleChanged, dirty);
}
void AppearanceEditModel::setShapeScale(qreal value) {
  const bool dirty = isDirty();
  value = std::clamp(value, 0.25, 4.0);
  if (current_.shape.scale == value) {
    return;
  }
  current_.shape.scale = value;
  changed(&AppearanceEditModel::shapeScaleChanged, dirty);
}
void AppearanceEditModel::setBaseRadiusEnabled(bool value) {
  if (baseRadiusEnabled() == value) {
    return;
  }
  const bool dirty = isDirty();
  current_.shape.base_radius = value ? std::optional<double>(0.0) : std::nullopt;
  emit baseRadiusEnabledChanged();
  changed(&AppearanceEditModel::baseRadiusChanged, dirty);
}
void AppearanceEditModel::setBaseRadius(qreal value) {
  const bool dirty = isDirty();
  const bool was_enabled = baseRadiusEnabled();
  value = std::clamp(value, 0.0, 128.0);
  if (current_.shape.base_radius == value) {
    return;
  }
  current_.shape.base_radius = value;
  if (!was_enabled) {
    emit baseRadiusEnabledChanged();
  }
  changed(&AppearanceEditModel::baseRadiusChanged, dirty);
}
void AppearanceEditModel::setBaseChamferEnabled(bool value) {
  if (baseChamferEnabled() == value) {
    return;
  }
  const bool dirty = isDirty();
  current_.shape.base_chamfer = value ? std::optional<double>(0.0) : std::nullopt;
  emit baseChamferEnabledChanged();
  changed(&AppearanceEditModel::baseChamferChanged, dirty);
}
void AppearanceEditModel::setBaseChamfer(qreal value) {
  const bool dirty = isDirty();
  const bool was_enabled = baseChamferEnabled();
  value = std::clamp(value, 0.0, 128.0);
  if (current_.shape.base_chamfer == value) {
    return;
  }
  current_.shape.base_chamfer = value;
  if (!was_enabled) {
    emit baseChamferEnabledChanged();
  }
  changed(&AppearanceEditModel::baseChamferChanged, dirty);
}

void AppearanceEditModel::load(const HoloNight::Config::Appearance& value) {
  const bool was_dirty = isDirty();
  current_ = value;
  snapshot_ = value;
  emit themeSchemeChanged();
  emit themeAccentChanged();
  emit themeModeChanged();
  emit uiFontChanged();
  emit uiFontSizeChanged();
  emit monospaceFontChanged();
  emit monospaceFontSizeChanged();
  emit titleFontChanged();
  emit titleFontSizeChanged();
  emit displayFontChanged();
  emit displayFontSizeChanged();
  emit iconThemeChanged();
  emit fallbackIconThemeChanged();
  emit cursorThemeChanged();
  emit layoutScaleChanged();
  emit shapeStyleChanged();
  emit shapeScaleChanged();
  emit baseRadiusEnabledChanged();
  emit baseRadiusChanged();
  emit baseChamferEnabledChanged();
  emit baseChamferChanged();
  if (was_dirty) {
    emit isDirtyChanged();
  }
  setValidationError({});
}
void AppearanceEditModel::markSaved() {
  const bool dirty = isDirty();
  snapshot_ = current_;
  if (dirty) {
    emit isDirtyChanged();
  }
}
void AppearanceEditModel::setValidationError(const QString& value) {
  if (validation_error_ == value) {
    return;
  }
  validation_error_ = value;
  emit validationErrorChanged();
}
