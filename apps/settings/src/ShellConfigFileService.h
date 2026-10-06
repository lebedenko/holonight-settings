#pragma once

#include "DocumentEditSession.h"

#include <QString>

#include <cstdint>
#include <memory>

class ShellSettingsEditModel;

class ShellConfigFileService {
 public:
  enum class SaveResult : std::uint8_t { Success, Conflict, Error };
  explicit ShellConfigFileService(ShellSettingsEditModel* model, QString path = {});
  [[nodiscard]] bool load();
  [[nodiscard]] SaveResult save();
  [[nodiscard]] QString error() const { return error_; }

 private:
  ShellSettingsEditModel* model_;
  QString path_;
  std::unique_ptr<DocumentEditSession> document_;
  QString error_;
};
