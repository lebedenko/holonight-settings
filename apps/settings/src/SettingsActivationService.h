#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QVariantMap>

#include <optional>

class QQuickWindow;

class SettingsActivationService : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Application")

 public:
  enum class StartupRole { Primary, Secondary, Error };

  explicit SettingsActivationService(QObject* parent = nullptr);
  explicit SettingsActivationService(const QDBusConnection& connection, QObject* parent = nullptr);
  ~SettingsActivationService() override;

  SettingsActivationService(const SettingsActivationService&) = delete;
  SettingsActivationService& operator=(const SettingsActivationService&) = delete;
  SettingsActivationService(SettingsActivationService&&) = delete;
  SettingsActivationService& operator=(SettingsActivationService&&) = delete;

  [[nodiscard]] StartupRole arbitrate(const QVariantMap& platform_data);
  void setWindow(QQuickWindow* window);

  [[nodiscard]] QString errorString() const;
  [[nodiscard]] static QVariantMap platformDataFromEnvironment();

 public slots:
  Q_SCRIPTABLE void Activate(const QVariantMap& platform_data);  // NOLINT(readability-identifier-naming)
  Q_SCRIPTABLE void Open(const QStringList& uris,                // NOLINT(readability-identifier-naming)
                         const QVariantMap& platform_data);
  Q_SCRIPTABLE void ActivateAction(const QString& action_name,  // NOLINT(readability-identifier-naming)
                                   const QVariantList& parameter, const QVariantMap& platform_data);

 private:
  void requestActivation(const QVariantMap& platform_data);
  [[nodiscard]] bool forwardActivation(const QVariantMap& platform_data);

  QDBusConnection connection_;
  QQuickWindow* window_ = nullptr;
  std::optional<QVariantMap> pending_activation_;
  QString error_string_;
  bool owns_service_ = false;
};
