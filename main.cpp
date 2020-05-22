#include <gtkmm.h>

#include "scope.hpp"
#include "resources.h"

int main(int argc, char** argv) {
  auto app = Gtk::Application::create(argc, argv, "adb.jackanalyser");

  auto builder = Gtk::Builder::create_from_file(RESOURCES "main.glade");

  Gtk::Window* window = nullptr;
  builder->get_widget("main_window", window);

  Scope* scope_draw = nullptr;
  builder->get_widget_derived("scope_draw", scope_draw, "PulseAudio JACK Sink:front-left");

  return app->run(*window);
}
