#include "BluetoothControllerQml.h"

#include <algorithm>
using namespace HoloNight::System;
namespace {
QString label(const BluetoothDevice& device) {
  if (!device.alias.isEmpty()) {
    return device.alias;
  }
  return !device.name.isEmpty() ? device.name : device.address;
}
int deviceSection(const BluetoothDevice& device) {
  if (device.connected) {
    return 0;
  }
  return device.paired ? 1 : 2;
}
QVariantMap adapterMap(const BluetoothAdapter& adapter) {
  return {
      {"id", adapter.id},
      {"alias", adapter.alias},
      {"address", adapter.address},
      {"powered", adapter.powered},
      {"discoverable", adapter.discoverable},
      {"discovering", adapter.discovering},
  };
}
QString interactionName(BluetoothInteraction kind) {
  switch (kind) {
    case BluetoothInteraction::PinEntry:
      return QStringLiteral("pin-entry");
    case BluetoothInteraction::PinDisplay:
      return QStringLiteral("pin-display");
    case BluetoothInteraction::PasskeyEntry:
      return QStringLiteral("passkey-entry");
    case BluetoothInteraction::PasskeyDisplay:
      return QStringLiteral("passkey-display");
    case BluetoothInteraction::Confirmation:
      return QStringLiteral("confirmation");
    case BluetoothInteraction::PairingAuthorization:
      return QStringLiteral("pairing-authorization");
    case BluetoothInteraction::ServiceAuthorization:
      return QStringLiteral("service-authorization");
  }
  return {};
}
QString serviceName(const QString& uuid) {
  const auto shortId = uuid.toLower().left(8);
  if (shortId == "0000110b") {
    return QObject::tr("Audio sink");
  }
  if (shortId == "0000110a") {
    return QObject::tr("Audio source");
  }
  if (shortId == "0000110e") {
    return QObject::tr("Media remote control");
  }
  if (shortId == "0000111e") {
    return QObject::tr("Hands-free audio");
  }
  if (shortId == "00001124" || shortId == "00001812") {
    return QObject::tr("Keyboard / pointing device");
  }
  if (shortId == "0000180f") {
    return QObject::tr("Battery service");
  }
  if (shortId == "0000180a") {
    return QObject::tr("Device information");
  }
  return QObject::tr("Other service");
}
}  // namespace
BluetoothControllerQml::BluetoothControllerQml(QObject* parent) : BluetoothControllerQml(nullptr, parent) {}
BluetoothControllerQml::BluetoothControllerQml(BluetoothBackend* backend, QObject* parent)
    : QObject(parent),
      controller_((backend != nullptr) ? std::make_unique<BluetoothController>(backend)
                                       : std::make_unique<BluetoothController>()) {
  release_.setSingleShot(true);
  release_.setInterval(0);
  connect(&release_, &QTimer::timeout, this, [this] {
    scan_requested_ = false;
    reconcileDiscovery();
    emit changed();
  });
  connect(controller_->adapters(), &QAbstractItemModel::modelReset, this, &BluetoothControllerQml::update);
  connect(controller_->adapters(), &QAbstractItemModel::dataChanged, this, &BluetoothControllerQml::update);
  connect(controller_->devices(), &QAbstractItemModel::modelReset, this, &BluetoothControllerQml::update);
  connect(controller_->devices(), &QAbstractItemModel::dataChanged, this, &BluetoothControllerQml::update);
  connect(controller_.get(), &BluetoothController::availableChanged, this, &BluetoothControllerQml::update);
  connect(controller_.get(), &BluetoothController::operationStateChanged, this, &BluetoothControllerQml::changed);
  connect(controller_.get(), &BluetoothController::operationFinished, this, &BluetoothControllerQml::completed);
  connect(controller_.get(), &BluetoothController::diagnostic, this,
          [this](const QString& name, const QString& message) {
            error_ = tr("Bluetooth agent: %1 (%2)").arg(message, name);
            emit changed();
          });
  connect(controller_.get(), &BluetoothController::agentRequested, this, [this](const BluetoothAgentRequest& request) {
    interaction_ = {
        {"id", request.requestId},
        {"deviceId", request.deviceId},
        {"kind", interactionName(request.interaction)},
        {"code", request.code},
        {"service", serviceName(request.serviceUuid)},
        {"entered", request.entered},
    };
    emit changed();
    emit pairingRequested();
  });
  connect(controller_.get(), &BluetoothController::agentCanceled, this, [this](const QString& identifier) {
    if (interaction_.value("id").toString() == identifier) {
      interaction_.clear();
      emit changed();
    }
  });
  update();
}
BluetoothControllerQml::~BluetoothControllerQml() {
  disconnect(controller_.get(), nullptr, this, nullptr);
  disconnect(controller_->adapters(), nullptr, this, nullptr);
  disconnect(controller_->devices(), nullptr, this, nullptr);
}
QVariantList BluetoothControllerQml::adapters() const {
  QVariantList out;
  for (const auto& adapter : controller_->adapters()->items()) {
    out.append(adapterMap(adapter));
  }
  return out;
}
QVariantMap BluetoothControllerQml::selectedAdapter() const {
  const auto adapter = controller_->adapters()->find(selected_);
  auto result = adapter ? adapterMap(*adapter) : QVariantMap{};
  result.insert("busy", controller_->busy(selected_));
  return result;
}
bool BluetoothControllerQml::deviceLess(const BluetoothDevice& adapter, const BluetoothDevice& other) const {
  const int left_section = deviceSection(adapter);
  const int right_section = deviceSection(other);
  if (left_section != right_section) {
    return left_section < right_section;
  }
  if (active_) {
    const auto left_key = QString::number(left_section) + adapter.id;
    const auto right_key = QString::number(right_section) + other.id;
    const auto left_index = display_order_.indexOf(left_key);
    const auto right_index = display_order_.indexOf(right_key);
    if (left_index >= 0 || right_index >= 0) {
      if (left_index < 0) {
        return false;
      }
      if (right_index < 0) {
        return true;
      }
      return left_index < right_index;
    }
  }
  if (left_section == 2 && adapter.rssi != other.rssi) {
    return adapter.rssi.value_or(-32768) > other.rssi.value_or(-32768);
  }
  const int comparison = QString::localeAwareCompare(label(adapter), label(other));
  return comparison != 0 ? comparison < 0 : adapter.id < other.id;
}

