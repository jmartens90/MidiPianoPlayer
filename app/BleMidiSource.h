#ifndef MIDIPIANO_APP_BLEMIDISOURCE_H
#define MIDIPIANO_APP_BLEMIDISOURCE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <vector>

#include "midipiano/midi/BleMidiParser.h"
#include "midipiano/midi/MidiSource.h"

#ifdef MIDIPIANO_HAS_BLUETOOTH
#include <QBluetoothDeviceInfo>
class QBluetoothDeviceDiscoveryAgent;
class QLowEnergyController;
class QLowEnergyService;
#endif

namespace midipiano::app {

// MIDI-bron via Bluetooth Low Energy: zoekt BLE-MIDI-toestellen (zoals de PSoC 6-sequencer of een
// BLE-keyboard), maakt verbinding, schrijft zich in op de MIDI-characteristic en geeft elke
// ontvangen noot door aan de synth. Erft van MidiSource, net als de DemoSequencer: SynthController
// behandelt beide bronnen op dezelfde manier. Erft ook van QObject, zodat QML de lijst met toestellen
// en de status kan tonen (Instellingen-paneel).
class BleMidiSource : public QObject, public midi::MidiSource {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Provided by SynthController.bluetooth")
  Q_PROPERTY(bool available READ available CONSTANT)
  Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
  Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
  Q_PROPERTY(QString connectedName READ connectedName NOTIFY connectionChanged)
  Q_PROPERTY(QStringList devices READ devices NOTIFY devicesChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(int messageCount READ messageCount NOTIFY messageCountChanged)

 public:
  explicit BleMidiSource(QObject* parent = nullptr);
  ~BleMidiSource() override;

  // MidiSource
  void start(Callback onMessage) override;
  void stop() override;
  bool isRunning() const override { return connected_; }
  std::string name() const override { return "Bluetooth MIDI"; }

  Q_INVOKABLE void startScan();
  Q_INVOKABLE void stopScan();
  Q_INVOKABLE void connectTo(int index);
  Q_INVOKABLE void disconnectDevice();

  bool available() const;
  bool scanning() const { return scanning_; }
  bool connected() const { return connected_; }
  QString connectedName() const { return connectedName_; }
  QStringList devices() const { return deviceNames_; }
  QString statusText() const { return statusText_; }
  int messageCount() const { return messageCount_; }

  // Voor een pakket dat binnenkomt (ook bruikbaar zonder Bluetooth-hardware).
  void handlePacket(const QByteArray& packet);

 signals:
  void scanningChanged();
  void connectionChanged();
  void devicesChanged();
  void statusTextChanged();
  void messageCountChanged();

 private:
  void setStatus(const QString& text);
  void setScanning(bool scanning);
  void setConnected(bool connected, const QString& name = {});

  Callback onMessage_;
  midi::BleMidiParser parser_;
  QStringList deviceNames_;
  QString statusText_;
  QString connectedName_;
  bool scanning_ = false;
  bool connected_ = false;
  int messageCount_ = 0;

#ifdef MIDIPIANO_HAS_BLUETOOTH
  void beginDiscovery();
  void onDeviceDiscovered(const QBluetoothDeviceInfo& info);
  void onServiceDiscovered();
  void releaseController();

  QBluetoothDeviceDiscoveryAgent* agent_ = nullptr;  // Qt-kind van dit object
  QLowEnergyController* controller_ = nullptr;
  QLowEnergyService* service_ = nullptr;
  std::vector<QBluetoothDeviceInfo> deviceInfos_;
#endif
};

}  // namespace midipiano::app

#endif  // MIDIPIANO_APP_BLEMIDISOURCE_H
