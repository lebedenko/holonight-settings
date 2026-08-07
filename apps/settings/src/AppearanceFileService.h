#pragma once

#include "FileRevision.h"

#include <QString>

class AppearanceEditModel;

class AppearanceFileService {
 public:
  enum class SaveResult { Success, Conflict, Error };
  explicit AppearanceFileService(AppearanceEditModel* model, QString path = {});
  [[nodiscard]] bool load();
  [[nodiscard]] SaveResult save(bool overwrite = false);
  [[nodiscard]] QString error() const { return error_; }
  [[nodiscard]] QString path() const { return path_; }

 private:
  AppearanceEditModel* model_;
  QString path_;
  FileRevision revision_;
  FileRevision conflict_revision_;
  QString error_;
  bool initialized_{false};
};
