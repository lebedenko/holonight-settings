#include "SettingsActivationService.h"

#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QTimer>

#include <chrono>

namespace {

constexpr auto kServiceName = "org.holonight.Settings";
constexpr auto kObjectPath = "/org/holonight/Settings";
constexpr auto kApplicationInterface = "org.freedesktop.Application";
constexpr auto kActivationToken = "activation-token";
constexpr auto kDesktopStartupId = "desktop-startup-id";
constexpr auto kActivationTokenEnvironment = "XDG_ACTIVATION_TOKEN";
constexpr auto kDesktopStartupIdEnvironment = "DESKTOP_STARTUP_ID";
constexpr auto kActivationGracePeriod = std::chrono::milliseconds(500);
constexpr auto kForwardingTimeout = std::chrono::seconds(3);

class ScopedEnvironmentValue {
 public:
  ScopedEnvironmentValue(const char* name, const QVariantMap& data, const char* key)
      : name_(name), previous_(qgetenv(name)) {
    const QByteArray value = data.value(QLatin1String(key)).toString().toUtf8();
    if (!value.isEmpty()) {
      qputenv(name_, value);
      changed_ = true;
    }
  }

  ~ScopedEnvironmentValue() {
    if (!changed_) {
      return;
    }
    if (previous_.isNull()) {
      qunsetenv(name_);
    } else {
      qputenv(name_, previous_);
    }
  }

  ScopedEnvironmentValue(const ScopedEnvironmentValue&) = delete;
  ScopedEnvironmentValue& operator=(const ScopedEnvironmentValue&) = delete;
  ScopedEnvironmentValue(ScopedEnvironmentValue&&) = delete;
  ScopedEnvironmentValue& operator=(ScopedEnvironmentValue&&) = delete;

 private:
  const char* name_;
  QByteArray previous_;
  bool changed_ = false;
};

}  // namespace

SettingsActivationService::SettingsActivationService(QObject* parent)
    : SettingsActivationService(QDBusConnection::sessionBus(), parent) {}

SettingsActivationService::SettingsActivationService(QDBusConnection connection, QObject* parent)
    : QObject(parent), connection_(std::move(connection)) {}

SettingsActivationService::~SettingsActivationService() {
  if (owns_service_) {
    connection_.unregisterService(QLatin1String(kServiceName));
  }
  connection_.unregisterObject(QLatin1String(kObjectPath));
}

SettingsActivationService::StartupRole SettingsActivationService::arbitrate(const QVariantMap& platform_data) {
  if (!connection_.isConnected()) {
    error_string_ = tr("The session D-Bus is unavailable: %1").arg(connection_.lastError().message());
    return StartupRole::Error;
  }

  auto* interface = connection_.interface();
  if (interface == nullptr) {
    error_string_ = tr("The session D-Bus does not expose a connection interface.");
    return StartupRole::Error;
  }

  if (!connection_.registerObject(QLatin1String(kObjectPath), this, QDBusConnection::ExportScriptableSlots)) {
    error_string_ = tr("Could not export the settings activation object: %1").arg(connection_.lastError().message());
    return StartupRole::Error;
  }

  const QDBusReply<QDBusConnectionInterface::RegisterServiceReply> registration =
      interface->registerService(QLatin1String(kServiceName), QDBusConnectionInterface::DontQueueService,
                                 QDBusConnectionInterface::DontAllowReplacement);
  if (registration.isValid() && registration.value() == QDBusConnectionInterface::ServiceRegistered) {
    owns_service_ = true;
    requestActivation(platform_data);
    return StartupRole::Primary;
  }

  connection_.unregisterObject(QLatin1String(kObjectPath));
  if (forwardActivation(platform_data)) {
    return StartupRole::Secondary;
  }

  const QString registration_error =
      registration.isValid() ? tr("the service name is unavailable") : registration.error().message();
  error_string_ = tr("Could not establish settings uniqueness (%1), and activation forwarding failed: %2")
                      .arg(registration_error, error_string_);
  return StartupRole::Error;
}

void SettingsActivationService::setWindow(QQuickWindow* window) {
  window_ = window;
  if (window_ != nullptr && pending_activation_.has_value()) {
    const QVariantMap platform_data = *pending_activation_;
    pending_activation_.reset();
    requestActivation(platform_data);
  }
}

QString SettingsActivationService::errorString() const { return error_string_; }

QVariantMap SettingsActivationService::platformDataFromEnvironment() {
  QVariantMap data;
  const QByteArray activation_token = qgetenv(kActivationTokenEnvironment);
  const QByteArray startup_id = qgetenv(kDesktopStartupIdEnvironment);
  if (!activation_token.isEmpty()) {
    data.insert(QLatin1String(kActivationToken), QString::fromUtf8(activation_token));
  }
  if (!startup_id.isEmpty()) {
    data.insert(QLatin1String(kDesktopStartupId), QString::fromUtf8(startup_id));
  }
  return data;
}

void SettingsActivationService::Activate(const QVariantMap& platform_data) { requestActivation(platform_data); }

void SettingsActivationService::Open(const QStringList& /*uris*/, const QVariantMap& platform_data) {
  requestActivation(platform_data);
}

void SettingsActivationService::ActivateAction(const QString& /*action_name*/, const QVariantList& /*parameter*/,
                                               const QVariantMap& platform_data) {
  requestActivation(platform_data);
}

void SettingsActivationService::requestActivation(const QVariantMap& platform_data) {
  if (window_ == nullptr) {
    pending_activation_ = platform_data;
    return;
  }

  ScopedEnvironmentValue activation_token(kActivationTokenEnvironment, platform_data, kActivationToken);
  ScopedEnvironmentValue startup_id(kDesktopStartupIdEnvironment, platform_data, kDesktopStartupId);
  if (window_->visibility() == QWindow::Minimized || !window_->isVisible()) {
    window_->showNormal();
  }
  window_->raise();
  window_->requestActivate();

  QTimer::singleShot(kActivationGracePeriod, window_, [window = window_]() {
    if (window->isVisible() && !window->isActive()) {
      window->alert(0);
    }
  });
}

bool SettingsActivationService::forwardActivation(const QVariantMap& platform_data) {
  QDBusInterface application(QLatin1String(kServiceName), QLatin1String(kObjectPath),
                             QLatin1String(kApplicationInterface), connection_);
  if (!application.isValid()) {
    error_string_ = application.lastError().message();
    return false;
  }

  QDBusPendingCallWatcher watcher(application.asyncCall(QStringLiteral("Activate"), platform_data));
  QEventLoop wait_loop;
  QTimer timeout;
  timeout.setSingleShot(true);
  connect(&watcher, &QDBusPendingCallWatcher::finished, &wait_loop, &QEventLoop::quit);
  connect(&timeout, &QTimer::timeout, &wait_loop, &QEventLoop::quit);
  timeout.start(kForwardingTimeout);
  wait_loop.exec();

  if (!watcher.isFinished()) {
    error_string_ = tr("Timed out while forwarding activation.");
    return false;
  }
  const QDBusPendingReply<> reply = watcher.reply();
  if (reply.isError()) {
    error_string_ = reply.error().message();
    return false;
  }
  return true;
}
