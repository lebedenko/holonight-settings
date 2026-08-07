#include "ShellConfigFileService.h"

#include "ShellSettingsEditModel.h"

#include <QFile>

#include <holonight_shell_config/config_path.h>
#include <holonight_shell_config/config_writer.h>

ShellConfigFileService::ShellConfigFileService(ShellSettingsEditModel* model, QString path)
    : model_(model), path_(path.isEmpty() ? HoloNight::ShellConfig::resolveProductConfigPath() : std::move(path)) {}

bool ShellConfigFileService::load() {
  QFile file(path_);
  HoloNight::ShellConfig::ProductConfig value;
  revision_ = readFileRevision(path_);
  if (revision_.exists) {
    if (!file.open(QIODevice::ReadOnly)) {
      error_ = QStringLiteral("Unable to read Shell settings");
      return false;
    }
    try {
      auto table = toml::parse(file.readAll().toStdString());
      HoloNight::ShellConfig::MissingDefaults missing;
      value = HoloNight::ShellConfig::parseConfigTable(table, missing);
    } catch (const toml::parse_error&) {
      error_ = QStringLiteral("Shell settings contain invalid TOML");
      return false;
    }
  }
  model_->load(value);
  error_.clear();
  return true;
}

ShellConfigFileService::SaveResult ShellConfigFileService::save(bool overwrite) {
  const FileRevision disk = readFileRevision(path_);
  if (!overwrite && disk != revision_) {
    conflict_revision_ = disk;
    error_ = QStringLiteral("Shell settings changed outside Settings");
    return SaveResult::Conflict;
  }
  if (overwrite && disk != conflict_revision_) {
    conflict_revision_ = disk;
    error_ = QStringLiteral("Shell settings changed again; review the latest version");
    return SaveResult::Conflict;
  }
  if (!HoloNight::ShellConfig::ProductConfigWriter::write(model_->value(), path_)) {
    error_ = QStringLiteral("Unable to write Shell settings");
    return SaveResult::Error;
  }
  revision_ = readFileRevision(path_);
  conflict_revision_ = {};
  model_->markSaved();
  error_.clear();
  return SaveResult::Success;
}
