#include "BleMidiSource.h"

#include <QCoreApplication>

#ifdef MIDIPIANO_HAS_BLUETOOTH
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>
#include <QLowEnergyController>
#include <QLowEnergyService>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif
#endif

namespace midipiano::app {

#ifdef MIDIPIANO_HAS_BLUETOOTH
namespace {
// Vaste UUID's uit de "MIDI over Bluetooth Low Energy"-specificatie (Apple/MMA). Elk BLE-MIDI-toestel
// gebruikt deze, dus ook de PSoC 6 moet ze adverteren.
const QBluetoothUuid kMidiService(QStringLiteral("03B80E5A-EDE8-4B33-A751-6CE34EC4C700"));
const QBluetoothUuid kMidiCharacteristic(QStringLiteral("7772E5DB-3868-4112-A1A9-F2669D106BF3"));
}  // namespace
#endif

BleMidiSource::BleMidiSource(QObject* parent) : QObject(parent) {
  setStatus(available() ? tr("Not connected") : tr("This build has no Bluetooth (Qt Bluetooth module missing)"));
}

BleMidiSource::~BleMidiSource() {
  onMessage_ = nullptr;  // tijdens het afsluiten niets meer naar de synth sturen
  stop();
}

bool BleMidiSource::available() const {
#ifdef MIDIPIANO_HAS_BLUETOOTH
  return true;
#else
  return false;
#endif
}

void BleMidiSource::start(Callback onMessage) { onMessage_ = std::move(onMessage); }

void BleMidiSource::stop() {
  stopScan();
  disconnectDevice();
}

void BleMidiSource::setStatus(const QString& text) {
  if (text == statusText_) return;
  statusText_ = text;
  emit statusTextChanged();
}

void BleMidiSource::setScanning(bool scanning) {
  if (scanning == scanning_) return;
  scanning_ = scanning;
  emit scanningChanged();
}

void BleMidiSource::setConnected(bool connected, const QString& name) {
  connected_ = connected;
  connectedName_ = connected ? name : QString();
  emit connectionChanged();
}

// Vereiste 59: 59.project that communicates (e.g. UART, BT) with hardware
// De app praat via Bluetooth Low Energy met externe hardware: de PSoC 6 (of elk ander BLE-MIDI-toestel)
// stuurt MIDI-pakketten als notificaties op de MIDI-characteristic. Hier komen die bytes binnen; de
// BleMidiParser (core, getest zonder hardware) zet ze om in MidiMessages, en die gaan via dezelfde
// callback als de demo-sequencer naar de synth. Zo speelt de hardware rechtstreeks op de gsm.
void BleMidiSource::handlePacket(const QByteArray& packet) {
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(packet.constData());
  for (const midi::MidiMessage& message : parser_.parse(bytes, static_cast<std::size_t>(packet.size()))) {
    ++messageCount_;
    if (onMessage_) onMessage_(message);
  }
  emit messageCountChanged();
}

#ifdef MIDIPIANO_HAS_BLUETOOTH

void BleMidiSource::startScan() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  // Android 12+ (en macOS) vragen de gebruiker toestemming voor Bluetooth. Qt regelt de vraag; als
  // het antwoord binnenkomt, proberen we opnieuw.
  QBluetoothPermission permission;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
  permission.setCommunicationModes(QBluetoothPermission::Access);  // alleen verbinden, niet adverteren
#endif
  switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Undetermined:
      qApp->requestPermission(permission, this, [this](const QPermission& result) {
        if (result.status() == Qt::PermissionStatus::Granted) startScan();
        else setStatus(tr("Bluetooth permission was denied"));
      });
      return;
    case Qt::PermissionStatus::Denied:
      setStatus(tr("Bluetooth permission was denied. Allow it in the phone's app settings."));
      return;
    case Qt::PermissionStatus::Granted:
      break;
  }
#endif
  beginDiscovery();
}

