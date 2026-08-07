#include "SettingsActivationService.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QFile>
#include <QQuickWindow>
#include <QSignalSpy>

#include <gtest/gtest.h>

namespace {

class ScopedEnvironmentValue {
 public:
  ScopedEnvironmentValue(const char* name, const QByteArray& value) : name_(name), previous_(qgetenv(name)) {
    qputenv(name_, value);
  }

  ~ScopedEnvironmentValue() {
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
};

}  // namespace

TEST(SettingsActivationServiceTest, ReadsStandardActivationDataFromEnvironment) {
  ScopedEnvironmentValue token("XDG_ACTIVATION_TOKEN", "test-token");
  ScopedEnvironmentValue startup_id("DESKTOP_STARTUP_ID", "test-startup-id");

  const QVariantMap data = SettingsActivationService::platformDataFromEnvironment();

  EXPECT_EQ(data.value(QStringLiteral("activation-token")).toString(), QStringLiteral("test-token"));
  EXPECT_EQ(data.value(QStringLiteral("desktop-startup-id")).toString(), QStringLiteral("test-startup-id"));
}

TEST(SettingsActivationServiceTest, ExportsTheFreedesktopApplicationMethodContract) {
  const QMetaObject& meta_object = SettingsActivationService::staticMetaObject;

  EXPECT_GE(meta_object.indexOfMethod("Activate(QVariantMap)"), 0);
  EXPECT_GE(meta_object.indexOfMethod("Open(QStringList,QVariantMap)"), 0);
  EXPECT_GE(meta_object.indexOfMethod("ActivateAction(QString,QVariantList,QVariantMap)"), 0);
  EXPECT_STREQ(meta_object.classInfo(meta_object.indexOfClassInfo("D-Bus Interface")).value(),
               "org.freedesktop.Application");
}

TEST(SettingsActivationServiceTest, QueuesActivationUntilTheWindowIsReady) {
  SettingsActivationService service;
  QQuickWindow window;
  window.hide();

  service.Activate({{QStringLiteral("activation-token"), QStringLiteral("queued-token")}});
  service.setWindow(&window);

  EXPECT_TRUE(window.isVisible());
  EXPECT_NE(window.visibility(), QWindow::Minimized);
}

TEST(SettingsActivationServiceTest, FailsClosedWhenTheBusIsDisconnected) {
  const QString connection_name =
      QStringLiteral("settings-activation-disconnected-%1").arg(QCoreApplication::applicationPid());
  QDBusConnection connection =
      QDBusConnection::connectToBus(QStringLiteral("unix:path=/does/not/exist"), connection_name);
  ASSERT_FALSE(connection.isConnected());

  SettingsActivationService service(connection);

  EXPECT_EQ(service.arbitrate({}), SettingsActivationService::StartupRole::Error);
  EXPECT_FALSE(service.errorString().isEmpty());
  QDBusConnection::disconnectFromBus(connection_name);
}

TEST(SettingsActivationServiceTest, DesktopAndServiceMetadataShareTheStableIdentity) {
  QFile desktop_file(QStringLiteral(TEST_SOURCE_DIR "/data/applications/org.holonight.Settings.desktop"));
  QFile service_file(QStringLiteral(TEST_SOURCE_DIR "/data/dbus-1/services/org.holonight.Settings.service.in"));
  ASSERT_TRUE(desktop_file.open(QIODevice::ReadOnly));
  ASSERT_TRUE(service_file.open(QIODevice::ReadOnly));

  const QByteArray desktop = desktop_file.readAll();
  const QByteArray service = service_file.readAll();
  EXPECT_TRUE(desktop.contains("DBusActivatable=true"));
  EXPECT_TRUE(desktop.contains("Exec=holonight-settings"));
  EXPECT_TRUE(service.contains("Name=org.holonight.Settings"));
  EXPECT_TRUE(service.contains("/holonight-settings"));
}

// GoogleTest assertion macros inflate this integration test's measured complexity.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(SettingsActivationServiceTest, ArbitratesOwnershipAndForwardsSecondaryActivation) {
  if (!QDBusConnection::sessionBus().isConnected()) {
    GTEST_SKIP() << "No session D-Bus is available";
  }

  const QString primary_name = QStringLiteral("settings-activation-primary-%1").arg(QCoreApplication::applicationPid());
  const QString secondary_name =
      QStringLiteral("settings-activation-secondary-%1").arg(QCoreApplication::applicationPid());
  QDBusConnection primary_connection = QDBusConnection::connectToBus(QDBusConnection::SessionBus, primary_name);
  QDBusConnection secondary_connection = QDBusConnection::connectToBus(QDBusConnection::SessionBus, secondary_name);
  ASSERT_TRUE(primary_connection.isConnected());
  ASSERT_TRUE(secondary_connection.isConnected());

  {
    SettingsActivationService primary(primary_connection);
    QQuickWindow window;
    primary.setWindow(&window);
    EXPECT_EQ(primary.arbitrate({}), SettingsActivationService::StartupRole::Primary);

    SettingsActivationService secondary(secondary_connection);
    EXPECT_EQ(secondary.arbitrate({{QStringLiteral("activation-token"), QStringLiteral("forwarded-token")}}),
              SettingsActivationService::StartupRole::Secondary);

    QDBusInterface introspection(QStringLiteral("org.holonight.Settings"), QStringLiteral("/org/holonight/Settings"),
                                 QStringLiteral("org.freedesktop.DBus.Introspectable"), primary_connection);
    const QDBusMessage reply = introspection.call(QDBus::BlockWithGui, QStringLiteral("Introspect"));
    ASSERT_EQ(reply.type(), QDBusMessage::ReplyMessage);
    const QString xml = reply.arguments().constFirst().toString();
    EXPECT_TRUE(xml.contains(QStringLiteral("org.freedesktop.Application")));
    EXPECT_TRUE(xml.contains(QStringLiteral("method name=\"Activate\"")));
  }

  QDBusConnection::disconnectFromBus(primary_name);
  QDBusConnection::disconnectFromBus(secondary_name);
}
