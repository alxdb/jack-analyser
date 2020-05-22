#include <gtkmm.h>
#include <jack/jack.h>
#include <iostream>

#include "scope.hpp"
#include "resources.h"

Gtk::Window* window = nullptr;
Scope* scope_draw = nullptr;

int main(int argc, char** argv) {
  auto app = Gtk::Application::create(argc, argv, "adb.jackanalyser");
  auto builder = Gtk::Builder::create_from_file(RESOURCES "main.glade");
  builder->get_widget("main_window", window);

  builder->get_widget_derived("scope_draw", scope_draw, "PulseAudio JACK Sink:front-left");

  return app->run(*window);
}
