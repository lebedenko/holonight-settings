#include "SettingsApplication.h"

#include <QGuiApplication>

int main(int argc, char* argv[]) {
  SettingsApplication app(argc, argv);
  if (!app.shouldRun()) {
    return app.startupExitCode();
  }
  return QGuiApplication::exec();
}
