#pragma once

#include <QGuiApplication>

#include <memory>

class QQmlApplicationEngine;
class SettingsEditModel;
class ConfigFileService;
class ShellStatusService;
class SettingsActivationService;

class SettingsApplication : public QGuiApplication {
  Q_OBJECT

 public:
  SettingsApplication(int& argc, char** argv);
  ~SettingsApplication() override;

  [[nodiscard]] bool shouldRun() const;
  [[nodiscard]] int startupExitCode() const;

  SettingsApplication(const SettingsApplication&) = delete;
  SettingsApplication& operator=(const SettingsApplication&) = delete;
  SettingsApplication(SettingsApplication&&) = delete;
  SettingsApplication& operator=(SettingsApplication&&) = delete;

 private:
  std::unique_ptr<SettingsActivationService> activation_service_;
  std::unique_ptr<SettingsEditModel> edit_model_;
  std::unique_ptr<ConfigFileService> file_service_;
  std::unique_ptr<ShellStatusService> shell_status_;
  std::unique_ptr<QQmlApplicationEngine> engine_;
  bool should_run_ = false;
  int startup_exit_code_ = 0;
};
