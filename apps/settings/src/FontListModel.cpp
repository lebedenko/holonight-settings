#include "FontListModel.h"

#include <QFontDatabase>

FontListModel::FontListModel(QObject* parent) : QAbstractListModel(parent), all_families_(QFontDatabase::families()) {
  rebuild();
}

void FontListModel::setFixedPitchOnly(bool value) {
  if (fixed_pitch_only_ == value) {
    return;
  }
  fixed_pitch_only_ = value;
  emit fixedPitchOnlyChanged();
  rebuild();
}

void FontListModel::rebuild() {
  beginResetModel();
  families_.clear();
  if (fixed_pitch_only_) {
    for (const QString& family : all_families_) {
      if (QFontDatabase::isFixedPitch(family)) {
        families_.append(family);
      }
    }
  } else {
    families_ = all_families_;
  }
  endResetModel();
}

int FontListModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(families_.size());
}

QVariant FontListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= families_.size()) {
    return {};
  }
  if (role == Qt::DisplayRole) {
    return families_.at(index.row());
  }
  return {};
}

QHash<int, QByteArray> FontListModel::roleNames() const {
  static const QHash<int, QByteArray> kRoles{{Qt::DisplayRole, "display"}};
  return kRoles;
}

int FontListModel::indexOf(const QString& family) const { return static_cast<int>(families_.indexOf(family)); }