QVariantList BluetoothControllerQml::devices() const {
  auto items = controller_->devices()->items();
  std::ranges::stable_sort(items, [this](const auto& left, const auto& right) { return deviceLess(left, right); });
  QVariantList out;
  display_order_.clear();
  for (const auto& device : items) {
    if (device.adapterId != selected_) {
      continue;
    }
    display_order_.append(QString::number(deviceSection(device)) + device.id);
    QStringList services;
    for (const auto& identifier : device.uuids) {
      const auto name = serviceName(identifier);
      if (!services.contains(name)) {
        services.append(name);
      }
    }
    QString section = tr("Available");
    if (device.connected) {
      section = tr("Connected");
    } else if (device.paired) {
      section = tr("Paired");
    }
    out.append(QVariantMap{
        {"id", device.id},
        {"name", label(device)},
        {"originalName", device.name},
        {"alias", device.alias},
        {"address", device.address},
        {"addressType", device.addressType},
        {"paired", device.paired},
        {"bonded", device.bonded},
        {"connected", device.connected},
        {"trusted", device.trusted},
        {"blocked", device.blocked},
        {"icon", device.icon.isEmpty() ? QStringLiteral("bluetooth") : device.icon},
        {"battery", device.battery ? QVariant(*device.battery) : QVariant{}},
        {"rssi", device.rssi ? QVariant(*device.rssi) : QVariant{}},
        {"uuids", device.uuids},
        {"services", services},
        {"deviceClass", device.deviceClass},
        {"appearance", device.appearance},
        {"busy", controller_->busy(device.id)},
        {
            "section",
            section,
        },
    });
  }
  return out;
}
int BluetoothControllerQml::connectedCount() const {
  int count = 0;
  for (const auto& device : controller_->devices()->items()) {
    if (device.connected) {
      ++count;
    }
  }
  return count;
}
QString BluetoothControllerQml::tooltip() const {
  QStringList lines;
  for (const auto& device : controller_->devices()->items()) {
    if (device.connected) {
      lines.append(device.battery ? tr("%1 — %2%").arg(label(device)).arg(*device.battery) : label(device));
    }
  }
  if (lines.isEmpty()) {
    return selectedAdapter().value("powered").toBool() ? tr("Bluetooth enabled") : tr("Bluetooth off");
  }
  return lines.join(QLatin1Char('\n'));
}
void BluetoothControllerQml::update() {
  if (!explicit_selection_ || !controller_->adapters()->find(selected_)) {
    explicit_selection_ = false;
    const auto items = controller_->adapters()->items();
    selected_.clear();
    for (const auto& adapter : items) {
      if (adapter.powered) {
        selected_ = adapter.id;
        break;
      }
    }
    if (selected_.isEmpty() && !items.isEmpty()) {
      selected_ = items.first().id;
    }
  }
  for (auto it = owned_.begin(); it != owned_.end();) {
    const auto adapter = controller_->adapters()->find(*it);
    if (!adapter || !adapter->powered) {
      it = owned_.erase(it);
    } else {
      ++it;
    }
  }
  if (!pairing_.isEmpty() && !controller_->devices()->find(pairing_)) {
    cancelPairing();
  }
  reconcileDiscovery();
  emit changed();
}
void BluetoothControllerQml::selectAdapter(const QString& identifier) {
  if (!controller_->adapters()->find(identifier)) {
    return;
  }
  explicit_selection_ = true;
  if (selected_ == identifier) {
    return;
  }
  cancelPairing();
  selected_ = identifier;
  explicit_selection_ = true;
  reconcileDiscovery();
  emit changed();
}
void BluetoothControllerQml::setActive(bool active) {
  if (active_ == active) {
    return;
  }
  active_ = active;
  if (active) {
    release_.stop();
    scan_requested_ = true;
    reconcileDiscovery();
  } else {
    cancelPairing();
    release_.start();
  }
}
void BluetoothControllerQml::scan(bool enabled) {
  error_.clear();
  release_.stop();
  scan_requested_ = enabled && active_;
  reconcileDiscovery();
  emit changed();
}
void BluetoothControllerQml::reconcileDiscovery() {
  if (!discovery_request_.isEmpty()) {
    return;
  }
  QString desired;
  const auto adapter = controller_->adapters()->find(selected_);
  if (scan_requested_ && adapter && adapter->powered) {
    desired = selected_;
  }
  for (const auto& identifier : owned_) {
    if (identifier != desired && !controller_->busy(identifier)) {
      discovery_target_ = identifier;
      discovery_request_ = controller_->execute(BluetoothOperation::StopDiscovery, identifier);
      return;
    }
  }
  if (!desired.isEmpty() && !owned_.contains(desired) && !controller_->busy(desired)) {
    discovery_target_ = desired;
    discovery_request_ = controller_->execute(BluetoothOperation::StartDiscovery, desired);
  }
}
void BluetoothControllerQml::power(bool enabled) {
  error_.clear();
  controller_->execute(BluetoothOperation::Power, selected_, enabled);
}
void BluetoothControllerQml::discoverable(bool enabled) {
  error_.clear();
  controller_->execute(BluetoothOperation::Discoverable, selected_, enabled);
}
void BluetoothControllerQml::renameAdapter(const QString& alias) {
  error_.clear();
  controller_->execute(BluetoothOperation::AdapterAlias, selected_, alias);
}
void BluetoothControllerQml::action(const QString& identifier, const QString& action, const QVariant& value) {
  error_.clear();
  BluetoothOperation operation = BluetoothOperation::Pair;
  if (action == "pair") {
    cancelPairing();
    pairing_ = identifier;
    operation = BluetoothOperation::Pair;
  } else if (action == "connect") {
    operation = BluetoothOperation::Connect;
  } else if (action == "disconnect") {
    operation = BluetoothOperation::Disconnect;
  } else if (action == "rename") {
    operation = BluetoothOperation::DeviceAlias;
  } else if (action == "trust") {
    operation = BluetoothOperation::Trust;
  } else if (action == "block") {
    operation = BluetoothOperation::Block;
  } else if (action == "forget") {
    operation = BluetoothOperation::Forget;
  } else {
    return;
  }
  const auto request = controller_->execute(operation, identifier, value);
  if (operation == BluetoothOperation::Pair) {
    chain_request_ = request;
    chain_operation_ = operation;
  }
}
void BluetoothControllerQml::recordFailure(const BluetoothResult& result) {
  QString advice = tr("Try the operation again.");
  if (result.errorName.endsWith("NotReady")) {
    advice = tr("Turn Bluetooth on and try again.");
  } else if (result.errorName.contains("Authentication")) {
    advice = tr("Put the device in pairing mode and check the pairing code.");
  } else if (result.errorName.endsWith("Busy") || result.errorName.endsWith("InProgress")) {
    advice = tr("Wait for the current operation to finish.");
  } else if (result.errorName.endsWith("NotAuthorized")) {
    advice = tr("Bluetooth permission was denied.");
  }
  error_ = tr("Bluetooth operation failed. %1\n%2 (%3)").arg(advice, result.errorMessage, result.errorName);
  if (result.requestId == chain_request_ && chain_operation_ != BluetoothOperation::Pair) {
    error_ =
        (chain_operation_ == BluetoothOperation::Trust ? tr("Device paired, but trusting failed. ")
                                                       : tr("Device paired and trusted, but connecting failed. ")) +
        error_;
  }
}

