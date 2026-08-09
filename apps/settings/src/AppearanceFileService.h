#pragma once

#include "FileRevision.h"

#include <QFile>
#include <QString>

#include <cstdint>

class AppearanceEditModel;

class AppearanceFileService {
 public:
  enum class SaveResult : std::uint8_t { Success, Conflict, Error };
  explicit AppearanceFileService(AppearanceEditModel* model, QString path = {});
  [[nodiscard]] bool load();
  [[nodiscard]] SaveResult save(bool overwrite = false);
  [[nodiscard]] SaveResult stage(bool overwrite = false);
  [[nodiscard]] bool commit();
  [[nodiscard]] bool rollback();
  [[nodiscard]] QString error() const { return error_; }
  [[nodiscard]] QString path() const { return path_; }

 private:
  AppearanceEditModel* model_;
  QString path_;
  FileRevision revision_;
  FileRevision conflict_revision_;
  QString error_;
  bool initialized_{false};
  QByteArray previous_contents_;
  QFile::Permissions previous_permissions_;
  bool previous_existed_{false};
  bool staged_{false};
};
