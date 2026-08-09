#include "SettingsSaveCoordinator.h"

#include "AppearanceAdapterClient.h"
#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"

SettingsSaveCoordinator::SettingsSaveCoordinator(AppearanceEditModel* appearance,
                                                 AppearanceFileService* appearance_files, ShellSettingsEditModel* shell,
                                                 ShellConfigFileService* shell_files, AppearanceAdapterClient* adapter,
                                                 QObject* parent)
    : QObject(parent),
      appearance_(appearance),
      appearance_files_(appearance_files),
      shell_(shell),
      shell_files_(shell_files),
      adapter_(adapter) {
  connect(appearance_, &AppearanceEditModel::isDirtyChanged, this, &SettingsSaveCoordinator::isDirtyChanged);
  connect(shell_, &ShellSettingsEditModel::isDirtyChanged, this, &SettingsSaveCoordinator::isDirtyChanged);
  if (adapter_ != nullptr) {
    connect(adapter_, &AppearanceAdapterClient::completed, this, [this](const AppearanceAdapterResponse& response) {
      if (!busy_) {
        return;
      }
      if (!appearance_staged_) {
        setResult(adapter_->resultText());
        setBusy(false);
        return;
      }
      if (!appearance_files_->commit()) {
        ++failed_;
        finishSave(QStringLiteral("Appearance: unable to commit saved appearance"));
        return;
      }
      appearance_staged_ = false;
      setConflict({});
      ++succeeded_;
      finishSave(response.result == QStringLiteral("degraded")
                     ? QStringLiteral("Appearance saved with limited native toolkit propagation")
                     : QString{});
    });
    connect(adapter_, &AppearanceAdapterClient::failed, this, [this](const QString& diagnostic) {
      if (!busy_) {
        return;
      }
      if (!appearance_staged_) {
        setResult(diagnostic);
        setBusy(false);
        return;
      }
      const bool restored = appearance_files_->rollback();
      appearance_staged_ = false;
      ++failed_;
      finishSave(restored ? QStringLiteral("Appearance: ") + diagnostic
                          : QStringLiteral("Appearance rollback failed; edits were retained"));
    });
  }
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
  succeeded_ = 0;
  failed_ = 0;
  appearance_staged_ = false;
  if (appearance_->isDirty()) {
    const auto result = adapter_ != nullptr ? appearance_files_->stage() : appearance_files_->save();
    if (result == AppearanceFileService::SaveResult::Success) {
      if (adapter_ != nullptr) {
        appearance_staged_ = true;
      } else {
        ++succeeded_;
      }
    } else {
      ++failed_;
      if (result == AppearanceFileService::SaveResult::Conflict) {
        setConflict(QStringLiteral("Appearance"));
      }
      setResult(QStringLiteral("Appearance: ") + appearance_files_->error());
    }
  }
  if (shell_->isDirty()) {
    const auto result = shell_files_->save();
    if (result == ShellConfigFileService::SaveResult::Success) {
      ++succeeded_;
    } else {
      ++failed_;
      if (result == ShellConfigFileService::SaveResult::Conflict && conflict_domain_.isEmpty()) {
        setConflict(QStringLiteral("Shell settings"));
      }
      setResult(QStringLiteral("Shell settings: ") + shell_files_->error());
    }
  }
  if (appearance_staged_) {
    adapter_->apply(appearance_files_->path());
    return;
  }
  finishSave();
}

void SettingsSaveCoordinator::finishSave(QString appearance_result) {
  if (failed_ == 0) {
    setResult(appearance_result.isEmpty() ? QStringLiteral("Changes saved") : std::move(appearance_result));
  } else if (succeeded_ > 0) {
    setResult(QStringLiteral("Some changes saved; remaining domain needs attention"));
  } else if (!appearance_result.isEmpty()) {
    setResult(std::move(appearance_result));
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
    if (adapter_ != nullptr) {
      const auto staged = appearance_files_->stage(true);
      if (staged == AppearanceFileService::SaveResult::Success) {
        succeeded_ = 0;
        failed_ = 0;
        appearance_staged_ = true;
        adapter_->apply(appearance_files_->path());
        return;
      }
    } else {
      succeeded = appearance_files_->save(true) == AppearanceFileService::SaveResult::Success;
    }
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

void SettingsSaveCoordinator::reapplyAppearance() {
  if (busy_ || adapter_ == nullptr || appearance_->isDirty()) {
    return;
  }
  setBusy(true);
  adapter_->apply(appearance_files_->path());
}
void SettingsSaveCoordinator::refreshIntegrations() {
  if (busy_ || adapter_ == nullptr) {
    return;
  }
  adapter_->status();
}
void SettingsSaveCoordinator::restoreNativeDefaults() {
  if (busy_ || adapter_ == nullptr || appearance_->isDirty()) {
    return;
  }
  adapter_->revert();
}
