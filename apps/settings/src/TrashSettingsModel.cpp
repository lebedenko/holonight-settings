#include "TrashSettingsModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>

namespace trash = holonightd::trash;

TrashSettingsModel::TrashSettingsModel(QObject* parent)
    : QObject(parent), path_(QFile::decodeName(trash::policyPath().c_str())) {
  load();
}

bool TrashSettingsModel::isDirty() const {
  return pending_.enabled != saved_.enabled || pending_.retention_days != saved_.retention_days ||
         pending_.nfs_shares != saved_.nfs_shares;
}

void TrashSettingsModel::setEnabled(bool enabled) {
  if (pending_.enabled != enabled) {
    pending_.enabled = enabled;
    emit changed();
  }
}

void TrashSettingsModel::setRetentionDays(int days) {
  if (days >= 1 && days <= 365 && pending_.retention_days != days) {
    pending_.retention_days = days;
    emit changed();
  }
}

QVariantList TrashSettingsModel::shares() const {
  auto choices = active_;
  for (const auto& share : pending_.nfs_shares) {
    if (std::ranges::find(choices, share) == choices.end()) {
      choices.push_back(share);
    }
  }
  QVariantList rows;
  for (const auto& share : choices) {
    rows.push_back(QVariantMap{
        {QStringLiteral("mount"), QFile::decodeName(share.mount_point.c_str())},
        {QStringLiteral("source"), QFile::decodeName(share.export_source.c_str())},
        {QStringLiteral("enabled"), std::ranges::find(pending_.nfs_shares, share) != pending_.nfs_shares.end()},
        {QStringLiteral("online"), std::ranges::find(active_, share) != active_.end()},
    });
  }
  return rows;
}

void TrashSettingsModel::setShare(const QString& mount, const QString& source, bool enabled) {
  const trash::Share share{
      .mount_point = QFile::encodeName(mount).toStdString(),
      .export_source = QFile::encodeName(source).toStdString(),
  };
  if (enabled && std::ranges::find(active_, share) != active_.end() &&
      std::ranges::find(pending_.nfs_shares, share) == pending_.nfs_shares.end()) {
    pending_.nfs_shares.push_back(share);
  } else if (!enabled) {
    std::erase(pending_.nfs_shares, share);
  }
  emit changed();
}

void TrashSettingsModel::refreshShares() {
  active_ = trash::activeNfsShares();
  emit changed();
}

void TrashSettingsModel::load() {
  revision_ = readFileRevision(path_);
  saved_ = {};
  error_.clear();
  if (revision_.exists) {
    const auto policy = trash::readPolicy(QFile::encodeName(path_).toStdString());
    if (policy) {
      saved_ = *policy;
    } else {
      error_ = QString::fromStdString(policy.error());
    }
  }
  pending_ = saved_;
  refreshShares();
}

bool TrashSettingsModel::save() {
  const auto bytes = trash::serializePolicy(pending_);
  if (!bytes) {
    error_ = QString::fromStdString(bytes.error());
  } else if (readFileRevision(path_) != revision_) {
    error_ = tr("Trash settings changed on disk. Discard to reload them before editing again.");
  } else if (path_.isEmpty() || !QDir().mkpath(QFileInfo(path_).absolutePath())) {
    error_ = tr("Could not create the Trash settings directory.");
  } else {
    QSaveFile file(path_);
    const auto data = QByteArray::fromStdString(*bytes);
    if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit()) {
      saved_ = pending_;
      revision_ = readFileRevision(path_);
      error_.clear();
      emit changed();
      return true;
    }
    error_ = file.errorString();
  }
  emit changed();
  return false;
}
