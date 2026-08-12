#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QVariantMap>

#include <cstdint>
#include <optional>

class QQuickWindow;

class SettingsActivationService : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Application")

 public:
  enum class StartupRole : std::uint8_t { Primary, Secondary, Error };

  explicit SettingsActivationService(QObject* parent = nullptr);
  explicit SettingsActivationService(QDBusConnection connection, QObject* parent = nullptr);
  ~SettingsActivationService() override;

  SettingsActivationService(const SettingsActivationService&) = delete;
  SettingsActivationService& operator=(const SettingsActivationService&) = delete;
  SettingsActivationService(SettingsActivationService&&) = delete;
  SettingsActivationService& operator=(SettingsActivationService&&) = delete;

  [[nodiscard]] StartupRole arbitrate(const QVariantMap& platform_data);
  void setWindow(QQuickWindow* window);

  [[nodiscard]] QString errorString() const;
  [[nodiscard]] static QVariantMap platformDataFromEnvironment();

  Q_SLOT Q_SCRIPTABLE void Activate(const QVariantMap& platform_data);  // NOLINT(readability-identifier-naming)
  Q_SLOT Q_SCRIPTABLE void Open(const QStringList& uris,                // NOLINT(readability-identifier-naming)
                                const QVariantMap& platform_data);
  Q_SLOT Q_SCRIPTABLE void ActivateAction(const QString& action_name,  // NOLINT(readability-identifier-naming)
                                          const QVariantList& parameter, const QVariantMap& platform_data);

 Q_SIGNALS:
  void pageRequested(const QString& page_key);

 private:
  struct ActivationRequest {
    QVariantMap platform_data;
    std::optional<QString> page_key;
  };

  void requestActivation(ActivationRequest request);
  [[nodiscard]] bool forwardActivation(const QVariantMap& platform_data);

  QDBusConnection connection_;
  QQuickWindow* window_ = nullptr;
  std::optional<ActivationRequest> pending_activation_;
  QString error_string_;
  bool owns_service_ = false;
};
