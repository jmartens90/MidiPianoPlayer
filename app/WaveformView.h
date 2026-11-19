#ifndef MIDIPIANO_APP_WAVEFORMVIEW_H
#define MIDIPIANO_APP_WAVEFORMVIEW_H

#include <QColor>
#include <QPointer>
#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

#include <vector>

#include "SynthController.h"

namespace midipiano::app {

// Een oscilloscoop in QML die de laatste samples van de synth tekent met OpenGL.
class WaveformView : public QQuickFramebufferObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(midipiano::app::SynthController* synth READ synth WRITE setSynth NOTIFY synthChanged)
  Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)

 public:
  explicit WaveformView(QQuickItem* parent = nullptr);

  Renderer* createRenderer() const override;

  SynthController* synth() const { return synth_; }
  void setSynth(SynthController* synth);
  QColor lineColor() const { return lineColor_; }
  void setLineColor(const QColor& color);

  // De renderer leest deze in synchronize(), terwijl de GUI-thread geblokkeerd is.
  const std::vector<float>& displaySamples() const { return display_; }

 signals:
  void synthChanged();
  void lineColorChanged();

 private slots:
  void refresh();

 private:
  QPointer<SynthController> synth_;
  QMetaObject::Connection connection_;
  std::vector<float> scratch_;
  std::vector<float> display_;
  QColor lineColor_ = QColor("#4dd0c1");
};

}  // namespace midipiano::app

#endif  // MIDIPIANO_APP_WAVEFORMVIEW_H
