#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

class AppearanceEditModel;
class AppearanceFileService;
class ShellSettingsEditModel;
class ShellConfigFileService;
class AppearanceAdapterClient;
struct AppearanceAdapterResponse;

class SettingsSaveCoordinator : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("SettingsSaveCoordinator is created by SettingsApplication")
  Q_PROPERTY(bool isDirty READ isDirty NOTIFY isDirtyChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
  Q_PROPERTY(QString resultText READ resultText NOTIFY resultTextChanged)
  Q_PROPERTY(QString conflictDomain READ conflictDomain NOTIFY conflictDomainChanged)

 public:
  SettingsSaveCoordinator(AppearanceEditModel* appearance, AppearanceFileService* appearance_files,
                          ShellSettingsEditModel* shell, ShellConfigFileService* shell_files,
                          AppearanceAdapterClient* adapter = nullptr, QObject* parent = nullptr);
  [[nodiscard]] bool isDirty() const;
  [[nodiscard]] bool isBusy() const { return busy_; }
  [[nodiscard]] QString resultText() const { return result_text_; }
  [[nodiscard]] QString conflictDomain() const { return conflict_domain_; }
  Q_INVOKABLE void save();
  Q_INVOKABLE void discard();
  Q_INVOKABLE void reloadConflict();
  Q_INVOKABLE void overwriteConflict();
  Q_INVOKABLE void cancelConflict();
  Q_INVOKABLE void reapplyAppearance();
  Q_INVOKABLE void refreshIntegrations();
  Q_INVOKABLE void restoreNativeDefaults();

 Q_SIGNALS:
  void isDirtyChanged();
  void isBusyChanged();
  void resultTextChanged();
  void conflictDomainChanged();

 private:
  void setBusy(bool value);
  void setResult(QString value);
  void setConflict(QString value);
  void beginAppearance(bool overwrite);
  void finishSave(QString appearance_result = {});
  AppearanceEditModel* appearance_;
  AppearanceFileService* appearance_files_;
  ShellSettingsEditModel* shell_;
  ShellConfigFileService* shell_files_;
  AppearanceAdapterClient* adapter_;
  bool busy_{false};
  int succeeded_{0};
  int failed_{0};
  bool appearance_staged_{false};
  QString result_text_;
  QString conflict_domain_;
};
