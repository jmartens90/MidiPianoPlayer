#ifndef MIDIPIANO_APP_APPLICATION_H
#define MIDIPIANO_APP_APPLICATION_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>

namespace midipiano::app {

// Zet Qt op (OpenGL, stijl, app-naam) en laadt de QML-gebruikersinterface. Zo blijft main() leeg.
class Application {
 public:
  Application(int& argc, char** argv);
  int run();

 private:
  void lockLandscape();

  QGuiApplication app_;
  QQmlApplicationEngine engine_;
};

}  // namespace midipiano::app

#endif  // MIDIPIANO_APP_APPLICATION_H
