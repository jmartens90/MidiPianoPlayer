#include "Application.h"

// Vereiste 11: 11.clean main (i.e. nothing in the main that should be in a class)
// main() maakt alleen de Application aan en start ze. Alle opzet (Qt, OpenGL, stijl, QML laden)
// zit in de klasse Application, de logica in SynthController en de synth in midipiano_core.
int main(int argc, char* argv[]) {
  midipiano::app::Application application(argc, argv);
  return application.run();
}
