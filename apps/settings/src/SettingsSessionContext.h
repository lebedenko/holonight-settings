// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#pragma once
#include <QObject>
#include <QProcessEnvironment>
#include <QQmlEngine>

class SettingsSessionContext : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by the Settings application")
  Q_PROPERTY(QString compositor READ compositor CONSTANT)
 public:
  explicit SettingsSessionContext(QObject* parent = nullptr);
  explicit SettingsSessionContext(const QProcessEnvironment& environment, QObject* parent = nullptr);
  [[nodiscard]] QString compositor() const;
  Q_INVOKABLE bool isApplicable(const QString& property) const;

 private:
  const QString compositor_;
};
