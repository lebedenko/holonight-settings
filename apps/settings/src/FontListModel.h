#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QtQml/qqml.h>

class FontListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool fixedPitchOnly READ fixedPitchOnly WRITE setFixedPitchOnly NOTIFY fixedPitchOnlyChanged)
  Q_PROPERTY(QString retainedFamily READ retainedFamily WRITE setRetainedFamily NOTIFY retainedFamilyChanged)

 public:
  explicit FontListModel(QObject* parent = nullptr);

  [[nodiscard]] bool fixedPitchOnly() const { return fixed_pitch_only_; }
  void setFixedPitchOnly(bool value);
  [[nodiscard]] QString retainedFamily() const { return retained_family_; }
  void setRetainedFamily(const QString& value);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] Q_INVOKABLE int indexOf(const QString& family) const;

 Q_SIGNALS:
  void fixedPitchOnlyChanged();
  void retainedFamilyChanged();

 private:
  void rebuild();

  bool fixed_pitch_only_{false};
  const QStringList all_families_;
  QStringList families_;
  QString retained_family_;
};
