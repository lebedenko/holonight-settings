#include "ConfigFileService.h"

#include "SettingsEditModel.h"
#include "ThemeConfigFile.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <appearanceconfig.h>
#include <holonight/config.h>
#include <holonight_config/config_parsers.h>
#include <holonight_config/config_writer.h>

Q_LOGGING_CATEGORY(lcFileService, "holonight.settings.fileservice")

ConfigFileService::ConfigFileService(SettingsEditModel* model, QObject* parent) : QObject(parent), model_(model) {}

QString ConfigFileService::configPath() {
  QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
  if (xdg.isEmpty()) {
    xdg = QDir::homePath() + QLatin1String("/.config");
  }
  return xdg + QLatin1String("/holonight/config.toml");
}

QString ConfigFileService::themeConfigPath() { return ThemeConfigFile::path(); }

QString ConfigFileService::appearanceConfigPath() { return Holonight::AppearanceConfig::configFilePath(); }

bool ConfigFileService::load() {
  const QString path = configPath();
  ParsedConfig loaded;
  const Holonight::ThemeConfig theme_config = Holonight::ThemeConfig::loadFile();
  const Holonight::AppearanceConfig shape_appearance = Holonight::AppearanceConfig::load(appearanceConfigPath());

  QFile file(path);
  if (file.exists()) {
    try {
      auto table = toml::parse_file(path.toStdString());
      MissingDefaults missing;
      loaded = parseConfigTable(table, missing);
      qCInfo(lcFileService) << "Loaded config from" << path;
    } catch (const toml::parse_error& err) {
      qCWarning(lcFileService) << "Parse error in" << path << ":" << err.description().data();
    }
  } else {
    qCInfo(lcFileService) << "Config not found, using defaults:" << path;
  }

  model_->setFromParsedConfig(loaded);
  model_->setThemeConfigSnapshot(theme_config);
  model_->setShapeAppearanceSnapshot(shape_appearance);
  return true;
}

bool ConfigFileService::save() {
  if (is_saving_) {
    return false;
  }

  is_saving_ = true;
  emit isSavingChanged();
  emit saveStarted();

  const ParsedConfig config = model_->toParsedConfig();
  const Holonight::ThemeConfig theme_config = model_->toThemeConfig();
  const Holonight::AppearanceConfig shape_appearance = model_->toShapeAppearanceConfig();
  const QString path = configPath();

  QDir dir = QFileInfo(path).dir();
  if (!dir.exists()) {
    dir.mkpath(QStringLiteral("."));
  }

  bool success = ConfigWriter::write(config, path);
  QString failed_path = path;
  QString save_error;
  if (success) {
    success = theme_config.save(&save_error);
    failed_path = Holonight::ThemeConfig::configFilePath();
  }
  QString appearance_error;
  if (success) {
    success = shape_appearance.save(appearanceConfigPath(), &appearance_error);
    failed_path = appearanceConfigPath();
  }
  if (!success) {
    QString msg = tr("Failed to write configuration to %1").arg(failed_path);
    if (!save_error.isEmpty()) {
      msg += QStringLiteral(": ") + save_error;
    } else if (!appearance_error.isEmpty()) {
      msg += QStringLiteral(": ") + appearance_error;
    }
    qCWarning(lcFileService) << msg;
    emit saveError(msg);
  } else {
    qCInfo(lcFileService) << "Saved config to" << path << "," << ThemeConfigFile::path() << "and"
                          << appearanceConfigPath();
    model_->markSaved(config, theme_config, shape_appearance);
  }

  is_saving_ = false;
  emit isSavingChanged();
  emit saveFinished(success);
  return success;
}
