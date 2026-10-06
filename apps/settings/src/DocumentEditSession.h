// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqml.h>

#include <functional>
#include <holonight/config/document.h>
#include <holonight/document_watcher.h>

class DocumentEditSession : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Document edit sessions belong to Settings models")
  Q_PROPERTY(QVariantList fields READ fields NOTIFY stateChanged)
  Q_PROPERTY(QVariantList conflicts READ conflicts NOTIFY stateChanged)
  Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY stateChanged)
  Q_PROPERTY(bool blocked READ blocked NOTIFY stateChanged)
 public:
  struct Field {
    QByteArray property;
    HoloNight::Config::KeyPath key;
    QString label;
    QByteArray enabled_property;
    bool sensitive{false};
    QVariantMap constraints;
  };
  using Projection = std::function<HoloNight::Config::Result<QVariantMap>(const HoloNight::Config::DocumentSnapshot&)>;
  DocumentEditSession(QObject* model, QString path, std::vector<Field> fields, HoloNight::Config::DocumentSchema schema,
                      Projection projection, bool appearance);
  [[nodiscard]] static QVariantMap properties(const QObject& model, const std::vector<Field>& fields);
  [[nodiscard]] bool reload(bool discard = false);
  [[nodiscard]] bool dirty() const;
  [[nodiscard]] bool blocked() const { return blocked_; }
  [[nodiscard]] QString diagnostics() const { return diagnostics_; }
  [[nodiscard]] QVariantList fields() const;
  [[nodiscard]] QVariantList conflicts() const;
  [[nodiscard]] const HoloNight::Config::EditBatch& edits() const { return edits_; }
  void accept(const HoloNight::Config::DocumentSnapshot& snapshot, const HoloNight::Config::EditBatch& saved = {});
  void suspend(bool value);
  void retry();
  void report(const HoloNight::Config::SaveResult& result);
  Q_INVOKABLE [[nodiscard]] QVariantMap metadata(const QString& property) const;
  Q_INVOKABLE void reset(const QString& property);
  Q_INVOKABLE void resolve(const QString& property, bool keepPending);
 Q_SIGNALS:
  void stateChanged();
  void dirtyChanged();
 private Q_SLOTS:
  void capture();

 private:
  [[nodiscard]] std::optional<HoloNight::Config::Value> controlValue(const Field& field) const;
  void show();
  void reconcile();
  void notify();
  QObject* model_;
  std::vector<Field> fields_;
  HoloNight::Config::DocumentSchema schema_;
  Projection projection_;
  bool appearance_;
  Holonight::DocumentWatcher watcher_;
  HoloNight::Config::DocumentSnapshot snapshot_;
  HoloNight::Config::EditBatch edits_;
  QVariantMap effective_;
  std::map<HoloNight::Config::KeyPath, std::optional<HoloNight::Config::Value>> shown_;
  QString diagnostics_;
  bool blocked_{false};
  bool refreshing_{false};
  bool suspended_{false};
  bool retry_{false};
  bool last_dirty_{false};
};
