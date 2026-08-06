#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

class ShellStatusService : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("ShellStatusService is created by SettingsApplication")

  Q_PROPERTY(bool shellRunning READ shellRunning NOTIFY shellRunningChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY shellRunningChanged)

 public:
  explicit ShellStatusService(QObject* parent = nullptr);

  [[nodiscard]] bool shellRunning() const { return shell_running_; }
  [[nodiscard]] QString statusText() const;

  Q_INVOKABLE void refresh();

 Q_SIGNALS:
  void shellRunningChanged();

 private:
  [[nodiscard]] static bool detectShellProcess();

  bool shell_running_{false};
};
