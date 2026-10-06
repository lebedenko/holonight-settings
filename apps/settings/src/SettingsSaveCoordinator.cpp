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
  connect(appearance_->document(), &DocumentEditSession::stateChanged, this,
          &SettingsSaveCoordinator::conflictsChanged);
  connect(shell_->document(), &DocumentEditSession::stateChanged, this, &SettingsSaveCoordinator::conflictsChanged);
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
        appearance_staged_ = false;
        ++failed_;
        finishSave(QStringLiteral("Appearance: ") + appearance_files_->error());
        return;
      }
      appearance_staged_ = false;
      const auto remaining = conflicts();
      setConflict(remaining.isEmpty() ? QString{}
                                      : remaining.first().toMap().value(QStringLiteral("domain")).toString());
      ++succeeded_;
      outcomes_ << QStringLiteral("Appearance saved");
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
                          : QStringLiteral("Appearance: ") + diagnostic + QStringLiteral("; rollback refused: ") +
                                appearance_files_->error());
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
  outcomes_.clear();
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
        outcomes_ << QStringLiteral("Appearance saved");
      }
    } else {
      ++failed_;
      if (result == AppearanceFileService::SaveResult::Conflict) {
        setConflict(QStringLiteral("Appearance"));
      }
      outcomes_ << QStringLiteral("Appearance: ") + appearance_files_->error();
    }
  }
  if (shell_->isDirty()) {
    const auto result = shell_files_->save();
    if (result == ShellConfigFileService::SaveResult::Success) {
      ++succeeded_;
      outcomes_ << QStringLiteral("Shell settings saved");
    } else {
      ++failed_;
      if (result == ShellConfigFileService::SaveResult::Conflict && conflict_domain_.isEmpty()) {
        setConflict(QStringLiteral("Shell settings"));
      }
      outcomes_ << QStringLiteral("Shell settings: ") + shell_files_->error();
    }
  }
  if (appearance_staged_) {
    adapter_->apply(appearance_files_->path());
    return;
  }
  finishSave();
}

void SettingsSaveCoordinator::finishSave(const QString& appearance_result) {
  if (!appearance_result.isEmpty()) {
    outcomes_ << appearance_result;
  } else if (appearance_staged_ || (succeeded_ > 0 && outcomes_.isEmpty())) {
    outcomes_ << QStringLiteral("Appearance saved");
  }
  setResult(outcomes_.join(QStringLiteral("; ")));
  setBusy(false);
}
void SettingsSaveCoordinator::discard() {
  if (busy_) {
    return;
  }
  setBusy(true);
  QStringList errors;
  if (!appearance_files_->load()) {
    errors << QStringLiteral("Appearance: ") + appearance_files_->error();
  }
  if (!shell_files_->load()) {
    errors << QStringLiteral("Shell settings: ") + shell_files_->error();
  }
  setResult(errors.isEmpty() ? QStringLiteral("Changes discarded") : errors.join(QStringLiteral("; ")));
  setConflict({});
  setBusy(false);
}
QVariantList SettingsSaveCoordinator::conflicts() const {
  QVariantList result;
  for (const auto& domain : {QStringLiteral("Appearance"), QStringLiteral("Shell settings")}) {
    const auto* document = domain == QStringLiteral("Appearance") ? appearance_->document() : shell_->document();
    for (const auto& item : document->conflicts()) {
      auto row = item.toMap();
      row["domain"] = domain;
      result.push_back(row);
    }
  }
  return result;
}
void SettingsSaveCoordinator::resolveConflict(const QString& domain, const QString& property, bool keepPending) {
  if (busy_) {
    return;
  }
  auto* document = domain == QStringLiteral("Appearance") ? appearance_->document() : shell_->document();
  document->resolve(property, keepPending);
  if (conflicts().isEmpty()) {
    setConflict({});
  }
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
  adapter_->status(appearance_files_->path());
}
void SettingsSaveCoordinator::restoreNativeDefaults() {
  if (busy_ || adapter_ == nullptr || appearance_->isDirty()) {
    return;
  }
  adapter_->revert();
}
