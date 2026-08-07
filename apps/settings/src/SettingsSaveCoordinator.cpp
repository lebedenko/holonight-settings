#include "SettingsSaveCoordinator.h"

#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"

SettingsSaveCoordinator::SettingsSaveCoordinator(AppearanceEditModel* appearance,
                                                 AppearanceFileService* appearance_files, ShellSettingsEditModel* shell,
                                                 ShellConfigFileService* shell_files, QObject* parent)
    : QObject(parent),
      appearance_(appearance),
      appearance_files_(appearance_files),
      shell_(shell),
      shell_files_(shell_files) {
  connect(appearance_, &AppearanceEditModel::isDirtyChanged, this, &SettingsSaveCoordinator::isDirtyChanged);
  connect(shell_, &ShellSettingsEditModel::isDirtyChanged, this, &SettingsSaveCoordinator::isDirtyChanged);
}
bool SettingsSaveCoordinator::isDirty() const { return appearance_->isDirty() || shell_->isDirty(); }
void SettingsSaveCoordinator::setBusy(bool value) {
  if (busy_ == value) {
    return;
  }
  busy_ = value;
  emit isBusyChanged();
}
void SettingsSaveCoordinator::setResult(QString value) {
  if (result_text_ == value) {
    return;
  }
  result_text_ = std::move(value);
  emit resultTextChanged();
}
void SettingsSaveCoordinator::setConflict(QString value) {
  if (conflict_domain_ == value) {
    return;
  }
  conflict_domain_ = std::move(value);
  emit conflictDomainChanged();
}

void SettingsSaveCoordinator::save() {
  if (busy_ || !isDirty()) {
    return;
  }
  setBusy(true);
  setConflict({});
  int succeeded = 0;
  int failed = 0;
  if (appearance_->isDirty()) {
    const auto result = appearance_files_->save();
    if (result == AppearanceFileService::SaveResult::Success) {
      ++succeeded;
    } else {
      ++failed;
      if (result == AppearanceFileService::SaveResult::Conflict) {
        setConflict(QStringLiteral("Appearance"));
      }
      setResult(QStringLiteral("Appearance: ") + appearance_files_->error());
    }
  }
  if (shell_->isDirty()) {
    const auto result = shell_files_->save();
    if (result == ShellConfigFileService::SaveResult::Success) {
      ++succeeded;
    } else {
      ++failed;
      if (result == ShellConfigFileService::SaveResult::Conflict && conflict_domain_.isEmpty()) {
        setConflict(QStringLiteral("Shell settings"));
      }
      setResult(QStringLiteral("Shell settings: ") + shell_files_->error());
    }
  }
  if (failed == 0) {
    setResult(QStringLiteral("Changes saved"));
  } else if (succeeded > 0) {
    setResult(QStringLiteral("Some changes saved; remaining domain needs attention"));
  }
  setBusy(false);
}
void SettingsSaveCoordinator::discard() {
  if (busy_ || !isDirty()) {
    return;
  }
  setBusy(true);
  QStringList errors;
  if (appearance_->isDirty() && !appearance_files_->load()) {
    errors << QStringLiteral("Appearance: ") + appearance_files_->error();
  }
  if (shell_->isDirty() && !shell_files_->load()) {
    errors << QStringLiteral("Shell settings: ") + shell_files_->error();
  }
  setResult(errors.isEmpty() ? QStringLiteral("Changes discarded") : errors.join(QStringLiteral("; ")));
  setConflict({});
  setBusy(false);
}
void SettingsSaveCoordinator::reloadConflict() {
  if (busy_ || conflict_domain_.isEmpty()) {
    return;
  }
  setBusy(true);
  const bool succeeded =
      conflict_domain_ == QStringLiteral("Appearance") ? appearance_files_->load() : shell_files_->load();
  setResult(succeeded ? QStringLiteral("External changes loaded") : QStringLiteral("Reload failed"));
  if (succeeded) {
    setConflict({});
  }
  setBusy(false);
}
void SettingsSaveCoordinator::overwriteConflict() {
  if (busy_ || conflict_domain_.isEmpty()) {
    return;
  }
  setBusy(true);
  bool succeeded = false;
  if (conflict_domain_ == QStringLiteral("Appearance")) {
    succeeded = appearance_files_->save(true) == AppearanceFileService::SaveResult::Success;
  } else {
    succeeded = shell_files_->save(true) == ShellConfigFileService::SaveResult::Success;
  }
  setResult(succeeded ? QStringLiteral("External changes overwritten")
                      : QStringLiteral("File changed again; reload before overwriting"));
  if (succeeded) {
    setConflict({});
  }
  setBusy(false);
}
void SettingsSaveCoordinator::cancelConflict() {
  setConflict({});
  setResult(QStringLiteral("Save cancelled; edits retained"));
}
