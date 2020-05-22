#pragma once
#include <vector>
#include <gtkmm.h>
#include <jack/jack.h>

class Scope : public Gtk::GLArea {
private:
  void realize();
  void unrealize();
  Glib::RefPtr<Gtk::Builder> builder;
  Glib::RefPtr<Gtk::Adjustment> line_width_adj;
  void adj_line_width_change() { line_width = line_width_adj->get_value(); };
  Glib::RefPtr<Gtk::Adjustment> n_buffers_adj;
  void adj_n_buffers_change();
  Glib::Dispatcher render_dispatch;

  size_t max_buffers;
  size_t n_buffers;
  size_t buffer_size;
  std::list<std::vector<float>> buffers;

  void gl_init();
  void gl_uninit();
  Glib::RefPtr<Gdk::GLContext> gl_create_context();
  void gl_init_shaders();
  void gl_init_buffers();
  bool render(const Glib::RefPtr<Gdk::GLContext>& context);
  struct Vertex {
    float pos[2];
  };
  unsigned int vbo;
  unsigned int vao;
  unsigned int program;
  float line_width;

  void init_audio();
  static int jack_process_callback(jack_nframes_t, void*);
  static int jack_buffer_size_callback(jack_nframes_t, void*);
  jack_client_t* jack_client;
  jack_port_t* in_port;
  std::string connected_port_name;
  void connect_to_port();

public:
  Scope(GtkGLArea* cobj, const Glib::RefPtr<Gtk::Builder>& builder, std::string port_name);
  virtual ~Scope() {};
};
