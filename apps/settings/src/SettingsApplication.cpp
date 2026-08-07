#include "SettingsApplication.h"

#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "SettingsActivationService.h"
#include "SettingsSaveCoordinator.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"
#include "ShellStatusService.h"

#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <cstdlib>

SettingsApplication::SettingsApplication(int& argc, char** argv) : QGuiApplication(argc, argv) {
  setApplicationName(QStringLiteral("holonight-settings"));
  setApplicationVersion(QStringLiteral(HOLONIGHT_SETTINGS_VERSION));
  setDesktopFileName(QStringLiteral("org.holonight.Settings"));

  activation_service_ = std::make_unique<SettingsActivationService>();
  const SettingsActivationService::StartupRole role =
      activation_service_->arbitrate(SettingsActivationService::platformDataFromEnvironment());
  if (role == SettingsActivationService::StartupRole::Secondary) {
    return;
  }
  if (role == SettingsActivationService::StartupRole::Error) {
    qCritical().noquote() << activation_service_->errorString();
    startup_exit_code_ = EXIT_FAILURE;
    return;
  }
  should_run_ = true;

  appearance_model_ = std::make_unique<AppearanceEditModel>();
  shell_model_ = std::make_unique<ShellSettingsEditModel>();
  appearance_files_ = std::make_unique<AppearanceFileService>(appearance_model_.get());
  shell_files_ = std::make_unique<ShellConfigFileService>(shell_model_.get());
  save_coordinator_ = std::make_unique<SettingsSaveCoordinator>(appearance_model_.get(), appearance_files_.get(),
                                                                shell_model_.get(), shell_files_.get());
  shell_status_ = std::make_unique<ShellStatusService>();

  static_cast<void>(appearance_files_->load());
  static_cast<void>(shell_files_->load());

  engine_ = std::make_unique<QQmlApplicationEngine>();
  engine_->setInitialProperties({
      {QStringLiteral("appearanceModel"), QVariant::fromValue(appearance_model_.get())},
      {QStringLiteral("shellModel"), QVariant::fromValue(shell_model_.get())},
      {QStringLiteral("saveCoordinator"), QVariant::fromValue(save_coordinator_.get())},
      {QStringLiteral("shellStatus"), QVariant::fromValue(shell_status_.get())},
      {QStringLiteral("appVersion"), applicationVersion()},
  });

  connect(
      engine_.get(), &QQmlApplicationEngine::objectCreationFailed, this, []() { QCoreApplication::exit(EXIT_FAILURE); },
      Qt::QueuedConnection);
  engine_->loadFromModule(QStringLiteral("HolonightSettings"), QStringLiteral("SettingsWindow"));
  if (!engine_->rootObjects().isEmpty()) {
    activation_service_->setWindow(qobject_cast<QQuickWindow*>(engine_->rootObjects().constFirst()));
  }
}

SettingsApplication::~SettingsApplication() = default;

bool SettingsApplication::shouldRun() const { return should_run_; }

int SettingsApplication::startupExitCode() const { return startup_exit_code_; }
