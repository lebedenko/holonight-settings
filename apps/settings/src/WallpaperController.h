// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#pragma once
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <holonight/document_watcher.h>

class WallpaperController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by the application")
  Q_PROPERTY(QStringList displays READ displays NOTIFY stateChanged)
  Q_PROPERTY(int target READ target WRITE setTarget NOTIFY stateChanged)
  Q_PROPERTY(bool allDisplays READ allDisplays WRITE setAllDisplays NOTIFY stateChanged)
  Q_PROPERTY(QUrl selectedImage READ selectedImage NOTIFY stateChanged)
  Q_PROPERTY(QString selectedPath READ selectedPath NOTIFY stateChanged)
  Q_PROPERTY(QString currentPath READ currentPath NOTIFY stateChanged)
  Q_PROPERTY(QUrl currentImage READ currentImage NOTIFY stateChanged)
  Q_PROPERTY(bool canApply READ canApply NOTIFY stateChanged)
  Q_PROPERTY(bool conflict READ conflict NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
 public:
  explicit WallpaperController(QObject* parent = nullptr, const QString& path = {});
  [[nodiscard]] QStringList displays() const { return displays_; }
  [[nodiscard]] int target() const { return target_; }
  [[nodiscard]] bool allDisplays() const { return all_displays_; }
  [[nodiscard]] QString selectedPath() const;
  [[nodiscard]] QString currentPath() const;
  [[nodiscard]] QUrl selectedImage() const;
  [[nodiscard]] QUrl currentImage() const;
  [[nodiscard]] bool canApply() const;
  [[nodiscard]] bool conflict() const;
  [[nodiscard]] QString error() const { return error_; }
  void setTarget(int target);
  void setAllDisplays(bool value);
  Q_INVOKABLE void select(const QString& path);
  Q_INVOKABLE void reopen(const QString& connector = {});
  Q_INVOKABLE void discard();
  Q_INVOKABLE bool apply();
  Q_INVOKABLE void resolve(bool keepPending);
  Q_INVOKABLE void reload();
  static HoloNight::Config::Value encodeImages(const QStringList& images);
  static QStringList assign(const QStringList& images, int count, int target, const QString& path, bool all);
 Q_SIGNALS:
  void stateChanged();
  void openRequested();

 private:
  void updateDisplays();
  Holonight::DocumentWatcher watcher_;
  HoloNight::Config::DocumentSnapshot snapshot_;
  std::optional<HoloNight::Config::Value> baseline_;
  QStringList applied_;
  QStringList pending_;
  QStringList displays_;
  QString error_;
  QString invalid_image_;
  int target_ = -1;
  bool all_displays_ = false;
  bool valid_ = false;
  bool displays_initialized_ = false;
};
