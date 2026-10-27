#include "BluetoothControllerQml.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>
using namespace HoloNight::System;
namespace {
class BluetoothFake : public BluetoothBackend {
 public:
  QList<BluetoothAdapter> adapters{{"off", "AA", "Off"}, {"on", "BB", "On", true}};
  QList<BluetoothDevice> devices{{.id = "keyboard", .adapterId = "on", .alias = "Keyboard"}};
  QList<BluetoothResult> calls;
  void start() override { publish(); }
  void stop() override {}
  void execute(const QString& id, BluetoothOperation op, const QString& target, const QVariant&) override {
    calls.append({id, target, op, {}, {}});
  }
  void respond(const QString& id, bool, const QString&) override { emit agentCanceled(id); }
  void requestDefaultAgent() override {}
  void publish() { emit snapshotChanged(adapters, devices, true); }
  void complete(bool success = true) {
    auto result = calls.last();
    if (!success) {
      result.errorName = "org.bluez.Error.Failed";
      result.errorMessage = "Test failure";
    }
    emit operationFinished(result);
  }
};
}  // namespace
TEST(BluetoothPresentation, SelectsPoweredAdapterAndRetainsExplicitChoice) {
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  // Synchronous injected snapshots are already present when the adapter is constructed.
  EXPECT_EQ(adapter.selectedId(), "on");
  adapter.selectAdapter("off");
  backend.publish();
  EXPECT_EQ(adapter.selectedId(), "off");
  backend.adapters.removeFirst();
  backend.publish();
  EXPECT_EQ(adapter.selectedId(), "on");
}
TEST(BluetoothPresentation, ExplicitPairingTrustsThenConnects) {
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  adapter.action("keyboard", "pair");
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 1; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::Pair);
  backend.complete();
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 2; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::Trust);
  backend.complete();
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 3; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::Connect);
}
TEST(BluetoothPresentation, TrustFailurePreservesPairingAndReportsPartialFailure) {
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  adapter.action("keyboard", "pair");
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 1; }));
  backend.devices.first().paired = true;
  backend.publish();
  backend.complete();
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 2; }));
  backend.complete(false);
  EXPECT_FALSE(adapter.errorMessage().isEmpty());
  EXPECT_TRUE(adapter.devices().first().toMap().value("paired").toBool());
  EXPECT_EQ(backend.calls.size(), 2);
}
TEST(BluetoothPresentation, ScanOwnershipFollowsSelectionAndVisibility) {
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  adapter.setActive(true);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 1; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::StartDiscovery);
  backend.complete();
  adapter.scan(false);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 2; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::StopDiscovery);
  backend.complete();
  backend.publish();
  EXPECT_FALSE(adapter.scanning());
  adapter.setActive(false);
  adapter.setActive(true);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 3; }));
  backend.complete();
  adapter.setActive(false);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 4; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::StopDiscovery);
}
TEST(BluetoothPresentation, DismissalCancelsPairingAndNeverTrustsIncomingAuthorization) {
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  adapter.setActive(true);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 1; }));
  backend.complete();
  adapter.scan(false);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 2; }));
  backend.complete();
  backend.calls.clear();
  emit backend.agentRequested({"incoming", "keyboard", BluetoothInteraction::PairingAuthorization, {}, {}, 0});
  adapter.respond(true);
  EXPECT_TRUE(backend.calls.isEmpty());
  adapter.action("keyboard", "pair");
  ASSERT_TRUE(QTest::qWaitFor([&] { return !backend.calls.isEmpty(); }));
  adapter.setActive(false);
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 2; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::CancelPairing);
  emit backend.operationFinished(backend.calls.first());
  EXPECT_EQ(backend.calls.size(), 2);
}

TEST(BluetoothSettingsUi, DeviceDetailsPendingControlsAndPairingDialog) {
  qmlRegisterUncreatableType<BluetoothControllerQml>("HolonightSettings", 1, 0, "BluetoothController", "Test-owned");
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  QQmlEngine engine;
  QQmlComponent component(&engine,
                          QUrl::fromLocalFile(QStringLiteral(TEST_SOURCE_DIR "/apps/settings/qml/BluetoothPage.qml")));
  ASSERT_TRUE(component.isReady()) << component.errorString().toStdString();
  std::unique_ptr<QObject> page(
      component.createWithInitialProperties({{"bluetoothController", QVariant::fromValue(&adapter)}}));
  ASSERT_NE(page, nullptr) << component.errorString().toStdString();
  QQuickWindow window;
  window.resize(960, 700);
  auto* item = qobject_cast<QQuickItem*>(page.get());
  ASSERT_NE(item, nullptr);
  item->setParentItem(window.contentItem());
  item->setSize(QSizeF(960, 700));
  window.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
  page->setProperty("selectedDeviceId", "keyboard");
  auto* action = page->findChild<QObject*>("bluetoothDeviceAction");
  ASSERT_NE(action, nullptr);
  EXPECT_EQ(action->property("text").toString(), "Pair");
  adapter.action("keyboard", "pair");
  ASSERT_TRUE(QTest::qWaitFor([&] { return !action->property("enabled").toBool(); }));
  emit backend.agentRequested({"prompt", "keyboard", BluetoothInteraction::Confirmation, "000042", {}, 0});
  auto* accept = page->findChild<QObject*>("bluetoothPairingAccept");
  ASSERT_NE(accept, nullptr);
  EXPECT_TRUE(accept->property("visible").toBool());
  adapter.cancelPairing();
  EXPECT_TRUE(adapter.interaction().isEmpty());
}

TEST(BluetoothPresentation, CancelingIncomingKeyboardDisplayCancelsBlueZPairing) {
  BluetoothFake backend;
  BluetoothControllerQml adapter(&backend, nullptr);
  emit backend.agentRequested({"display", "keyboard", BluetoothInteraction::PasskeyDisplay, "000042", {}, 0});
  adapter.cancelPairing();
  ASSERT_TRUE(QTest::qWaitFor([&] { return backend.calls.size() == 1; }));
  EXPECT_EQ(backend.calls.last().operation, BluetoothOperation::CancelPairing);
  EXPECT_TRUE(adapter.interaction().isEmpty());
}
