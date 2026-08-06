#include "ThemeConfigFile.h"

#include <QDir>
#include <QLoggingCategory>
#include <QSettings>

#include <holonight/config.h>
#include <holonight/theme_catalog.h>

Q_LOGGING_CATEGORY(lcThemeConfigFile, "holonight.settings.themeconfig")

QString ThemeConfigFile::path() {
  QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
  if (xdg.isEmpty()) {
    xdg = QDir::homePath() + QLatin1String("/.config");
  }
  return xdg + QLatin1String("/holonight/theme.conf");
}

QString ThemeConfigFile::defaultMode() { return QStringLiteral("dark"); }

QString ThemeConfigFile::defaultScheme() { return Holonight::defaultSchemeId(); }

QString ThemeConfigFile::defaultAccent() { return Holonight::defaultAccentId(); }

QString ThemeConfigFile::normalizeMode(const QString& mode) {
  QString normalized = mode.trimmed().toLower();
  if (normalized == QLatin1String("dark") || normalized == QLatin1String("light") ||
      normalized == QLatin1String("system")) {
    return normalized;
  }
  return defaultMode();
}

QString ThemeConfigFile::normalizeScheme(const QString& scheme) { return Holonight::normalizeSchemeId(scheme); }

QString ThemeConfigFile::normalizeAccent(const QString& accent) { return Holonight::normalizeAccentId(accent); }

QString ThemeConfigFile::modeForScheme(const QString& scheme) { return Holonight::modeNameForScheme(scheme); }

QString ThemeConfigFile::schemeForMode(const QString& mode) {
  return normalizeMode(mode) == QLatin1String("light") ? QStringLiteral("holonight-light") : defaultScheme();
}

QString ThemeConfigFile::siblingSchemeForMode(const QString& current_scheme, const QString& target_mode) {
  const QString normalized_mode = normalizeMode(target_mode);
  const Holonight::ThemeVariantCatalogEntry* current_variant =
      Holonight::themeVariantForSchemeId(normalizeScheme(current_scheme));
  if (current_variant == nullptr) {
    return defaultScheme();
  }

  const bool want_light = normalized_mode == QLatin1String("light");
  for (const Holonight::ThemeFamilyCatalogEntry& family : Holonight::themeFamilies()) {
    if (family.id != current_variant->family_id) {
      continue;
    }
    for (const QString& variant_id : family.variant_ids) {
      const Holonight::ThemeVariantCatalogEntry* candidate = Holonight::themeVariantForSchemeId(variant_id);
      if (candidate != nullptr && (candidate->mode == Holonight::ColorMode::Light) == want_light) {
        return candidate->id;
      }
    }
  }
  return defaultScheme();
}

bool ThemeConfigFile::hasMode() {
  QSettings settings{path(), QSettings::IniFormat};
  return settings.contains(QStringLiteral("appearance/mode"));
}

bool ThemeConfigFile::hasScheme() {
  QSettings settings{path(), QSettings::IniFormat};
  return !normalizeScheme(settings.value(QStringLiteral("appearance/scheme")).toString()).isEmpty();
}

ThemeConfigFile::Appearance ThemeConfigFile::loadAppearance() {
  QSettings settings{path(), QSettings::IniFormat};
  QString scheme = normalizeScheme(settings.value(QStringLiteral("appearance/scheme")).toString());
  if (scheme.isEmpty()) {
    scheme = schemeForMode(settings.value(QStringLiteral("appearance/mode"), defaultMode()).toString());
  }

  Appearance appearance;
  appearance.scheme = scheme;
  appearance.accent = normalizeAccent(settings.value(QStringLiteral("appearance/accent"), defaultAccent()).toString());
  appearance.mode = modeForScheme(scheme);
  return appearance;
}

QString ThemeConfigFile::loadMode() { return loadAppearance().mode; }

bool ThemeConfigFile::writeAppearance(const Appearance& appearance) {
  Holonight::ThemeConfig config = Holonight::ThemeConfig::loadFile();
  const QString scheme =
      normalizeScheme(appearance.scheme).isEmpty() ? defaultScheme() : normalizeScheme(appearance.scheme);
  config.scheme = scheme;
  config.accent = normalizeAccent(appearance.accent);
  config.appearance_mode = modeForScheme(scheme) == QLatin1String("light") ? Holonight::AppearanceMode::Light
                                                                           : Holonight::AppearanceMode::Dark;
  QString error;
  const bool saved = config.save(&error);
  if (!saved) {
    qCWarning(lcThemeConfigFile) << error;
  }
  return saved;
}

bool ThemeConfigFile::writeMode(const QString& mode) {
  const QString scheme = schemeForMode(mode);
  return writeAppearance(Appearance{.scheme = scheme, .accent = defaultAccent(), .mode = modeForScheme(scheme)});
}
