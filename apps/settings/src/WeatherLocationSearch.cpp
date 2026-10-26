// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "WeatherLocationSearch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include <cmath>

WeatherLocationSearch::WeatherLocationSearch(QObject* parent) : WeatherLocationSearch(nullptr, parent) {}
WeatherLocationSearch::WeatherLocationSearch(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), network_(this), manager_((network != nullptr) ? network : &network_) {}
QVariantList WeatherLocationSearch::parseResults(const QJsonObject& root) {
  QVariantList results;
  for (const auto& value : root.value("results").toArray()) {
    const auto item = value.toObject();
    const auto lat = item.value("latitude");
    const auto lon = item.value("longitude");
    if (!lat.isDouble() || !lon.isDouble() || !std::isfinite(lat.toDouble()) || !std::isfinite(lon.toDouble()) ||
        std::abs(lat.toDouble()) > 90 || std::abs(lon.toDouble()) > 180 || item.value("name").toString().isEmpty()) {
      continue;
    }
    results.append(QVariantMap{
        {"city", item.value("name").toString()},
        {"region", item.value("admin1").toString()},
        {"country", item.value("country").toString()},
        {"latitude", lat.toDouble()},
        {"longitude", lon.toDouble()},
    });
  }
  return results;
}
void WeatherLocationSearch::search(const QString& text, const QString& language) {
  if (reply_) {
    disconnect(reply_, nullptr, this, nullptr);
    reply_->abort();
    reply_->deleteLater();
    reply_ = nullptr;
  }
  results_.clear();
  status_.clear();
  if (text.trimmed().size() < 2) {
    emit changed();
    return;
  }
  QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
  QUrlQuery query;
  query.addQueryItem("name", text.trimmed());
  query.addQueryItem("count", "10");
  query.addQueryItem("language", language);
  url.setQuery(query);
  QNetworkRequest request(url);
  request.setTransferTimeout(15000);
  reply_ = manager_->get(request);
  auto* reply = reply_.data();
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    if (reply_ != reply) {
      return;
    }
    reply_ = nullptr;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (reply->error() != QNetworkReply::NoError || error.error != QJsonParseError::NoError || !doc.isObject() ||
        doc.object().value("error").toBool()) {
      status_ = tr("Location search failed. Try again.");
    } else {
      results_ = parseResults(doc.object());
      if (results_.isEmpty()) {
        status_ = tr("No matching locations.");
      }
    }
    reply->deleteLater();
    emit changed();
  });
  emit changed();
}
