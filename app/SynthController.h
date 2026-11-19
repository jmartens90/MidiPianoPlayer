#ifndef MIDIPIANO_APP_SYNTHCONTROLLER_H
#define MIDIPIANO_APP_SYNTHCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <vector>

#include "midipiano/audio/AudioOutput.h"
#include "midipiano/dsp/Synth.h"
#include "midipiano/midi/MidiSource.h"
#include "midipiano/preset/PresetStore.h"

namespace midipiano::app {

// Vereiste 51: 51.useful Qt class (self made)
// SynthController is een eigen QObject dat de C++-synth koppelt aan de QML-interface. Met
// Q_PROPERTY en Q_INVOKABLE kan QML rechtstreeks noten spelen, knoppen instellen en presets laden,
// en via signals krijgt QML te horen wanneer er iets verandert (patch, presets, aantal stemmen).
// QML_ELEMENT maakt het een QML-type: in Main.qml staat gewoon "SynthController { id: synth }".
class SynthController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(int waveform READ waveform WRITE setWaveform NOTIFY patchChanged)
  Q_PROPERTY(QStringList waveformNames READ waveformNames CONSTANT)
  Q_PROPERTY(QString patchName READ patchName NOTIFY patchChanged)
  Q_PROPERTY(QStringList presets READ presets NOTIFY presetsChanged)
  Q_PROPERTY(bool sequencerRunning READ sequencerRunning NOTIFY sequencerRunningChanged)
  Q_PROPERTY(int activeVoices READ activeVoices NOTIFY activeVoicesChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(int instrument READ instrument WRITE setInstrument NOTIFY instrumentChanged)
  Q_PROPERTY(QStringList instrumentNames READ instrumentNames CONSTANT)
  Q_PROPERTY(bool synthControlsEnabled READ synthControlsEnabled NOTIFY instrumentChanged)

 public:
  explicit SynthController(QObject* parent = nullptr);
  ~SynthController() override;

  Q_INVOKABLE void noteOn(int note, int velocity = 100);
  Q_INVOKABLE void noteOff(int note);
  Q_INVOKABLE void allNotesOff();
  Q_INVOKABLE double param(const QString& name) const;
  Q_INVOKABLE void setParam(const QString& name, double value);
  Q_INVOKABLE void toggleSequencer();
  Q_INVOKABLE void savePreset(const QString& name);
  Q_INVOKABLE void loadPreset(const QString& name);

  int waveform() const;
  void setWaveform(int waveform);
  QStringList waveformNames() const;
  QString patchName() const { return patchName_; }
  QStringList presets() const { return presets_; }
  bool sequencerRunning() const;
  int activeVoices() const { return activeVoices_; }
  QString statusText() const { return statusText_; }
  int instrument() const { return static_cast<int>(synth_.currentInstrument()); }
  void setInstrument(int index);
  QStringList instrumentNames() const { return instrumentNames_; }
  bool synthControlsEnabled() const { return synth_.currentUsesPatch(); }

  void copyScope(std::vector<float>& out) const { synth_.scopeSnapshot(out); }

 signals:
  void patchChanged();
  void presetsChanged();
  void sequencerRunningChanged();
  void activeVoicesChanged();
  void statusTextChanged();
  void instrumentChanged();
  void noteStateChanged(int note, bool on);  // noten van de sequencer, om toetsen te laten oplichten
  void scopeUpdated();

 private slots:
  void onTick();

 private:
  void onSourceMessage(const midi::MidiMessage& message);
  void refreshPresets();
  void loadSoundFont();
  void setStatus(const QString& text);

  dsp::Synth synth_;  // als eerste gedeclareerd, dus als laatste vernietigd (audio_ gebruikt hem)
  // Vereiste 40: 40.2 useful and correct smart pointers
  // De controller is de enige eigenaar van de geluidskaart-koppeling en van de MIDI-bron, dus
  // std::unique_ptr. audio_ mag leeg blijven als er geen geluidskaart is (de app werkt dan stil
  // verder), en source_ wijst naar een MidiSource: nu de DemoSequencer, later de BLE-bron.
  std::unique_ptr<audio::AudioOutput> audio_;
  std::unique_ptr<midi::MidiSource> source_;
  std::unique_ptr<preset::PresetStore> store_;
  QTimer* tick_;
  QString patchName_ = "Init";
  QStringList presets_;
  QStringList instrumentNames_;
  QString statusText_;
  int activeVoices_ = 0;
};

}  // namespace midipiano::app

#endif  // MIDIPIANO_APP_SYNTHCONTROLLER_H
