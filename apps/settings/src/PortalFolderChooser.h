// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class PortalFolderChooser : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
 public:
  explicit PortalFolderChooser(QObject* parent = nullptr);
  explicit PortalFolderChooser(QDBusConnection connection, QObject* parent = nullptr);
  ~PortalFolderChooser() override;
  Q_DISABLE_COPY_MOVE(PortalFolderChooser)
  [[nodiscard]] bool busy() const { return !request_path_.isEmpty(); }
  [[nodiscard]] QString error() const { return error_; }
  Q_INVOKABLE void open();
  Q_INVOKABLE void cancel();
 Q_SIGNALS:
  void stateChanged();
  void folderSelected(const QUrl& folder);

 private:
  Q_SLOT void response(uint code, const QVariantMap& results, const QDBusMessage& message);
  [[nodiscard]] bool subscribe(const QString& path);
  void finish();
  void fail(const QString& message);
  QDBusConnection connection_;
  QString request_path_;
  QString error_;
  quint64 generation_ = 0;
};
