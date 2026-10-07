// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "WallpaperModel.h"

#include <QCache>
#include <QDateTime>
#include <QDirIterator>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QStandardPaths>

#include <algorithm>

namespace {
QString canonical(const QString& path) { return QFileInfo(path).canonicalFilePath(); }
bool raster(QImageReader& reader) {
  const auto format = reader.format().toLower();
  return reader.canRead() && !reader.supportsAnimation() && format != "svg" && format != "svgz";
}
struct ScanResult {
  QList<WallpaperModel::Entry> entries;
  QStringList watched;
  QStringList errors;
};
void scanFolder(const QString& folder, ScanResult& result, QSet<QString>& seen) {
  auto& entries = result.entries;
  auto& watched = result.watched;
  auto& errors = result.errors;
  if (!QFileInfo(folder).isReadable() || !QFileInfo(folder).isDir()) {
    errors << WallpaperModel::tr("Cannot read folder: %1").arg(folder);
    auto parent = QFileInfo(folder).dir();
    while (!parent.exists() && parent.cdUp()) {
    }
    watched << parent.absolutePath();
    return;
  }
  watched << folder;
  QDirIterator iterator(folder, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (iterator.hasNext()) {
    iterator.next();
    const QFileInfo file = iterator.fileInfo();
    if (file.isDir()) {
      if (!file.isSymLink()) {
        watched << file.absoluteFilePath();
      }
      continue;
    }
    const auto path = file.canonicalFilePath();
    if (path.isEmpty() || seen.contains(path)) {
      continue;
    }
    seen.insert(path);
    QImageReader reader(path);
    if (!raster(reader) || !reader.size().isValid()) {
      continue;
    }
    entries << WallpaperModel::Entry{
        .path = path,
        .size = reader.size(),
        .bytes = file.size(),
        .modified = file.lastModified().toMSecsSinceEpoch(),
    };
    watched << path;
  }
}
class ThumbnailResponse : public QQuickImageResponse {
 public:
  ThumbnailResponse(QString path, QSize size) {
    QThreadPool::globalInstance()->start([this, path = std::move(path), size] {
      static QMutex mutex;
      static QCache<QString, QImage> cache(32768);
      const QString key = path + QString::number(QFileInfo(path).lastModified().toMSecsSinceEpoch()) +
                          QStringLiteral("/%1x%2").arg(size.width()).arg(size.height());
      {
        const QMutexLocker lock(&mutex);
        if (const auto* cached = cache.object(key)) {
          image_ = *cached;
        }
      }
      if (image_.isNull()) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        if (raster(reader)) {
          const QSize original = reader.size();
          if (original.isValid()) {
            reader.setScaledSize(original.scaled(size, Qt::KeepAspectRatio).expandedTo(QSize(1, 1)));
          }
          image_ = reader.read();
        }
        if (image_.isNull()) {
          error_ = QStringLiteral("Cannot decode wallpaper");
        } else {
          const QMutexLocker lock(&mutex);
          cache.insert(key, new QImage(image_), static_cast<int>((image_.sizeInBytes() / 1024) + 1));
        }
      }
      emit finished();
    });
  }
  [[nodiscard]] QQuickTextureFactory* textureFactory() const override {
    return QQuickTextureFactory::textureFactoryForImage(image_);
  }
  [[nodiscard]] QString errorString() const override { return error_; }

 private:
  QImage image_;
  QString error_;
};
}  // namespace
WallpaperModel::WallpaperModel(QObject* parent, const QString& settingsPath)
    : QAbstractListModel(parent),
      settings_(settingsPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
                                             QStringLiteral("/holonight/settings-wallpapers.ini")
                                       : settingsPath,
                QSettings::IniFormat) {
  user_folders_ = settings_.value("folders").toStringList();
  favorites_ = settings_.value("favorites").toStringList();
  if (settingsPath.isEmpty()) {
    for (const auto& location : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
      const auto path = canonical(location + QStringLiteral("/wallpapers/holonight"));
      if (!path.isEmpty()) {
        folders_ << path;
      }
    }
    const auto pictures = canonical(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (!pictures.isEmpty()) {
      folders_ << pictures;
    }
  }
  folders_ << user_folders_;
  folders_.removeDuplicates();
  pool_.setMaxThreadCount(1);
  debounce_.setSingleShot(true);
  debounce_.setInterval(200);
  connect(&watcher_, &QFileSystemWatcher::directoryChanged, &debounce_, qOverload<>(&QTimer::start));
  connect(&watcher_, &QFileSystemWatcher::fileChanged, &debounce_, qOverload<>(&QTimer::start));
  connect(&debounce_, &QTimer::timeout, this, &WallpaperModel::refresh);
}
WallpaperModel::~WallpaperModel() { pool_.waitForDone(); }
int WallpaperModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(entries_.size());
}
QHash<int, QByteArray> WallpaperModel::roleNames() const {
  return {
      {Path, "imagePath"}, {Title, "imageTitle"},  {Dimensions, "dimensions"},
      {Bytes, "byteSize"}, {Favorite, "favorite"}, {Thumbnail, "thumbnail"},
  };
}
QVariant WallpaperModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() >= entries_.size()) {
    return {};
  }
  const auto& entry = entries_[index.row()];
  switch (role) {
    case Path:
      return entry.path;
    case Title:
      return QFileInfo(entry.path).completeBaseName();
    case Dimensions:
      return QStringLiteral("%1 × %2").arg(entry.size.width()).arg(entry.size.height());
    case Bytes:
      return entry.bytes;
    case Favorite:
      return favorites_.contains(entry.path);
    case Thumbnail:
      return QUrl(QStringLiteral("image://wallpaper/") + QString::fromLatin1(QUrl::toPercentEncoding(entry.path)) +
                  QStringLiteral("?%1").arg(entry.modified));
    default:
      return {};
  }
}
QVariantMap WallpaperModel::details(const QString& path) const {
  const auto found = std::ranges::find(entries_, path, &Entry::path);
  if (found == entries_.end()) {
    const QFileInfo file(path);
    const QSize size = QImageReader(path).size();
    return {
        {"title", file.completeBaseName()},
        {"dimensions", size.isValid() ? QStringLiteral("%1 × %2").arg(size.width()).arg(size.height()) : QString{}},
        {"bytes", file.size()},
        {"favorite", favorites_.contains(file.canonicalFilePath())},
    };
  }
  const auto index = this->index(static_cast<int>(std::distance(entries_.begin(), found)));
  return {
      {"title", data(index, Title)},
      {"dimensions", data(index, Dimensions)},
      {"bytes", data(index, Bytes)},
      {"favorite", data(index, Favorite)},
  };
}
void WallpaperModel::persist() {
  settings_.setValue("folders", user_folders_);
  settings_.setValue("favorites", favorites_);
  settings_.sync();
  if (settings_.status() != QSettings::NoError) {
    preferences_error_ = tr("Could not save wallpaper folders and favorites.");
    error_ = preferences_error_;
  }
  emit stateChanged();
}
void WallpaperModel::addFolder(const QUrl& folder) {
  const auto path = canonical(folder.toLocalFile());
  if (path.isEmpty() || !QFileInfo(path).isDir() || folders_.contains(path)) {
    return;
  }
  user_folders_ << path;
  folders_ << path;
  persist();
  refresh();
}
void WallpaperModel::toggleFavorite(const QString& path) {
  const auto value = canonical(path);
  if (value.isEmpty()) {
    return;
  }
  if (!favorites_.removeOne(value)) {
    favorites_ << value;
  }
  persist();
  if (!entries_.isEmpty()) {
    emit dataChanged(index(0), index(rowCount() - 1), {Favorite});
  }
}
void WallpaperModel::refresh() {
  const auto generation = ++generation_;
  loading_ = true;
  emit stateChanged();
  pool_.clear();
  pool_.start([this, folders = folders_, generation] {
    ScanResult result;
    QSet<QString> seen;
    for (const auto& folder : folders) {
      scanFolder(folder, result, seen);
    }
    auto& entries = result.entries;
    std::ranges::sort(entries, [](const Entry& left, const Entry& right) {
      const auto left_name = QFileInfo(left.path).fileName();
      const auto right_name = QFileInfo(right.path).fileName();
      return left_name == right_name ? left.path < right.path : left_name < right_name;
    });
    QMetaObject::invokeMethod(
        this,
        [this, generation, entries = std::move(entries), watched = std::move(result.watched),
         errors = std::move(result.errors)] {
          if (generation != generation_) {
            return;
          }
          beginResetModel();
          entries_ = entries;
          endResetModel();
          if (!watcher_.directories().isEmpty()) {
            watcher_.removePaths(watcher_.directories());
          }
          if (!watcher_.files().isEmpty()) {
            watcher_.removePaths(watcher_.files());
          }
          if (!watched.isEmpty()) {
            watcher_.addPaths(watched);
          }
          loading_ = false;
          QStringList messages = errors;
          if (!preferences_error_.isEmpty()) {
            messages << preferences_error_;
          }
          error_ = messages.join('\n');
          emit stateChanged();
        },
        Qt::QueuedConnection);
  });
}
WallpaperFilter::WallpaperFilter(WallpaperModel* model, QObject* parent) : QSortFilterProxyModel(parent) {
  setSourceModel(model);
}
void WallpaperFilter::setCollection(const QString& collection) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
  beginFilterChange();
  collection_ = collection;
  endFilterChange(Direction::Rows);
#else
  collection_ = collection;
  invalidateFilter();
#endif
  emit collectionChanged();
}
bool WallpaperFilter::filterAcceptsRow(int row, const QModelIndex& parent) const {
  const auto index = sourceModel()->index(row, 0, parent);
  if (collection_ == QStringLiteral("favorites")) {
    return sourceModel()->data(index, WallpaperModel::Favorite).toBool();
  }
  return collection_.isEmpty() ||
         sourceModel()->data(index, WallpaperModel::Path).toString().startsWith(collection_ + '/');
}
QQuickImageResponse* WallpaperImageProvider::requestImageResponse(const QString& identifier, const QSize& size) {
  const auto path = QUrl::fromPercentEncoding(identifier.section('?', 0, 0).toUtf8());
  const QSize bounded = size.isValid() ? size.boundedTo(QSize(1024, 1024)) : QSize(320, 200);
  return new ThumbnailResponse(path, bounded);
}
