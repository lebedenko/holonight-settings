#pragma once

#include <QString>

#include <holonight_config/config_parsers.h>

class ConfigWriter {
 public:
  ConfigWriter() = delete;

  // Serializes config to TOML in canonical section order and writes it atomically
  // via QSaveFile. Creates parent directories if absent. Returns false on I/O failure.
  [[nodiscard]] static bool write(const ParsedConfig& config, const QString& path);
};
