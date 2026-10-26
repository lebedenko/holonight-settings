// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#pragma once
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QVariantList>
#include <QtQml/qqml.h>

class WeatherLocationSearch : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariantList results READ results NOTIFY changed)
  Q_PROPERTY(bool loading READ loading NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
 public:
  explicit WeatherLocationSearch(QObject* parent = nullptr);
  WeatherLocationSearch(QNetworkAccessManager* network, QObject* parent);
  [[nodiscard]] QVariantList results() const { return results_; }
  [[nodiscard]] bool loading() const { return reply_ != nullptr; }
  [[nodiscard]] QString status() const { return status_; }
  Q_INVOKABLE void search(const QString& text, const QString& language = QStringLiteral("en"));
  static QVariantList parseResults(const QJsonObject& root);
 Q_SIGNALS:
  void changed();

 private:
  QNetworkAccessManager network_;
  QNetworkAccessManager* manager_;
  QPointer<QNetworkReply> reply_;
  QVariantList results_;
  QString status_;
};
