#include "AppearanceAdapterClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>

namespace {
constexpr qsizetype kMaximumOutput = 1024 * 1024;
const QSet<QString> kStatuses{QStringLiteral("applied"),   QStringLiteral("unchanged"),
                              QStringLiteral("restored"),  QStringLiteral("unavailable"),
                              QStringLiteral("delegated"), QStringLiteral("application-owned"),
                              QStringLiteral("conflict"),  QStringLiteral("error")};
const QSet<QString> kModes{QStringLiteral("live"), QStringLiteral("relaunch"), QStringLiteral("session-restart"),
                           QStringLiteral("delegated")};
}  // namespace

AppearanceAdapterClient::AppearanceAdapterClient(QString executable, QObject* parent)
    : QObject(parent), executable_(std::move(executable)) {
  qRegisterMetaType<AppearanceAdapterResponse>();
  timer_.setSingleShot(true);
  timer_.setInterval(15000);
  connect(&timer_, &QTimer::timeout, this, [this] {
    process_.kill();
    fail(tr("Appearance propagation timed out"));
  });
  connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
    output_ += process_.readAllStandardOutput();
    if (output_.size() > kMaximumOutput) {
      process_.kill();
      fail(tr("Appearance propagation returned too much data"));
    }
  });
  connect(&process_, &QProcess::started, this, &AppearanceAdapterClient::busyChanged);
  connect(&process_, &QProcess::finished, this, &AppearanceAdapterClient::finish);
  connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) {
      fail(tr("Appearance propagation could not start"));
    }
  });
}

QVariantList AppearanceAdapterClient::outputs() const {
  QVariantList values;
  for (const auto& output : response_.outputs) {
    values.append(QVariantMap{{QStringLiteral("name"), output.name},
                              {QStringLiteral("status"), output.status},
                              {QStringLiteral("applyMode"), output.applyMode},
                              {QStringLiteral("diagnostic"), output.diagnostic}});
  }
  return values;
}

void AppearanceAdapterClient::apply(const QString& path) {
  start(QStringLiteral("apply"),
        {QStringLiteral("apply"), QStringLiteral("--appearance"), path, QStringLiteral("--json")});
}
void AppearanceAdapterClient::status() {
  start(QStringLiteral("status"), {QStringLiteral("status"), QStringLiteral("--json")});
}
void AppearanceAdapterClient::revert() {
  start(QStringLiteral("revert"), {QStringLiteral("revert"), QStringLiteral("--json")});
}

void AppearanceAdapterClient::start(QString operation, const QStringList& arguments) {
  if (busy()) {
    return;
  }
  operation_ = std::move(operation);
  active_ = true;
  output_.clear();
  QString executable = executable_;
  if (executable.isEmpty()) {
    executable = QStandardPaths::findExecutable(QStringLiteral("holonight-appearance-adapter"));
  }
  if (executable.isEmpty()) {
    response_ = {.operation = operation_, .result = QStringLiteral("degraded"), .outputs = {}};
    result_text_ = tr("Native toolkit adapter is unavailable; HoloNight appearance was saved");
    emit resultChanged();
    active_ = false;
    emit completed(response_);
    return;
  }
  process_.setProgram(executable);
  process_.setArguments(arguments);
  process_.setProcessChannelMode(QProcess::SeparateChannels);
  process_.start();
  timer_.start();
}

void AppearanceAdapterClient::fail(QString diagnostic) {
  if (!active_) {
    return;
  }
  active_ = false;
  timer_.stop();
  result_text_ = std::move(diagnostic);
  emit resultChanged();
  emit failed(result_text_);
  emit busyChanged();
}

void AppearanceAdapterClient::finish(int exit_code, QProcess::ExitStatus exit_status) {
  if (!active_) {
    return;
  }
  timer_.stop();
  output_ += process_.readAllStandardOutput();
  emit busyChanged();
  if (output_.size() > kMaximumOutput) {
    fail(tr("Appearance propagation returned too much data"));
    return;
  }
  QJsonParseError parse_error;
  const auto document = QJsonDocument::fromJson(output_, &parse_error);
  if (exit_status != QProcess::NormalExit || parse_error.error != QJsonParseError::NoError || !document.isObject()) {
    fail(tr("Appearance propagation returned an invalid response"));
    return;
  }
  const QJsonObject root = document.object();
  const QString result = root.value(QStringLiteral("result")).toString();
  const bool success = root.value(QStringLiteral("success")).toBool();
  const bool degraded = root.value(QStringLiteral("degraded")).toBool();
  const bool consistent = root.value(QStringLiteral("protocol_version")).toInt() == 1 &&
                          root.value(QStringLiteral("operation")).toString() == operation_ &&
                          (result == QStringLiteral("success") || result == QStringLiteral("degraded") ||
                           result == QStringLiteral("error")) &&
                          success == (result != QStringLiteral("error")) &&
                          degraded == (result == QStringLiteral("degraded")) &&
                          root.value(QStringLiteral("outputs")).isArray() && ((exit_code == 0) == success);
  if (!consistent) {
    fail(tr("Appearance propagation returned an inconsistent response"));
    return;
  }
  AppearanceAdapterResponse parsed{.operation = operation_, .result = result};
  for (const auto& item : root.value(QStringLiteral("outputs")).toArray()) {
    const auto value = item.toObject();
    const QString name = value.value(QStringLiteral("name")).toString();
    const QString status = value.value(QStringLiteral("status")).toString();
    const QString mode = value.value(QStringLiteral("apply_mode")).toString();
    if (!item.isObject() || name.isEmpty() || !kStatuses.contains(status) || !kModes.contains(mode)) {
      fail(tr("Appearance propagation returned an invalid output record"));
      return;
    }
    parsed.outputs.append({.name = name,
                           .status = status,
                           .applyMode = mode,
                           .diagnostic = value.value(QStringLiteral("diagnostic")).toString()});
  }
  if (!success) {
    fail(tr("Appearance propagation failed; the appearance file was restored"));
    return;
  }
  response_ = std::move(parsed);
  active_ = false;
  result_text_ = degraded ? tr("Appearance saved with limited native toolkit propagation")
                          : tr("Appearance propagated successfully");
  emit resultChanged();
  emit completed(response_);
}
