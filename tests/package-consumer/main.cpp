#include <holonight/config/appearance.h>
#include <holonight_shell_config/config_path.h>

int main() {
  const auto appearance = HoloNight::Config::defaults();
  const auto path = HoloNight::ShellConfig::resolveProductConfigPath(
      {{QStringLiteral("XDG_CONFIG_HOME"), QStringLiteral("/tmp/acf005-consumer")}});
  return appearance.theme.scheme == "holonight-dark" && path.endsWith(QStringLiteral("/holonight/config.toml")) ? 0 : 1;
}
