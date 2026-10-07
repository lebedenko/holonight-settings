// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "WallpaperController.h"

#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QScreen>

#include <holonight_shell_config/config_path.h>
#include <holonight_shell_config/config_schema.h>
#include <holonight_shell_config/config_structs.h>

namespace Config = HoloNight::Config;
namespace {
const Config::KeyPath kImages{"background", "images"};
QString diagnostics(const std::vector<Config::Diagnostic>& notes) {
  QStringList lines;
  for (const auto& note : notes) {
    lines << QString::fromStdString(note.message);
  }
  return lines.join('\n');
}
QUrl imageSource(const QString& path) {
  if (path.isEmpty()) {
    return {};
  }
  return {QStringLiteral("image://wallpaper/") + QString::fromLatin1(QUrl::toPercentEncoding(path)) +
          QStringLiteral("?%1").arg(QFileInfo(path).lastModified().toMSecsSinceEpoch())};
}
QString effective(const QStringList& images, int index) {
  return index < 0 ? QString{} : HoloNight::ShellConfig::BackgroundConfig::imageForMonitor(images, index);
}
}  // namespace
WallpaperController::WallpaperController(QObject* parent, const QString& path)
    : QObject(parent), watcher_(path.isEmpty() ? HoloNight::ShellConfig::resolveProductConfigPath() : path) {
  connect(&watcher_, &Holonight::DocumentWatcher::documentChanged, this, &WallpaperController::reload);
  connect(qGuiApp, &QGuiApplication::screenAdded, this, [this] { updateDisplays(); });
  connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this] { updateDisplays(); });
  updateDisplays();
  reload();
}
Config::Value WallpaperController::encodeImages(const QStringList& images) {
  QStringList quoted;
  for (const auto& image : images) {
    QString escaped;
    for (const QChar character : image) {
      switch (character.unicode()) {
        case '"':
          escaped += QStringLiteral("\\\"");
          break;
        case '\\':
          escaped += QStringLiteral("\\\\");
          break;
        default:
          if (character.unicode() < 0x20 || character.unicode() == 0x7f) {
            escaped += QStringLiteral("\\u%1").arg(static_cast<uint>(character.unicode()), 4, 16, QLatin1Char('0'));
          } else {
            escaped += character;
          }
      }
    }
    quoted << QLatin1Char('"') + escaped + QLatin1Char('"');
  }
  return Config::AggregateValue{('[' + quoted.join(QStringLiteral(", ")) + ']').toStdString()};
}
QStringList WallpaperController::assign(const QStringList& images, int count, int target, const QString& path,
                                        bool all) {
  if (all) {
    return {path};
  }
  if (target < 0 || target >= count) {
    return images;
  }
  QStringList result = images;
  while (result.size() < count) {
    result << (images.isEmpty() ? QString{} : images.last());
  }
  result[target] = path;
  return result;
}
void WallpaperController::updateDisplays() {
  const bool initial = !displays_initialized_;
  displays_initialized_ = true;
  const QString previous = displays_.value(target_);
  displays_.clear();
  for (auto* screen : QGuiApplication::screens()) {
    displays_ << screen->name();
  }
  auto* primary = QGuiApplication::primaryScreen();
  if (initial) {
    target_ = primary != nullptr ? static_cast<int>(displays_.indexOf(primary->name())) : -1;
  } else {
    target_ = previous.isEmpty() ? -1 : static_cast<int>(displays_.indexOf(previous));
  }
  emit stateChanged();
}
QString WallpaperController::selectedPath() const { return effective(pending_, target_); }
QString WallpaperController::currentPath() const { return effective(applied_, target_); }
QUrl WallpaperController::selectedImage() const { return imageSource(selectedPath()); }
QUrl WallpaperController::currentImage() const { return imageSource(currentPath()); }
bool WallpaperController::conflict() const {
  return pending_ != applied_ && snapshot_.value(kImages) != baseline_ &&
         snapshot_.value(kImages) != encodeImages(pending_);
}
bool WallpaperController::canApply() const {
  if (!valid_ || target_ < 0 || target_ >= displays_.size() || pending_ == applied_ || conflict()) {
    return false;
  }
  for (const auto& image : pending_) {
    if (!image.isEmpty() && (image == invalid_image_ || !QImageReader(image).canRead())) {
      return false;
    }
  }
  return !selectedPath().isEmpty();
}
void WallpaperController::setTarget(int target) {
  target_ = target >= 0 && target < displays_.size() ? target : -1;
  emit stateChanged();
}
void WallpaperController::setAllDisplays(bool value) {
  all_displays_ = value;
  if (value && !selectedPath().isEmpty()) {
    pending_ = {selectedPath()};
  }
  emit stateChanged();
}
void WallpaperController::select(const QString& path) {
  invalid_image_.clear();
  pending_ = assign(pending_, static_cast<int>(displays_.size()), target_, path, all_displays_);
  error_ = QImageReader(path).canRead() ? QString{} : tr("The selected image cannot be read.");
  emit stateChanged();
}
void WallpaperController::reopen(const QString& connector) {
  reload();
  if (!connector.isEmpty()) {
    setTarget(static_cast<int>(displays_.indexOf(connector)));
  }
}
void WallpaperController::discard() {
  pending_ = applied_;
  baseline_ = snapshot_.value(kImages);
  error_.clear();
  emit stateChanged();
}
void WallpaperController::reload() {
  const auto loaded = watcher_.read();
  watcher_.refresh();
  valid_ = false;
  if (!loaded) {
    error_ = diagnostics(loaded.diagnostics);
    emit stateChanged();
    return;
  }
  const auto decoded = HoloNight::ShellConfig::decodeDocument(*loaded.value);
  if (!decoded) {
    error_ = diagnostics(decoded.diagnostics);
    emit stateChanged();
    return;
  }
  const bool clean = pending_ == applied_;
  snapshot_ = *loaded.value;
  applied_ = decoded.value->background.images;
  if (clean) {
    pending_ = applied_;
    baseline_ = snapshot_.value(kImages);
  }
  if (pending_ == applied_) {
    baseline_ = snapshot_.value(kImages);
  }
  valid_ = true;
  error_.clear();
  emit stateChanged();
}
void WallpaperController::resolve(bool keepPending) {
  reload();
  if (!valid_) {
    return;
  }
  if (!keepPending) {
    pending_ = applied_;
  }
  baseline_ = snapshot_.value(kImages);
  emit stateChanged();
}
bool WallpaperController::apply() {
  reload();
  if (!canApply()) {
    return false;
  }
  for (const auto& image : pending_) {
    if (image.isEmpty()) {
      continue;
    }
    QImageReader reader(image);
    const auto format = reader.format().toLower();
    if (reader.size().isValid()) {
      reader.setScaledSize(reader.size().scaled(QSize(256, 256), Qt::KeepAspectRatio));
    }
    if (reader.supportsAnimation() || format == "svg" || format == "svgz" || reader.read().isNull()) {
      invalid_image_ = image;
      error_ = tr("Cannot apply unreadable wallpaper: %1").arg(image);
      emit stateChanged();
      return false;
    }
  }
  const auto saved = Config::saveDocument(watcher_.path().toStdString(),
                                          {{.key = kImages, .baseline = baseline_, .pending = encodeImages(pending_)}},
                                          HoloNight::ShellConfig::documentSchema());
  if (saved.status != Config::SaveStatus::Success) {
    error_ = diagnostics(saved.diagnostics);
    if (error_.isEmpty()) {
      error_ = tr("Could not save wallpaper configuration.");
    }
    emit stateChanged();
    return false;
  }
  applied_ = pending_;
  reload();
  return true;
}