void BleMidiSource::beginDiscovery() {
  if (!agent_) {
    agent_ = new QBluetoothDeviceDiscoveryAgent(this);
    agent_->setLowEnergyDiscoveryTimeout(10000);
    connect(agent_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BleMidiSource::onDeviceDiscovered);
    connect(agent_, &QBluetoothDeviceDiscoveryAgent::finished, this, [this] {
      setScanning(false);
      setStatus(deviceInfos_.empty() ? tr("No Bluetooth devices found") : tr("Tap a device to connect"));
    });
    connect(agent_, &QBluetoothDeviceDiscoveryAgent::canceled, this, [this] { setScanning(false); });
    connect(agent_, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, [this] {
      setScanning(false);
      setStatus(tr("Bluetooth error: %1").arg(agent_->errorString()));
    });
  }
  deviceInfos_.clear();
  deviceNames_.clear();
  emit devicesChanged();
  setScanning(true);
  setStatus(tr("Searching for Bluetooth MIDI devices..."));
  agent_->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BleMidiSource::stopScan() {
  if (agent_ && agent_->isActive()) agent_->stop();
  setScanning(false);
}

void BleMidiSource::onDeviceDiscovered(const QBluetoothDeviceInfo& info) {
  if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) return;
  const bool isMidi = info.serviceUuids().contains(kMidiService);
  if (info.name().isEmpty() && !isMidi) return;  // naamloze toestellen zijn zelden interessant
  for (const QBluetoothDeviceInfo& known : deviceInfos_) {
    if (known.address() == info.address() && known.deviceUuid() == info.deviceUuid()) return;
  }
  const QString name = info.name().isEmpty() ? tr("Unnamed device") : info.name();
  // MIDI-toestellen bovenaan, met een label, zodat de PSoC meteen opvalt tussen koptelefoons e.d.
  const auto position = isMidi ? deviceInfos_.begin() : deviceInfos_.end();
  const int row = static_cast<int>(position - deviceInfos_.begin());
  deviceInfos_.insert(position, info);
  deviceNames_.insert(row, isMidi ? name + QStringLiteral("  •  MIDI") : name);
  emit devicesChanged();
}

void BleMidiSource::connectTo(int index) {
  if (index < 0 || index >= static_cast<int>(deviceInfos_.size())) return;
  stopScan();
  disconnectDevice();
  const QBluetoothDeviceInfo info = deviceInfos_[static_cast<std::size_t>(index)];
  const QString name = info.name().isEmpty() ? tr("device") : info.name();
  setStatus(tr("Connecting to %1...").arg(name));

  controller_ = QLowEnergyController::createCentral(info, this);
  connect(controller_, &QLowEnergyController::connected, controller_, &QLowEnergyController::discoverServices);
  connect(controller_, &QLowEnergyController::discoveryFinished, this, [this, name] {
    if (!controller_->services().contains(kMidiService)) {
      setStatus(tr("%1 has no Bluetooth MIDI service").arg(name));
      releaseController();
      return;
    }
    service_ = controller_->createServiceObject(kMidiService, this);
    if (!service_) return;
    connect(service_, &QLowEnergyService::stateChanged, this, [this, name](QLowEnergyService::ServiceState state) {
      if (state == QLowEnergyService::RemoteServiceDiscovered) {
        onServiceDiscovered();
        if (connected_ || !service_) return;
        setConnected(true, name);
        setStatus(tr("Connected to %1. Play on it!").arg(name));
      }
    });
    // Elke notificatie op de MIDI-characteristic is een BLE-MIDI-pakket.
    connect(service_, &QLowEnergyService::characteristicChanged, this,
            [this](const QLowEnergyCharacteristic&, const QByteArray& value) { handlePacket(value); });
    connect(service_, &QLowEnergyService::characteristicRead, this,
            [this](const QLowEnergyCharacteristic&, const QByteArray& value) { handlePacket(value); });
    service_->discoverDetails();
  });
  connect(controller_, &QLowEnergyController::errorOccurred, this, [this] {
    setStatus(tr("Connection error: %1").arg(controller_->errorString()));
    releaseController();
  });
  connect(controller_, &QLowEnergyController::disconnected, this, [this, name] {
    setStatus(tr("%1 disconnected").arg(name));
    releaseController();
  });
  controller_->connectToDevice();
}

void BleMidiSource::onServiceDiscovered() {
  const QLowEnergyCharacteristic characteristic = service_->characteristic(kMidiCharacteristic);
  if (!characteristic.isValid()) {
    setStatus(tr("The MIDI characteristic is missing"));
    releaseController();
    return;
  }
  // Notificaties aanzetten: schrijf 0x0001 naar de Client Characteristic Configuration-descriptor.
  const QLowEnergyDescriptor config =
      characteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
  if (config.isValid()) service_->writeDescriptor(config, QByteArray::fromHex("0100"));
  // De specificatie vraagt een eerste read (een leeg pakket); sommige toestellen starten pas daarna.
  service_->readCharacteristic(characteristic);
}

void BleMidiSource::releaseController() {
  parser_.reset();
  if (service_) {
    service_->deleteLater();
    service_ = nullptr;
  }
  if (controller_) {
    controller_->disconnect(this);  // geen signalen meer van een controller die we weggooien
    controller_->disconnectFromDevice();
    controller_->deleteLater();
    controller_ = nullptr;
  }
  if (connected_) setConnected(false);
  if (onMessage_) onMessage_(midi::MidiMessage(0xB0, 123, 0));  // All Notes Off: geen hangende noten
}

void BleMidiSource::disconnectDevice() {
  if (!controller_) return;
  releaseController();
  setStatus(tr("Not connected"));
}

#else  // Zonder Qt Bluetooth: de app werkt, alleen de Bluetooth-knoppen doen niets.

void BleMidiSource::startScan() {}
void BleMidiSource::stopScan() {}
void BleMidiSource::connectTo(int) {}
void BleMidiSource::disconnectDevice() {}

#endif

}  // namespace midipiano::app
