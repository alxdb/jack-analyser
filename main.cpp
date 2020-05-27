#include <gtkmm.h>

#include "scope.hpp"
#include "resources.h"

int main(int argc, char** argv) {
  auto app = Gtk::Application::create("adb.jackanalyser");

  auto builder = Gtk::Builder::create_from_file(RESOURCES "main.glade");

  Gtk::Window* window = nullptr;
  builder->get_widget("main_window", window);

  Scope* scope_draw = nullptr;
  std::string default_port;
  if (argc >= 2) {
    default_port = argv[1];
  } else {
    default_port = "PulseAudio JACK Sink:front-left";
  }
  builder->get_widget_derived("scope_draw", scope_draw, default_port);

  return app->run(*window);
}
