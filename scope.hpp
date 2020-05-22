#pragma once
#include <vector>
#include <gtkmm.h>
#include <jack/jack.h>

class Scope {
private:
  struct Vertex {
    float pos[2];
  };
  std::list<std::vector<float>> buffers;

  Gtk::GLArea* gl_area;
  void init_gl();
  void deinit_gl();
  void resize();
  void init_shaders();
  void init_gl_buffers();
  unsigned int vbo;
  unsigned int vao;
  unsigned int program;

  void init_audio();
  static int jack_process_callback(jack_nframes_t, void*);
  static int jack_buffer_size_callback(jack_nframes_t, void*);
  jack_client_t* jack_client;
  jack_port_t* in_port;
  std::string connected_port_name;
  long n_buffers = 32;
  long buffer_size;
public:
  bool render(const Glib::RefPtr<Gdk::GLContext>& context);
  void set_n_buffers(long n_buffers) { this->n_buffers = n_buffers; };

  Scope(std::string connected_port_name, Gtk::GLArea* area);
  ~Scope();
};
