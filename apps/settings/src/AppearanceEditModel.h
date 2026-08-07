#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

#include <holonight/config/appearance.h>

class AppearanceEditModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("AppearanceEditModel is created by SettingsApplication")

#define APPEARANCE_PROPERTY(type, name, getter, setter) \
  Q_PROPERTY(type name READ getter WRITE setter NOTIFY name##Changed)
  APPEARANCE_PROPERTY(QString, themeScheme, themeScheme, setThemeScheme)
  APPEARANCE_PROPERTY(QString, themeAccent, themeAccent, setThemeAccent)
  APPEARANCE_PROPERTY(QString, themeMode, themeMode, setThemeMode)
  APPEARANCE_PROPERTY(QString, uiFont, uiFont, setUiFont)
  APPEARANCE_PROPERTY(int, uiFontSize, uiFontSize, setUiFontSize)
  APPEARANCE_PROPERTY(QString, monospaceFont, monospaceFont, setMonospaceFont)
  APPEARANCE_PROPERTY(int, monospaceFontSize, monospaceFontSize, setMonospaceFontSize)
  APPEARANCE_PROPERTY(QString, titleFont, titleFont, setTitleFont)
  APPEARANCE_PROPERTY(int, titleFontSize, titleFontSize, setTitleFontSize)
  APPEARANCE_PROPERTY(QString, displayFont, displayFont, setDisplayFont)
  APPEARANCE_PROPERTY(int, displayFontSize, displayFontSize, setDisplayFontSize)
  APPEARANCE_PROPERTY(QString, iconTheme, iconTheme, setIconTheme)
  APPEARANCE_PROPERTY(QString, fallbackIconTheme, fallbackIconTheme, setFallbackIconTheme)
  APPEARANCE_PROPERTY(QString, cursorTheme, cursorTheme, setCursorTheme)
  APPEARANCE_PROPERTY(qreal, layoutScale, layoutScale, setLayoutScale)
  APPEARANCE_PROPERTY(QString, shapeStyle, shapeStyle, setShapeStyle)
  APPEARANCE_PROPERTY(qreal, shapeScale, shapeScale, setShapeScale)
  APPEARANCE_PROPERTY(bool, baseRadiusEnabled, baseRadiusEnabled, setBaseRadiusEnabled)
  APPEARANCE_PROPERTY(qreal, baseRadius, baseRadius, setBaseRadius)
  APPEARANCE_PROPERTY(bool, baseChamferEnabled, baseChamferEnabled, setBaseChamferEnabled)
  APPEARANCE_PROPERTY(qreal, baseChamfer, baseChamfer, setBaseChamfer)
#undef APPEARANCE_PROPERTY
  Q_PROPERTY(bool lightModeAvailable READ lightModeAvailable NOTIFY themeSchemeChanged)
  Q_PROPERTY(bool darkModeAvailable READ darkModeAvailable NOTIFY themeSchemeChanged)
  Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)
  Q_PROPERTY(QString validationError READ validationError NOTIFY validationErrorChanged)

 public:
  explicit AppearanceEditModel(QObject* parent = nullptr);

  [[nodiscard]] QString themeScheme() const;
  [[nodiscard]] QString themeAccent() const;
  [[nodiscard]] QString themeMode() const;
  [[nodiscard]] QString uiFont() const;
  [[nodiscard]] int uiFontSize() const;
  [[nodiscard]] QString monospaceFont() const;
  [[nodiscard]] int monospaceFontSize() const;
  [[nodiscard]] QString titleFont() const;
  [[nodiscard]] int titleFontSize() const;
  [[nodiscard]] QString displayFont() const;
  [[nodiscard]] int displayFontSize() const;
  [[nodiscard]] QString iconTheme() const;
  [[nodiscard]] QString fallbackIconTheme() const;
  [[nodiscard]] QString cursorTheme() const;
  [[nodiscard]] qreal layoutScale() const;
  [[nodiscard]] QString shapeStyle() const;
  [[nodiscard]] qreal shapeScale() const;
  [[nodiscard]] bool baseRadiusEnabled() const;
  [[nodiscard]] qreal baseRadius() const;
  [[nodiscard]] bool baseChamferEnabled() const;
  [[nodiscard]] qreal baseChamfer() const;
  [[nodiscard]] bool lightModeAvailable() const;
  [[nodiscard]] bool darkModeAvailable() const;
  [[nodiscard]] bool isDirty() const { return current_ != snapshot_; }
  [[nodiscard]] QString validationError() const { return validation_error_; }
  [[nodiscard]] const HoloNight::Config::Appearance& value() const { return current_; }

  void setThemeScheme(const QString& value);
  void setThemeAccent(const QString& value);
  void setThemeMode(const QString& value);
  void setUiFont(const QString& value);
  void setUiFontSize(int value);
  void setMonospaceFont(const QString& value);
  void setMonospaceFontSize(int value);
  void setTitleFont(const QString& value);
  void setTitleFontSize(int value);
  void setDisplayFont(const QString& value);
  void setDisplayFontSize(int value);
  void setIconTheme(const QString& value);
  void setFallbackIconTheme(const QString& value);
  void setCursorTheme(const QString& value);
  void setLayoutScale(qreal value);
  void setShapeStyle(const QString& value);
  void setShapeScale(qreal value);
  void setBaseRadiusEnabled(bool value);
  void setBaseRadius(qreal value);
  void setBaseChamferEnabled(bool value);
  void setBaseChamfer(qreal value);

  void load(const HoloNight::Config::Appearance& value);
  void markSaved();
  void setValidationError(const QString& value);

 Q_SIGNALS:
#define APPEARANCE_SIGNAL(name) void name##Changed();
  APPEARANCE_SIGNAL(themeScheme)
  APPEARANCE_SIGNAL(themeAccent)
  APPEARANCE_SIGNAL(themeMode)
  APPEARANCE_SIGNAL(uiFont)
  APPEARANCE_SIGNAL(uiFontSize)
  APPEARANCE_SIGNAL(monospaceFont)
  APPEARANCE_SIGNAL(monospaceFontSize)
  APPEARANCE_SIGNAL(titleFont)
  APPEARANCE_SIGNAL(titleFontSize)
  APPEARANCE_SIGNAL(displayFont)
  APPEARANCE_SIGNAL(displayFontSize)
  APPEARANCE_SIGNAL(iconTheme)
  APPEARANCE_SIGNAL(fallbackIconTheme)
  APPEARANCE_SIGNAL(cursorTheme)
  APPEARANCE_SIGNAL(layoutScale)
  APPEARANCE_SIGNAL(shapeStyle)
  APPEARANCE_SIGNAL(shapeScale)
  APPEARANCE_SIGNAL(baseRadiusEnabled)
  APPEARANCE_SIGNAL(baseRadius)
  APPEARANCE_SIGNAL(baseChamferEnabled)
  APPEARANCE_SIGNAL(baseChamfer)
#undef APPEARANCE_SIGNAL
  void isDirtyChanged();
  void validationErrorChanged();

 private:
  void changed(void (AppearanceEditModel::*signal)(), bool was_dirty);
  [[nodiscard]] QString sibling(const QString& mode) const;
  HoloNight::Config::Appearance current_{HoloNight::Config::defaults()};
  HoloNight::Config::Appearance snapshot_{current_};
  QString validation_error_;
};
