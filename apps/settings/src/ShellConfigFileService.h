#pragma once

#include "FileRevision.h"

#include <QString>

class ShellSettingsEditModel;

class ShellConfigFileService {
 public:
  enum class SaveResult { Success, Conflict, Error };
  explicit ShellConfigFileService(ShellSettingsEditModel* model, QString path = {});
  [[nodiscard]] bool load();
  [[nodiscard]] SaveResult save(bool overwrite = false);
  [[nodiscard]] QString error() const { return error_; }

 private:
  ShellSettingsEditModel* model_;
  QString path_;
  FileRevision revision_;
  FileRevision conflict_revision_;
  QString error_;
};
