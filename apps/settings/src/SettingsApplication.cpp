#include "SettingsApplication.h"

#include "AppearanceAdapterClient.h"
#include "AppearanceEditModel.h"
#include "AppearanceFileService.h"
#include "AudioControllerQml.h"
#include "SettingsActivationService.h"
#include "SettingsSaveCoordinator.h"
#include "ShellConfigFileService.h"
#include "ShellSettingsEditModel.h"
#include "ShellStatusService.h"
#include "WallpaperController.h"
#include "WallpaperModel.h"

#include <QDebug>
#include <QDir>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <cstdlib>

SettingsApplication::SettingsApplication(int& argc, char** argv) : QGuiApplication(argc, argv) {
  setApplicationName(QStringLiteral("holonight-settings"));
  setApplicationVersion(QStringLiteral(HOLONIGHT_SETTINGS_VERSION));
  setDesktopFileName(QStringLiteral("org.holonight.Settings"));

  activation_service_ = std::make_unique<SettingsActivationService>();
  const SettingsActivationService::StartupRole role = activation_service_->arbitrate(
      SettingsActivationService::platformDataFromEnvironment(), arguments().contains(QStringLiteral("--wallpaper")));
  if (role == SettingsActivationService::StartupRole::Secondary) {
    return;
  }
  if (role == SettingsActivationService::StartupRole::Error) {
    qCritical().noquote() << activation_service_->errorString();
    startup_exit_code_ = EXIT_FAILURE;
    return;
  }
  should_run_ = true;

  wallpaper_controller_ = std::make_unique<WallpaperController>();
  connect(activation_service_.get(), &SettingsActivationService::settingsRequested, this,
          &SettingsApplication::openSettings);
  connect(activation_service_.get(), &SettingsActivationService::wallpaperRequested, this,
          &SettingsApplication::openWallpaper);
  connect(wallpaper_controller_.get(), &WallpaperController::openRequested, this,
          [this] { activation_service_->ActivateAction(QStringLiteral("wallpaper"), {}, {}); });
  if (arguments().contains(QStringLiteral("--wallpaper"))) {
    openWallpaper({});
  } else {
    openSettings();
  }
}

void SettingsApplication::configureEngine(QQmlApplicationEngine* engine) {
  engine->addImageProvider(QStringLiteral("wallpaper"), new WallpaperImageProvider);
  const QString executable_dir = applicationDirPath();
  if (executable_dir == QStringLiteral(SETTINGS_BUILD_DIR)) {
    engine->addImportPath(QStringLiteral(SETTINGS_DEPENDENCY_QML_DIR));
  } else {
    engine->addImportPath(QDir(executable_dir).absoluteFilePath(QStringLiteral("../" SETTINGS_INSTALLED_QML_DIR)));
  }
  connect(
      engine, &QQmlApplicationEngine::objectCreationFailed, this, [] { QCoreApplication::exit(EXIT_FAILURE); },
      Qt::QueuedConnection);
}

void SettingsApplication::openSettings() {
  if (engine_) {
    return;
  }
  appearance_model_ = std::make_unique<AppearanceEditModel>();
  shell_model_ = std::make_unique<ShellSettingsEditModel>();
  appearance_files_ = std::make_unique<AppearanceFileService>(appearance_model_.get());
  appearance_adapter_ = std::make_unique<AppearanceAdapterClient>();
  shell_files_ = std::make_unique<ShellConfigFileService>(shell_model_.get());
  save_coordinator_ =
      std::make_unique<SettingsSaveCoordinator>(appearance_model_.get(), appearance_files_.get(), shell_model_.get(),
                                                shell_files_.get(), appearance_adapter_.get());
  shell_status_ = std::make_unique<ShellStatusService>();
  audio_controller_ = std::make_unique<AudioControllerQml>();
  audio_controller_->start();

  static_cast<void>(appearance_files_->load());
  static_cast<void>(shell_files_->load());

  engine_ = std::make_unique<QQmlApplicationEngine>();
  configureEngine(engine_.get());
  engine_->setInitialProperties({
      {QStringLiteral("appearanceModel"), QVariant::fromValue(appearance_model_.get())},
      {QStringLiteral("shellModel"), QVariant::fromValue(shell_model_.get())},
      {QStringLiteral("saveCoordinator"), QVariant::fromValue(save_coordinator_.get())},
      {QStringLiteral("appearanceAdapter"), QVariant::fromValue(appearance_adapter_.get())},
      {QStringLiteral("shellStatus"), QVariant::fromValue(shell_status_.get())},
      {QStringLiteral("audioController"), QVariant::fromValue(audio_controller_.get())},
      {QStringLiteral("appVersion"), applicationVersion()},
      {QStringLiteral("wallpaperController"), QVariant::fromValue(wallpaper_controller_.get())},
  });

  engine_->loadFromModule(QStringLiteral("HolonightSettings"), QStringLiteral("SettingsWindow"));
  if (!engine_->rootObjects().isEmpty()) {
    QObject* root = engine_->rootObjects().constFirst();
    connect(
        activation_service_.get(), &SettingsActivationService::pageRequested, root,
        [root](const QString& page_key) { QMetaObject::invokeMethod(root, "requestPage", Q_ARG(QVariant, page_key)); });
    activation_service_->setWindow(qobject_cast<QQuickWindow*>(root));
  }
}

void SettingsApplication::openWallpaper(const QString& connector) {
  if (wallpaper_engine_) {
    auto* window = qobject_cast<QQuickWindow*>(wallpaper_engine_->rootObjects().value(0));
    if ((window != nullptr) && !window->isVisible()) {
      wallpaper_controller_->reopen(connector);
      wallpaper_model_->refresh();
    } else if (!connector.isEmpty()) {
      {
        wallpaper_controller_->reopen(connector);
      }
    }
    return;
  }
  wallpaper_controller_->reopen(connector);
  wallpaper_model_ = std::make_unique<WallpaperModel>();
  wallpaper_filter_ = std::make_unique<WallpaperFilter>(wallpaper_model_.get());
  wallpaper_model_->refresh();
  wallpaper_engine_ = std::make_unique<QQmlApplicationEngine>();
  configureEngine(wallpaper_engine_.get());
  connect(wallpaper_model_.get(), &WallpaperModel::stateChanged, wallpaper_controller_.get(),
          &WallpaperController::stateChanged);
  wallpaper_engine_->setInitialProperties({
      {QStringLiteral("controller"), QVariant::fromValue(wallpaper_controller_.get())},
      {QStringLiteral("browser"), QVariant::fromValue(wallpaper_model_.get())},
      {QStringLiteral("wallpapers"), QVariant::fromValue(wallpaper_filter_.get())},
  });
  wallpaper_engine_->loadFromModule(QStringLiteral("HolonightSettings"), QStringLiteral("WallpaperWindow"));
  activation_service_->setWallpaperWindow(qobject_cast<QQuickWindow*>(wallpaper_engine_->rootObjects().value(0)));
}

SettingsApplication::~SettingsApplication() {
  engine_.reset();
  wallpaper_engine_.reset();
}

bool SettingsApplication::shouldRun() const { return should_run_; }

int SettingsApplication::startupExitCode() const { return startup_exit_code_; }
