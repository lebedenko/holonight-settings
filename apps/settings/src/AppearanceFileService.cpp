#include "AppearanceFileService.h"

#include "AppearanceEditModel.h"

#include <holonight/appearance.h>
#include <holonight/config/path.h>
#include <holonight/config/store.h>

namespace {
QString diagnosticText(const std::vector<HoloNight::Config::Diagnostic>& diagnostics) {
  return diagnostics.empty() ? QStringLiteral("Appearance operation failed")
                             : QString::fromStdString(diagnostics.front().message);
}
}  // namespace

AppearanceFileService::AppearanceFileService(AppearanceEditModel* model, QString path)
    : model_(model), path_(std::move(path)) {
  if (path_.isEmpty()) {
    const auto resolved = HoloNight::Config::resolveAppearancePath();
    if (resolved) {
      path_ = QString::fromStdString(resolved.value->string());
    }
  }
}

bool AppearanceFileService::load() {
  if (path_.isEmpty()) {
    error_ = QStringLiteral("Appearance path is unavailable");
    return false;
  }
  const auto loaded = HoloNight::Config::load(path_.toStdString());
  revision_ = readFileRevision(path_);
  if (!loaded) {
    error_ = diagnosticText(loaded.diagnostics);
    if (!revision_.exists) {
      return false;
    }
    if (!initialized_) {
      model_->loadInvalidDefaults();
    }
    model_->setValidationError(error_);
    initialized_ = true;
    return false;
  }
  model_->load(loaded.value->appearance);
  initialized_ = true;
  error_.clear();
  return true;
}

AppearanceFileService::SaveResult AppearanceFileService::save(bool overwrite) {
  const FileRevision disk = readFileRevision(path_);
  if (!overwrite && disk != revision_) {
    conflict_revision_ = disk;
    error_ = QStringLiteral("Appearance changed outside Settings");
    return SaveResult::Conflict;
  }
  if (overwrite && disk != conflict_revision_) {
    conflict_revision_ = disk;
    error_ = QStringLiteral("Appearance changed again; review the latest version");
    return SaveResult::Conflict;
  }
  const auto neutral = HoloNight::Config::validate(model_->value());
  if (!neutral.empty()) {
    error_ = diagnosticText(neutral);
    model_->setValidationError(error_);
    return SaveResult::Error;
  }
  const auto resolved = Holonight::resolveAppearance(model_->value());
  if (!resolved) {
    error_ = resolved.diagnostics.isEmpty() ? QStringLiteral("Unsupported appearance")
                                            : resolved.diagnostics.first().message;
    model_->setValidationError(error_);
    return SaveResult::Error;
  }
  const auto written = HoloNight::Config::writeAtomically(model_->value(), path_.toStdString());
  if (!written) {
    error_ = diagnosticText(written.diagnostics);
    return SaveResult::Error;
  }
  revision_ = readFileRevision(path_);
  conflict_revision_ = {};
  model_->markSaved();
  model_->setValidationError({});
  error_.clear();
  return SaveResult::Success;
}
