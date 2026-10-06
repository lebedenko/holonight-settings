#include "AppearanceFileService.h"

#include "AppearanceEditModel.h"

#include <holonight/appearance.h>
#include <holonight/config/appearance_document.h>
#include <holonight/config/path.h>

namespace Config = HoloNight::Config;
namespace {
std::vector<DocumentEditSession::Field> appearanceFields() {
  using Field = DocumentEditSession::Field;
  return {
      Field{.property = "themeScheme", .key = {"theme", "scheme"}, .label = "Color scheme"},
      Field{.property = "themeAccent", .key = {"theme", "accent"}, .label = "Accent"},
      Field{.property = "uiFont", .key = {"typography", "ui_family"}, .label = "Interface font"},
      Field{.property = "uiFontSize", .key = {"typography", "ui_size"}, .label = "Interface font size"},
      Field{.property = "monospaceFont", .key = {"typography", "monospace_family"}, .label = "Monospace font"},
      Field{.property = "monospaceFontSize", .key = {"typography", "monospace_size"}, .label = "Monospace font size"},
      Field{.property = "titleFont", .key = {"typography", "title_family"}, .label = "Title font"},
      Field{.property = "titleFontSize", .key = {"typography", "title_size"}, .label = "Title font size"},
      Field{.property = "displayFont", .key = {"typography", "display_family"}, .label = "Display font"},
      Field{.property = "displayFontSize", .key = {"typography", "display_size"}, .label = "Display font size"},
      Field{.property = "iconTheme", .key = {"icons", "theme"}, .label = "Icon theme"},
      Field{.property = "fallbackIconTheme", .key = {"icons", "fallback"}, .label = "Fallback icons"},
      Field{.property = "cursorTheme", .key = {"icons", "cursor"}, .label = "Cursor theme"},
      Field{.property = "layoutScale", .key = {"layout", "scale"}, .label = "Layout scale"},
      Field{.property = "shapeStyle", .key = {"shape", "style"}, .label = "Shape style"},
      Field{.property = "shapeScale", .key = {"shape", "scale"}, .label = "Shape scale"},
      Field{
          .property = "baseRadius",
          .key = {"shape", "base_radius"},
          .label = "Base radius",
          .enabled_property = "baseRadiusEnabled",
      },
      Field{
          .property = "baseChamfer",
          .key = {"shape", "base_chamfer"},
          .label = "Base chamfer",
          .enabled_property = "baseChamferEnabled",
      },
  };
}
}  // namespace
AppearanceFileService::AppearanceFileService(AppearanceEditModel* model, QString path)
    : model_(model), path_(std::move(path)) {
  if (path_.isEmpty()) {
    const auto resolved = Config::resolveAppearancePath();
    if (resolved) {
      path_ = QString::fromStdString(resolved.value->string());
    }
  }
  auto fields = appearanceFields();
  document_ = std::make_unique<DocumentEditSession>(
      model_, path_, fields, Config::appearanceDocumentSchema(),
      [fields](const Config::DocumentSnapshot& snapshot) -> Config::Result<QVariantMap> {
        const auto decoded = Config::decodeAppearanceDocument(snapshot);
        if (!decoded) {
          return Config::Result<QVariantMap>::failure(decoded.diagnostics);
        }
        const auto resolved = Holonight::resolveAppearance(decoded.value->appearance);
        if (!resolved) {
          auto diagnostics = decoded.diagnostics;
          for (const auto& note : resolved.diagnostics) {
            diagnostics.push_back({
                .code = Config::ErrorCode::ValidationError,
                .severity = Config::Severity::Error,
                .message = note.message.toStdString(),
                .path = snapshot.path,
                .position = std::nullopt,
            });
          }
          return Config::Result<QVariantMap>::failure(std::move(diagnostics));
        }
        AppearanceEditModel temporary;
        temporary.load(decoded.value->appearance);
        return Config::Result<QVariantMap>::success(DocumentEditSession::properties(temporary, fields),
                                                    decoded.diagnostics);
      },
      true);
  model_->attachDocument(document_.get());
}
bool AppearanceFileService::load() {
  const bool loaded = document_->reload(true);
  error_ = document_->diagnostics();
  model_->setValidationError(error_);
  return loaded;
}
AppearanceFileService::SaveResult AppearanceFileService::save() {
  const auto result = stage();
  if (result != SaveResult::Success) {
    return result;
  }
  return commit() ? SaveResult::Success : SaveResult::Error;
}
AppearanceFileService::SaveResult AppearanceFileService::stage() {
  if (staged_) {
    error_ = QStringLiteral("Appearance save is already in progress");
    return SaveResult::Error;
  }
  if (!document_->reload()) {
    error_ = document_->diagnostics();
    return SaveResult::Error;
  }
  staged_edits_ = document_->edits();
  staged_result_ = Config::stageAppearanceDocument(path_.toStdString(), document_->edits());
  const auto& result = staged_result_->result;
  if (result.status != Config::SaveStatus::Success) {
    document_->report(result);
    error_ = document_->diagnostics();
    model_->setValidationError(error_);
    return result.status == Config::SaveStatus::Conflict ? SaveResult::Conflict : SaveResult::Error;
  }
  document_->suspend(true);
  staged_ = true;
  error_.clear();
  return SaveResult::Success;
}
bool AppearanceFileService::commit() {
  if (!staged_) {
    return false;
  }
  const auto target = Config::resolveDocumentTarget(path_.toStdString());
  const auto disk = Config::readDocument(path_.toStdString());
  staged_ = false;
  if (!target || !disk || *target.value != staged_result_->previous->path ||
      disk.value->revision != staged_result_->result.snapshot->revision) {
    error_ = QStringLiteral("Appearance changed during application; the external document was preserved");
    document_->retry();
    document_->suspend(false);
    return false;
  }
  document_->accept(*staged_result_->result.snapshot, staged_edits_);
  error_.clear();
  model_->setValidationError({});
  return true;
}
bool AppearanceFileService::rollback() {
  if (!staged_) {
    return false;
  }
  const auto result = Config::restoreDocument(path_.toStdString(), staged_result_->result.snapshot->revision,
                                              *staged_result_->previous, Config::appearanceDocumentSchema());
  staged_ = false;
  document_->retry();
  document_->suspend(false);
  if (result.status != Config::SaveStatus::Success) {
    document_->report(result);
    error_ = document_->diagnostics();
    return false;
  }
  error_.clear();
  return true;
}
