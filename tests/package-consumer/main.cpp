#include <holonight_config/config_parsers.h>

int main() {
  const ParsedConfig config;
  return config.bar_workspaces.count > 0 ? 0 : 1;
}
