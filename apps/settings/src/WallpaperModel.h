// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#pragma once
#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QQuickAsyncImageProvider>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <cstdint>

class WallpaperModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by the application")
  Q_PROPERTY(QStringList folders READ folders NOTIFY stateChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
 public:
  // Qt model roles convert implicitly to the int-based model API.
  enum Role : std::uint16_t {  // NOLINT(cppcoreguidelines-use-enum-class)
    Path = Qt::UserRole + 1,
    Title,
    Dimensions,
    Bytes,
    Favorite,
    Thumbnail,
  };
  struct Entry {
    QString path;
    QSize size;
    qint64 bytes;
    qint64 modified;
  };
  explicit WallpaperModel(QObject* parent = nullptr, const QString& settingsPath = {});
  ~WallpaperModel() override;
  Q_DISABLE_COPY_MOVE(WallpaperModel)
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  [[nodiscard]] QStringList folders() const { return folders_; }
  [[nodiscard]] bool loading() const { return loading_; }
  [[nodiscard]] QString error() const { return error_; }
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void addFolder(const QUrl& folder);
  Q_INVOKABLE void toggleFavorite(const QString& path);
  Q_INVOKABLE [[nodiscard]] QVariantMap details(const QString& path) const;
 Q_SIGNALS:
  void stateChanged();

 private:
  void persist();
  QSettings settings_;
  QFileSystemWatcher watcher_;
  QTimer debounce_;
  QThreadPool pool_;
  QList<Entry> entries_;
  QStringList folders_;
  QStringList user_folders_;
  QStringList favorites_;
  QString error_;
  QString preferences_error_;
  quint64 generation_ = 0;
  bool loading_ = false;
};
class WallpaperFilter : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by the application")
  Q_PROPERTY(QString collection READ collection WRITE setCollection NOTIFY collectionChanged)
 public:
  explicit WallpaperFilter(WallpaperModel* model, QObject* parent = nullptr);
  [[nodiscard]] QString collection() const { return collection_; }
  void setCollection(const QString& collection);
 Q_SIGNALS:
  void collectionChanged();

 protected:
  [[nodiscard]] bool filterAcceptsRow(int row, const QModelIndex& parent) const override;

 private:
  QString collection_;
};
class WallpaperImageProvider : public QQuickAsyncImageProvider {
 public:
  QQuickImageResponse* requestImageResponse(const QString& identifier, const QSize& size) override;
};
