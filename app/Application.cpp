#include "Application.h"

#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/qnativeinterface.h>
#endif

namespace midipiano::app {

Application::Application(int& argc, char** argv) : app_(argc, argv) {
  QCoreApplication::setOrganizationName("MidiPianoPlayer");
  QCoreApplication::setApplicationName("MidiPianoPlayer");
  // De oscilloscoop (WaveformView) tekent met OpenGL. Op Windows en macOS kiest Qt standaard
  // Direct3D of Metal, dus we vragen expliciet OpenGL (op Android is het OpenGL ES).
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QQuickStyle::setStyle("Material");
}

void Application::lockLandscape() {
#ifdef Q_OS_ANDROID
  // Een pianotoetsenbord heeft breedte nodig: zet de app op Android altijd liggend
  // (Activity.setRequestedOrientation(SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 6)).
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
    QJniObject activity(QNativeInterface::QAndroidApplication::context());
    if (activity.isValid()) activity.callMethod<void>("setRequestedOrientation", "(I)V", 6);
  });
#endif
}

int Application::run() {
  lockLandscape();
  QObject::connect(
      &engine_, &QQmlApplicationEngine::objectCreationFailed, &app_, [] { QCoreApplication::exit(1); },
      Qt::QueuedConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  engine_.loadFromModule("MidiPiano", "Main");
#else
  engine_.load(QUrl(QStringLiteral("qrc:/MidiPiano/qml/Main.qml")));
#endif
  return app_.exec();
}

}  // namespace midipiano::app
