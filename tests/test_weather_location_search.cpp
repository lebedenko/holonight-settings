// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "WeatherLocationSearch.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QUrlQuery>

#include <algorithm>
#include <cstring>
#include <gtest/gtest.h>

namespace {
class FixtureReply : public QNetworkReply {
 public:
  explicit FixtureReply(const QNetworkRequest& request, QObject* parent) : QNetworkReply(parent) {
    setRequest(request);
    setUrl(request.url());
    open(QIODevice::ReadOnly);
  }
  void abort() override { aborted = true; }
  void complete(const QByteArray& bytes, bool fail = false) {
    bytes_ = bytes;
    if (fail) setError(QNetworkReply::ConnectionRefusedError, "fixture failure");
    setFinished(true);
    emit readyRead();
    emit finished();
  }
  bool aborted{false};

 protected:
  qint64 readData(char* data, qint64 size) override {
    const auto count = std::min<qint64>(size, bytes_.size() - offset_);
    if (count <= 0) return -1;
    memcpy(data, bytes_.constData() + offset_, count);
    offset_ += count;
    return count;
  }

 private:
  QByteArray bytes_;
  qint64 offset_{0};
};
class FixtureNetwork : public QNetworkAccessManager {
 public:
  QList<FixtureReply*> requests;

 protected:
  QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) override {
    auto* reply = new FixtureReply(request, this);
    requests.append(reply);
    return reply;
  }
};
}  // namespace

TEST(WeatherLocationSearch, SearchResultsContainSelectableCoordinatesAndRegion) {
  FixtureNetwork network;
  WeatherLocationSearch search(&network, nullptr);
  search.search("Berlin", "de");
  ASSERT_TRUE(search.loading());
  ASSERT_EQ(network.requests.size(), 1);
  EXPECT_EQ(QUrlQuery(network.requests[0]->url()).queryItemValue("language"), "de");
  network.requests[0]->complete(
      R"({"results":[{"name":"Berlin","admin1":"Berlin","country":"Germany","latitude":52.52,"longitude":13.4},{"name":"Invalid","latitude":91,"longitude":0}]})");
  EXPECT_FALSE(search.loading());
  ASSERT_EQ(search.results().size(), 1);
  const auto item = search.results().first().toMap();
  EXPECT_EQ(item.value("region").toString(), "Berlin");
  EXPECT_DOUBLE_EQ(item.value("latitude").toDouble(), 52.52);
}
TEST(WeatherLocationSearch, SupersededSearchCannotReplaceResults) {
  FixtureNetwork network;
  WeatherLocationSearch search(&network, nullptr);
  search.search("Old");
  auto* old = network.requests[0];
  search.search("New");
  EXPECT_TRUE(old->aborted);
  network.requests[1]->complete(R"({"results":[{"name":"New","latitude":0,"longitude":0}]})");
  old->complete(R"({"results":[{"name":"Old","latitude":1,"longitude":1}]})");
  ASSERT_EQ(search.results().size(), 1);
  EXPECT_EQ(search.results()[0].toMap().value("city").toString(), "New");
}
TEST(WeatherLocationSearch, EmptyAndFailedSearchesHaveDistinctStates) {
  FixtureNetwork network;
  WeatherLocationSearch search(&network, nullptr);
  search.search("Empty");
  network.requests[0]->complete("{}");
  EXPECT_FALSE(search.loading());
  EXPECT_TRUE(search.results().isEmpty());
  EXPECT_TRUE(search.status().contains("No matching"));
  search.search("Failed");
  network.requests[1]->complete("", true);
  EXPECT_FALSE(search.loading());
  EXPECT_TRUE(search.results().isEmpty());
  EXPECT_TRUE(search.status().contains("failed"));
  search.search("");
  EXPECT_TRUE(search.status().isEmpty());
  EXPECT_TRUE(search.results().isEmpty());
}
