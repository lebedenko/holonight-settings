#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QtQml/qqml.h>

struct AppearanceAdapterOutput {
  QString name{};
  QString status{};
  QString applyMode{};
  QString diagnostic{};
};

struct AppearanceAdapterResponse {
  QString operation{};
  QString result{};
  QList<AppearanceAdapterOutput> outputs;
};

Q_DECLARE_METATYPE(AppearanceAdapterOutput)
Q_DECLARE_METATYPE(AppearanceAdapterResponse)

class AppearanceAdapterClient : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("AppearanceAdapterClient is created by SettingsApplication")
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString resultText READ resultText NOTIFY resultChanged)
  Q_PROPERTY(QVariantList outputs READ outputs NOTIFY resultChanged)

 public:
  explicit AppearanceAdapterClient(QString executable = {}, QObject* parent = nullptr);
  [[nodiscard]] bool busy() const { return process_.state() != QProcess::NotRunning; }
  [[nodiscard]] QString resultText() const { return result_text_; }
  [[nodiscard]] QVariantList outputs() const;
  void apply(const QString& appearance_path);
  Q_INVOKABLE void status();
  Q_INVOKABLE void revert();

 Q_SIGNALS:
  void busyChanged();
  void resultChanged();
  void completed(const AppearanceAdapterResponse& response);
  void failed(const QString& diagnostic);

 private:
  void start(QString operation, const QStringList& arguments);
  void finish(int exit_code, QProcess::ExitStatus exit_status);
  void fail(QString diagnostic);
  QString executable_;
  QString operation_;
  QByteArray output_;
  QProcess process_;
  QTimer timer_;
  AppearanceAdapterResponse response_;
  QString result_text_;
  bool active_{false};
};
