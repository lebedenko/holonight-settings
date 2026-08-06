#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class ThemeSwatchTokens : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

 public:
  explicit ThemeSwatchTokens(QObject* parent = nullptr);

  [[nodiscard]] Q_INVOKABLE static QVariantMap getTokensForScheme(const QString& scheme_id);
};
