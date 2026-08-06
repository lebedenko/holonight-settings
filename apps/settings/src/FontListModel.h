#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QtQml/qqml.h>

class FontListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool fixedPitchOnly READ fixedPitchOnly WRITE setFixedPitchOnly NOTIFY fixedPitchOnlyChanged)

 public:
  explicit FontListModel(QObject* parent = nullptr);

  [[nodiscard]] bool fixedPitchOnly() const { return fixed_pitch_only_; }
  void setFixedPitchOnly(bool value);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] Q_INVOKABLE int indexOf(const QString& family) const;

 Q_SIGNALS:
  void fixedPitchOnlyChanged();

 private:
  void rebuild();

  bool fixed_pitch_only_{false};
  const QStringList all_families_;
  QStringList families_;
};
