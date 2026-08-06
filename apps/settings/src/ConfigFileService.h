#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

class SettingsEditModel;

class ConfigFileService : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("ConfigFileService is created by SettingsApplication")

  Q_PROPERTY(bool isSaving READ isSaving NOTIFY isSavingChanged)

 public:
  explicit ConfigFileService(SettingsEditModel* model, QObject* parent = nullptr);

  [[nodiscard]] bool isSaving() const { return is_saving_; }

  Q_INVOKABLE bool load();
  Q_INVOKABLE bool save();
  [[nodiscard]] Q_INVOKABLE static QString configPath();
  [[nodiscard]] Q_INVOKABLE static QString themeConfigPath();
  [[nodiscard]] Q_INVOKABLE static QString appearanceConfigPath();

 Q_SIGNALS:
  void isSavingChanged();
  void saveStarted();
  void saveFinished(bool success);
  void saveError(const QString& message);

 private:
  SettingsEditModel* model_;
  bool is_saving_{false};
};
