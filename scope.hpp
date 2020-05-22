#pragma once
#include <vector>
#include <gtkmm.h>
#include <jack/jack.h>

class Scope : public Gtk::GLArea {
private:
  struct Vertex {
    float pos[2];
  };
  std::list<std::vector<float>> buffers;
  long n_buffers;
  long buffer_size;

  void gl_area_realize();
  void gl_area_unrealize();
  Glib::RefPtr<Gdk::GLContext> gl_area_create_context();
  void init_gl_shaders();
  void init_gl_buffers();
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

  Glib::RefPtr<Gtk::Builder> builder;
  Glib::RefPtr<Gtk::Adjustment> line_width_adj;
  void adj_line_width_change() { line_width = line_width_adj->get_value(); };
  Glib::RefPtr<Gtk::Adjustment> n_buffers_adj;
  void adj_n_buffers_change();

public:
  bool render(const Glib::RefPtr<Gdk::GLContext>& context);
  void set_n_buffers(long n_buffers) { this->n_buffers = n_buffers; };
  void connect_to_port(std::string port_name);

  Scope(GtkGLArea* cobj, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~Scope();
};
