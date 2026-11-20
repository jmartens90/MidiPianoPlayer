#include "Application.h"

#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>

namespace midipiano::app {

Application::Application(int& argc, char** argv) : app_(argc, argv) {
  QCoreApplication::setOrganizationName("MidiPianoPlayer");
  QCoreApplication::setApplicationName("MidiPianoPlayer");
  // De oscilloscoop (WaveformView) tekent met OpenGL. Op Windows en macOS kiest Qt standaard
  // Direct3D of Metal, dus we vragen expliciet OpenGL (op Android is het OpenGL ES).
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Material");
}

int Application::run() {
  QObject::connect(
      &engine_, &QQmlApplicationEngine::objectCreationFailed, &app_, [] { QCoreApplication::exit(1); },
      Qt::QueuedConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  engine_.loadFromModule("MidiPiano", "Main");
#else
  engine_.load(QUrl(QStringLiteral("qrc:/MidiPiano/qml/Main.qml")));
#endif
#ifdef Q_OS_ANDROID
  // Op een gsm schermvullend ("immersive"): geen doorschijnende statusbalk bovenaan en geen terug- en
  // home-knoppen over het toetsenbord. Vanaf de rand vegen toont ze even.
  if (!engine_.rootObjects().isEmpty()) {
    if (auto* window = qobject_cast<QQuickWindow*>(engine_.rootObjects().constFirst())) window->showFullScreen();
  }
#endif
  return app_.exec();
}

}  // namespace midipiano::app
