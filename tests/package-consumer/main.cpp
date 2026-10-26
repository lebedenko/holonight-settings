#include <QCoreApplication>
#include <QTemporaryDir>

#include <holonight/config/appearance.h>
#include <holonight/document_watcher.h>
#include <holonight_shell_config/config_path.h>

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  Holonight::DocumentWatcher watcher(directory.filePath(QStringLiteral("config.toml")));
  QObject::connect(&watcher, &Holonight::DocumentWatcher::documentChanged, &app, [] {});
  watcher.refresh();
  if (!watcher.read()) {
    return 1;
  }
  const auto appearance = HoloNight::Config::defaults();
  const auto path = HoloNight::ShellConfig::resolveProductConfigPath(
      {{QStringLiteral("XDG_CONFIG_HOME"), QStringLiteral("/tmp/acf005-consumer")}});
  return appearance.theme.scheme == "holonight-dark" && path.endsWith(QStringLiteral("/holonight/config.toml")) ? 0 : 1;
}
