#include <gtkmm.h>
#include <jack/jack.h>
#include <iostream>

#include "scope.hpp"
#include "resources.h"

int main(int argc, char** argv) {
  auto app = Gtk::Application::create(argc, argv, "adb.jackanalyser");
  auto builder = Gtk::Builder::create_from_file(RESOURCES "main.glade");

  Gtk::Window* window = nullptr;
  builder->get_widget("main_window", window);
  Gtk::GLArea* scope_draw = nullptr;
  builder->get_widget("scope_draw", scope_draw);

  Scope scope("PulseAudio JACK Sink:front-left", scope_draw);

  return app->run(*window);
}
