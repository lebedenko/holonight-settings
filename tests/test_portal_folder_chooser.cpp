// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 HoloNight contributors
#include "PortalFolderChooser.h"

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include <gtest/gtest.h>

// Test fixtures expose portal request state to assertions.
// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
namespace {
constexpr auto kService = "org.freedesktop.portal.Desktop";
constexpr auto kDesktopPath = "/org/freedesktop/portal/desktop";
class MockFolderRequest : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.Request")
 public:
  int closes = 0;
  Q_SLOT Q_SCRIPTABLE void Close() { ++closes; }
};
class MockFolderPortal : public QObject, protected QDBusContext {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.FileChooser")
 public:
  explicit MockFolderPortal(QDBusConnection connection) : connection_(std::move(connection)) {}
  int calls = 0;
  QString parent_window;
  QString title;
  QVariantMap options;
  QString request_path;
  QString early_uri;
  MockFolderRequest request;
  void respond(uint code, const QStringList& uris = {}) const {
    auto signal = QDBusMessage::createSignal(request_path, "org.freedesktop.portal.Request", "Response");
    signal << code << QVariantMap{{"uris", uris}};
    EXPECT_TRUE(connection_.send(signal));
  }
  Q_SLOT Q_SCRIPTABLE QDBusObjectPath OpenFile(const QString& parent, const QString& caption,
                                               const QVariantMap& settings) {
    ++calls;
    parent_window = parent;
    title = caption;
    options = settings;
    const auto sender = message().service().mid(1).replace('.', '_');
    request_path = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
                       .arg(sender, settings.value("handle_token").toString());
    EXPECT_TRUE(connection_.registerObject(request_path, &request, QDBusConnection::ExportScriptableSlots));
    if (!early_uri.isEmpty()) {
      respond(0, {early_uri});
    }
    return QDBusObjectPath(request_path);
  }

 private:
  QDBusConnection connection_;
};
class PortalFolderChooserTest : public testing::Test {
 protected:
  const QString connection_name = QUuid::createUuid().toString(QUuid::WithoutBraces);
  QDBusConnection server = QDBusConnection::connectToBus(QDBusConnection::SessionBus, connection_name);
  MockFolderPortal portal{server};
  void SetUp() override {
    if (!server.isConnected()) {
      GTEST_SKIP() << "A private session bus is required";
    }
    ASSERT_TRUE(server.registerService(kService));
    ASSERT_TRUE(server.registerObject(kDesktopPath, &portal, QDBusConnection::ExportScriptableSlots));
  }
  void TearDown() override {
    server.unregisterObject(kDesktopPath);
    if (!portal.request_path.isEmpty()) {
      server.unregisterObject(portal.request_path);
    }
    server.unregisterService(kService);
    QDBusConnection::disconnectFromBus(connection_name);
  }
};
}  // namespace
TEST_F(PortalFolderChooserTest, RequestsOneDirectoryAndEmitsTheSelectedLocalFolder) {
  QTemporaryDir folder;
  PortalFolderChooser chooser;
  QSignalSpy selected(&chooser, &PortalFolderChooser::folderSelected);
  chooser.open();
  chooser.open();
  QTRY_COMPARE_WITH_TIMEOUT(portal.calls, 1, 3000);
  EXPECT_TRUE(chooser.busy());
  EXPECT_TRUE(portal.options.value("directory").toBool());
  EXPECT_FALSE(portal.options.value("multiple").toBool());
  EXPECT_TRUE(portal.options.value("modal").toBool());
  EXPECT_FALSE(portal.options.value("handle_token").toString().isEmpty());
  EXPECT_EQ(portal.title, QStringLiteral("Add wallpaper folder"));
  portal.respond(0, {QUrl::fromLocalFile(folder.path()).toString()});
  QTRY_COMPARE_WITH_TIMEOUT(selected.count(), 1, 3000);
  EXPECT_EQ(selected.first().first().toUrl(), QUrl::fromLocalFile(folder.path()));
  EXPECT_FALSE(chooser.busy());
  EXPECT_TRUE(chooser.error().isEmpty());
}
TEST_F(PortalFolderChooserTest, HandlesSelectionBeforeTheOpenFileReply) {
  QTemporaryDir folder;
  portal.early_uri = QUrl::fromLocalFile(folder.path()).toString();
  PortalFolderChooser chooser;
  QSignalSpy selected(&chooser, &PortalFolderChooser::folderSelected);
  chooser.open();
  QTRY_COMPARE_WITH_TIMEOUT(selected.count(), 1, 3000);
  EXPECT_FALSE(chooser.busy());
}
TEST_F(PortalFolderChooserTest, UserCancellationDoesNotSelectAFolderOrShowAnError) {
  PortalFolderChooser chooser;
  QSignalSpy selected(&chooser, &PortalFolderChooser::folderSelected);
  chooser.open();
  QTRY_COMPARE_WITH_TIMEOUT(portal.calls, 1, 3000);
  portal.respond(1);
  QTRY_VERIFY_WITH_TIMEOUT(!chooser.busy(), 3000);
  EXPECT_EQ(selected.count(), 0);
  EXPECT_TRUE(chooser.error().isEmpty());
}
TEST_F(PortalFolderChooserTest, RejectsNonLocalFolderResponses) {
  PortalFolderChooser chooser;
  QSignalSpy selected(&chooser, &PortalFolderChooser::folderSelected);
  chooser.open();
  QTRY_COMPARE_WITH_TIMEOUT(portal.calls, 1, 3000);
  portal.respond(0, {QStringLiteral("https://example.com/folder")});
  QTRY_VERIFY_WITH_TIMEOUT(!chooser.busy(), 3000);
  EXPECT_EQ(selected.count(), 0);
  EXPECT_FALSE(chooser.error().isEmpty());
}
TEST_F(PortalFolderChooserTest, CancellingAnOutstandingRequestClosesItAndIgnoresSelection) {
  QTemporaryDir folder;
  PortalFolderChooser chooser;
  QSignalSpy selected(&chooser, &PortalFolderChooser::folderSelected);
  chooser.open();
  QTRY_COMPARE_WITH_TIMEOUT(portal.calls, 1, 3000);
  chooser.cancel();
  portal.respond(0, {QUrl::fromLocalFile(folder.path()).toString()});
  QTRY_VERIFY_WITH_TIMEOUT(portal.request.closes > 0, 3000);
  EXPECT_FALSE(chooser.busy());
  EXPECT_EQ(selected.count(), 0);
}
TEST(PortalFolderChooserDisconnectedTest, ReportsAnUnavailableBusWithoutOpeningADialog) {
  PortalFolderChooser chooser(QDBusConnection(QStringLiteral("disconnected-folder-chooser")));
  chooser.open();
  EXPECT_FALSE(chooser.busy());
  EXPECT_FALSE(chooser.error().isEmpty());
}
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
#include "test_portal_folder_chooser.moc"
