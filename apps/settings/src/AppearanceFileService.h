#pragma once

#include "DocumentEditSession.h"

#include <QFile>
#include <QString>

#include <cstdint>
#include <memory>

class AppearanceEditModel;

class AppearanceFileService {
 public:
  enum class SaveResult : std::uint8_t { Success, Conflict, Error };
  explicit AppearanceFileService(AppearanceEditModel* model, QString path = {});
  [[nodiscard]] bool load();
  [[nodiscard]] SaveResult save();
  [[nodiscard]] SaveResult stage();
  [[nodiscard]] bool commit();
  [[nodiscard]] bool rollback();
  [[nodiscard]] QString error() const { return error_; }
  [[nodiscard]] QString path() const { return path_; }

 private:
  AppearanceEditModel* model_;
  QString path_;
  std::unique_ptr<DocumentEditSession> document_;
  QString error_;
  std::optional<HoloNight::Config::StagedSaveResult> staged_result_;
  HoloNight::Config::EditBatch staged_edits_;
  bool staged_{false};
};
