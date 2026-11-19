#include "WaveformView.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QtQuick/qquickopenglutils.h>

#include <algorithm>

namespace midipiano::app {

namespace {

constexpr std::size_t kDisplaySamples = 512;

// Vereiste 57: 57.usage of OpenGL or other 3D engine
// ScopeRenderer tekent de golfvorm rechtstreeks met OpenGL (OpenGL ES op Android): een eigen
// vertex- en fragment-shader, een raster en de golfvorm als GL_LINE_STRIP in een framebuffer-object
// met multisampling (gladde lijnen). Qt Quick plakt dat framebuffer daarna in de interface. Zo kost
// het hertekenen van 512 punten, 30 keer per seconde, bijna niets.
class ScopeRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
 public:
  ScopeRenderer() {
    initializeOpenGLFunctions();
    program_.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                     "attribute highp vec2 position;\n"
                                     "void main() { gl_Position = vec4(position, 0.0, 1.0); }\n");
    program_.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                     "uniform lowp vec4 color;\n"
                                     "void main() { gl_FragColor = color; }\n");
    program_.bindAttributeLocation("position", 0);
    program_.link();
  }

  QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);  // Qt Quick neemt het eigendom over
  }

  void synchronize(QQuickFramebufferObject* item) override {
    const auto* view = static_cast<WaveformView*>(item);
    color_ = view->lineColor();
    const std::vector<float>& samples = view->displaySamples();
    wave_.clear();
    const std::size_t count = samples.size();
    for (std::size_t i = 0; i < count; ++i) {
      const float x = count > 1 ? -1.0f + 2.0f * static_cast<float>(i) / static_cast<float>(count - 1) : 0.0f;
      wave_.push_back(x);
      wave_.push_back(std::clamp(samples[i], -1.0f, 1.0f) * 0.9f);
    }
  }

  void render() override {
    glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    program_.bind();
    program_.enableAttributeArray(0);

    // Raster: 8 vakken horizontaal, een middenlijn en lijnen op +-0,5.
    std::vector<float> grid;
    for (int i = 1; i < 8; ++i) {
      const float x = -1.0f + static_cast<float>(i) * 0.25f;
      grid.insert(grid.end(), {x, -1.0f, x, 1.0f});
    }
    grid.insert(grid.end(), {-1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.45f, 1.0f, 0.45f, -1.0f, -0.45f, 1.0f, -0.45f});
    program_.setUniformValue("color", QColor(255, 255, 255, 28));
    program_.setAttributeArray(0, GL_FLOAT, grid.data(), 2);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(grid.size() / 2));

    if (wave_.size() >= 4) {
      glLineWidth(2.0f);
      program_.setUniformValue("color", color_);
      program_.setAttributeArray(0, GL_FLOAT, wave_.data(), 2);
      glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(wave_.size() / 2));
    }

    program_.disableAttributeArray(0);
    program_.release();
    QQuickOpenGLUtils::resetOpenGLState();  // laat Qt Quick verder tekenen met een propere toestand
  }

 private:
  QOpenGLShaderProgram program_;
  std::vector<float> wave_;
  QColor color_;
};

}  // namespace

WaveformView::WaveformView(QQuickItem* parent) : QQuickFramebufferObject(parent) {
  setMirrorVertically(true);  // OpenGL telt y van onder naar boven, Qt Quick van boven naar onder
}

QQuickFramebufferObject::Renderer* WaveformView::createRenderer() const { return new ScopeRenderer(); }

void WaveformView::setSynth(SynthController* synth) {
  if (synth_ == synth) return;
  disconnect(connection_);
  synth_ = synth;
  if (synth_) connection_ = connect(synth_, &SynthController::scopeUpdated, this, &WaveformView::refresh);
  emit synthChanged();
}

void WaveformView::setLineColor(const QColor& color) {
  if (lineColor_ == color) return;
  lineColor_ = color;
  emit lineColorChanged();
  update();
}

void WaveformView::refresh() {
  if (!synth_) return;
  synth_->copyScope(scratch_);
  if (scratch_.size() < kDisplaySamples) return;
  // Trigger: begin bij de eerste stijgende nuldoorgang, zodat een aangehouden noot stilstaat op het
  // scherm in plaats van te verschuiven (zoals een echte oscilloscoop).
  std::size_t start = 0;
  const std::size_t searchEnd = scratch_.size() - kDisplaySamples;
  for (std::size_t i = 1; i <= searchEnd; ++i) {
    if (scratch_[i - 1] < 0.0f && scratch_[i] >= 0.0f) {
      start = i;
      break;
    }
  }
  display_.assign(scratch_.begin() + static_cast<std::ptrdiff_t>(start),
                  scratch_.begin() + static_cast<std::ptrdiff_t>(start + kDisplaySamples));
  update();
}

}  // namespace midipiano::app
