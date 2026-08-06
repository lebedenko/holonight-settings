#include "theme_swatch_tokens.h"

#include <holonight/palette.h>
#include <holonight/theme_catalog.h>

ThemeSwatchTokens::ThemeSwatchTokens(QObject* parent) : QObject(parent) {}

QVariantMap ThemeSwatchTokens::getTokensForScheme(const QString& scheme_id) {
  const Holonight::ThemeSchemeKind kind = Holonight::schemeKindForSchemeId(scheme_id);
  const Holonight::ColorTokens tokens = Holonight::tokensForScheme(kind);
  return QVariantMap{
      {QStringLiteral("surface"), tokens.surface},
      {QStringLiteral("surfaceHover"), tokens.surfaceHover},
      {QStringLiteral("surfacePressed"), tokens.surfaceElevated},
      {QStringLiteral("borderPassive"), tokens.borderPassive},
      {QStringLiteral("primary"), tokens.primary},
      {QStringLiteral("onPrimary"), tokens.onPrimary},
      {QStringLiteral("selectionIndicator"), tokens.selectionIndicator},
      {QStringLiteral("disabledOverlay"), tokens.disabledOverlay},
      {QStringLiteral("accent"), tokens.accentBlue},
      {QStringLiteral("secondaryAccent"), tokens.accentViolet},
  };
}
