#pragma once

#include <QString>

class ThemeConfigFile {
 public:
  struct Appearance {
    QString scheme;
    QString accent;
    QString mode;
  };

  ThemeConfigFile() = delete;

  [[nodiscard]] static QString path();
  [[nodiscard]] static QString defaultMode();
  [[nodiscard]] static QString defaultScheme();
  [[nodiscard]] static QString defaultAccent();
  [[nodiscard]] static QString normalizeMode(const QString& mode);
  [[nodiscard]] static QString normalizeScheme(const QString& scheme);
  [[nodiscard]] static QString normalizeAccent(const QString& accent);
  [[nodiscard]] static QString modeForScheme(const QString& scheme);
  [[nodiscard]] static QString schemeForMode(const QString& mode);
  [[nodiscard]] static QString siblingSchemeForMode(const QString& current_scheme, const QString& target_mode);
  [[nodiscard]] static Appearance loadAppearance();
  [[nodiscard]] static QString loadMode();
  [[nodiscard]] static bool hasMode();
  [[nodiscard]] static bool hasScheme();
  [[nodiscard]] static bool writeAppearance(const Appearance& appearance);
  [[nodiscard]] static bool writeMode(const QString& mode);
};
