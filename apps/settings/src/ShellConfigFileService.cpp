#include "ShellConfigFileService.h"

#include "ShellSettingsEditModel.h"

#include <holonight_shell_config/config_path.h>
#include <holonight_shell_config/config_schema.h>

namespace Config = HoloNight::Config;
namespace {
std::vector<DocumentEditSession::Field> shellFields() {
  using Field = DocumentEditSession::Field;
  std::vector<Field> result{
      Field{.property = "taskbarEnabled", .key = {"bar", "taskbar", "enabled"}, .label = {}},
      Field{.property = "taskbarGrouped", .key = {"bar", "taskbar", "grouped"}, .label = {}},
      Field{.property = "windowOverviewAccess", .key = {"bar", "taskbar", "overview_access"}, .label = {}},
      Field{.property = "desktopMenuEnabled", .key = {"bar", "taskbar", "desktop_menu"}, .label = {}},
      Field{.property = "workspaceCount", .key = {"bar", "workspaces", "count"}, .label = {}},
      Field{.property = "trayMaxItems", .key = {"bar", "systemtray", "max_items"}, .label = {}},
      Field{.property = "weatherProvider", .key = {"weather", "provider"}, .label = {}},
      Field{.property = "weatherLocationSource", .key = {"weather", "location_source"}, .label = {}},
      Field{
          .property = "weatherApiKey",
          .key = {"weather", "api_key"},
          .label = {},
          .enabled_property = {},
          .sensitive = true,
      },
      Field{.property = "weatherCity", .key = {"weather", "city"}, .label = {}},
      Field{.property = "weatherTempUnit", .key = {"weather", "temp_unit"}, .label = {}},
      Field{.property = "weatherWindUnit", .key = {"weather", "wind_unit"}, .label = {}},
      Field{.property = "weatherPressureUnit", .key = {"weather", "pressure_unit"}, .label = {}},
      Field{.property = "weatherShowInBar", .key = {"weather", "show_in_bar"}, .label = {}},
      Field{.property = "weatherCompactMode", .key = {"weather", "compact_mode"}, .label = {}},
      Field{.property = "weatherShowFeelsLike", .key = {"weather", "show_feels_like"}, .label = {}},
      Field{.property = "weatherShowLocation", .key = {"weather", "show_location"}, .label = {}},
      Field{.property = "weatherRefreshInterval", .key = {"weather", "refresh_interval"}, .label = {}},
  };
  for (auto& field : result) {
    for (const auto& metadata : HoloNight::ShellConfig::settingMetadata()) {
      if (metadata.key != field.key) {
        continue;
      }
      field.label = QString::fromStdString(metadata.description);
      if (metadata.minimum) {
        field.constraints["minimum"] = *metadata.minimum;
      }
      if (metadata.maximum) {
        field.constraints["maximum"] = *metadata.maximum;
      }
      QStringList choices;
      for (const auto& choice : metadata.choices) {
        choices << QString::fromStdString(choice);
      }
      field.constraints["choices"] = choices;
    }
  }
  return result;
}
}  // namespace
ShellConfigFileService::ShellConfigFileService(ShellSettingsEditModel* model, QString path)
    : model_(model), path_(path.isEmpty() ? HoloNight::ShellConfig::resolveProductConfigPath() : std::move(path)) {
  auto fields = shellFields();
  document_ = std::make_unique<DocumentEditSession>(
      model_, path_, fields, HoloNight::ShellConfig::documentSchema(),
      [fields](const Config::DocumentSnapshot& snapshot) -> Config::Result<QVariantMap> {
        const auto decoded = HoloNight::ShellConfig::decodeDocument(snapshot);
        if (!decoded) {
          return Config::Result<QVariantMap>::failure(decoded.diagnostics);
        }
        ShellSettingsEditModel temporary;
        temporary.load(*decoded.value);
        return Config::Result<QVariantMap>::success(DocumentEditSession::properties(temporary, fields),
                                                    decoded.diagnostics);
      },
      false);
  model_->attachDocument(document_.get());
}
bool ShellConfigFileService::load() {
  const bool loaded = document_->reload(true);
  error_ = document_->diagnostics();
  return loaded;
}
ShellConfigFileService::SaveResult ShellConfigFileService::save() {
  if (!document_->reload()) {
    error_ = document_->diagnostics();
    return SaveResult::Error;
  }
  const auto result =
      Config::saveDocument(path_.toStdString(), document_->edits(), HoloNight::ShellConfig::documentSchema());
  if (result.status != Config::SaveStatus::Success) {
    document_->report(result);
    error_ = document_->diagnostics();
    return result.status == Config::SaveStatus::Conflict ? SaveResult::Conflict : SaveResult::Error;
  }
  document_->accept(*result.snapshot);
  error_.clear();
  return SaveResult::Success;
}
