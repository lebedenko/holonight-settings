#pragma once

#include "FileRevision.h"

#include <QObject>
#include <QVariantList>
#include <QtQml/qqml.h>

#include <holonightd/Trash.h>

class TrashSettingsModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("TrashSettingsModel is owned by SettingsSaveCoordinator")
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
  Q_PROPERTY(int retentionDays READ retentionDays WRITE setRetentionDays NOTIFY changed)
  Q_PROPERTY(QVariantList shares READ shares NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)

 public:
  explicit TrashSettingsModel(QObject* parent = nullptr);
  [[nodiscard]] bool enabled() const { return pending_.enabled; }
  [[nodiscard]] int retentionDays() const { return pending_.retention_days; }
  [[nodiscard]] QString error() const { return error_; }
  [[nodiscard]] bool isDirty() const;
  [[nodiscard]] QVariantList shares() const;
  void setEnabled(bool enabled);
  void setRetentionDays(int days);
  Q_INVOKABLE void setShare(const QString& mount, const QString& source, bool enabled);
  Q_INVOKABLE void refreshShares();
  bool save();
  void load();

 Q_SIGNALS:
  void changed();

 private:
  QString path_;
  QString error_;
  FileRevision revision_;
  holonightd::trash::Policy saved_;
  holonightd::trash::Policy pending_;
  std::vector<holonightd::trash::Share> active_;
};
