#include <exception>
#include <iostream>

#include "DemoApp.h"

// Vereiste 11: 11.clean main (i.e. nothing in the main that should be in a class)
int main() {
  try {
    midipiano::DemoApp app;
    app.run("demo.wav");
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Fout: " << e.what() << '\n';
    return 1;
  }
}