void BluetoothControllerQml::completed(const BluetoothResult& result) {
  if (!result.succeeded()) {
    recordFailure(result);
  }
  if (result.requestId == chain_request_) {
    chain_request_.clear();
    if (result.succeeded() && chain_operation_ != BluetoothOperation::Connect) {
      chain_operation_ =
          chain_operation_ == BluetoothOperation::Pair ? BluetoothOperation::Trust : BluetoothOperation::Connect;
      chain_request_ = controller_->execute(
          chain_operation_, pairing_, chain_operation_ == BluetoothOperation::Trust ? QVariant(true) : QVariant{});
    } else {
      {
        pairing_.clear();
      }
    }
  }
  if (result.requestId == discovery_request_) {
    discovery_request_.clear();
    if (result.succeeded()) {
      if (result.operation == BluetoothOperation::StartDiscovery) {
        owned_.insert(discovery_target_);
      } else {
        owned_.remove(discovery_target_);
      }
    } else {
      scan_requested_ = false;
      owned_.remove(discovery_target_);
    }
  }
  reconcileDiscovery();
  emit changed();
}
void BluetoothControllerQml::respond(bool accept, const QString& input) {
  controller_->respond(interaction_.value("id").toString(), accept, input);
}
void BluetoothControllerQml::cancelPairing() {
  QString target = pairing_;
  const auto kind = interaction_.value("kind").toString();
  if (target.isEmpty() && (kind == "pin-display" || kind == "passkey-display")) {
    target = interaction_.value("deviceId").toString();
  }
  if (!interaction_.isEmpty()) {
    respond(false);
  }
  if (!target.isEmpty()) {
    controller_->execute(BluetoothOperation::CancelPairing, target);
    pairing_.clear();
  }
  chain_request_.clear();
  interaction_.clear();
  emit changed();
}
