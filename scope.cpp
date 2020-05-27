#include "scope.hpp"

#include <fstream>
#include <iostream>
#include <future>
#include <chrono>

#include "resources.h"
#include "GL/glew.h"

// The scope displays a live signal from jack. one thread reads blocks from jack,
// and puts them on the back of a list. if the size of the list is equal to
// `n_buffers` then futher blocks will be ignored until there is space. If the
// number of buffers accumulated is greater than or equal to `n_buffers`, and a
// redraw has not already been requested since the last draw then this thread will
// request a redraw.
//
// Concurrently, the render thread will render a new frame if the number of buffers
// accumulated is greater than or equal to `n_buffers`. In this case, a new vector for
// vertices is allocated, the render thread will then pop `n_buffers` buffers from the
// front of the list, and use its contents to fill the vertex buffer.
//
// it is possible that the jack thread writes buffers to the back of the list,
// while the front of the list is being popped by the render thread.
//
// since the jack thread can only add to the back, and the render thread can only pop from the front,
// this should be threadsafe.

Scope::Scope(GtkGLArea* cobj, const Glib::RefPtr<Gtk::Builder>& builder, const std::string& port_name)
  : Gtk::GLArea(cobj), builder(builder), connected_port_name(port_name)
{
  signal_realize().connect(sigc::mem_fun(this, &Scope::scope_realize));
  signal_unrealize().connect(sigc::mem_fun(this, &Scope::scope_unrealize));
  signal_create_context().connect(sigc::mem_fun(this, &Scope::gl_create_context));
  signal_render().connect(sigc::mem_fun(this, &Scope::render));

  line_width_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder->get_object("line_width"));
  line_width_adj->signal_value_changed().connect(sigc::mem_fun(this, &Scope::adj_line_width_change));
  line_width = (float) line_width_adj->get_value();

  n_buffers_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder->get_object("n_buffers"));
  n_buffers_adj->signal_value_changed().connect(sigc::mem_fun(this, &Scope::adj_n_buffers_change));
  max_buffers = n_buffers_adj->get_upper();
  n_buffers = n_buffers_adj->get_value();

  buffers_per_frame_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(builder->get_object("buffers_per_frame"));
  buffers_per_frame_adj->signal_value_changed().connect(sigc::mem_fun(this, &Scope::adj_buffers_per_frame));
  adj_buffers_per_frame();

  render_dispatch.connect(sigc::mem_fun(this, &Scope::queue_render));
}

void Scope::adj_n_buffers_change() {
  n_buffers = n_buffers_adj->get_value();
  if (buffers_per_frame > n_buffers) {
    buffers_per_frame = n_buffers;
    buffers_per_frame_adj->set_value(n_buffers);
  }
  make_current();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, buffer_size * n_buffers * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
}

void Scope::adj_buffers_per_frame() {
  size_t buffers_per_frame_adj_val = buffers_per_frame_adj->get_value();
  if (buffers_per_frame_adj_val <= n_buffers) {
    buffers_per_frame = buffers_per_frame_adj->get_value();
  } else {
    buffers_per_frame = n_buffers;
    buffers_per_frame_adj->set_value(n_buffers);
  }
}

void Scope::scope_realize() {
  std::cout << "realize" << std::endl;

  init_audio();
  connect_to_port();
  gl_init();
}

void Scope::scope_unrealize() {
  gl_uninit();
  jack_deactivate(jack_client);
}

// Audio

void Scope::init_audio()
{
  std::cout << "init audio" << std::endl;
  jack_status_t jack_status;
  jack_client = jack_client_open("jack analyser", JackOptions::JackNoStartServer, &jack_status);
  if (jack_status == JackStatus::JackFailure) throw std::runtime_error("jack error: " + jack_status);

  in_port = jack_port_register(
    jack_client,
    "input",
    JACK_DEFAULT_AUDIO_TYPE,
    JackPortFlags::JackPortIsInput | JackPortFlags::JackPortIsTerminal,
    0
  );

  jack_set_process_callback(jack_client, jack_process_callback, this);
  jack_set_buffer_size_callback(jack_client, jack_buffer_size_callback, this);

  buffer_size = jack_get_buffer_size(jack_client);

  jack_activate(jack_client);
}

int Scope::jack_process_callback(jack_nframes_t n_frames, void* data)
{
  static int missed_blocks;
  auto* scope = (Scope*) data;

  if (scope->buffers.size() < scope->max_buffers) {
    missed_blocks = 0;

    auto* raw_ptr = (float*) jack_port_get_buffer(scope->in_port, n_frames);
    std::vector<float> buffer(raw_ptr, raw_ptr + n_frames);

    scope->buffers.push_back(buffer);
  } else {
    std::async([]() {
      std::cerr << "missed " << ++missed_blocks << " blocks" << std::endl;
    });
  }
  if (!scope->render_queued && scope->buffers.size() >= scope->n_buffers) {
    scope->render_dispatch.emit();
    scope->render_queued = true;
  }

  return 0;
}

int Scope::jack_buffer_size_callback(jack_nframes_t n_frames, void* data)
{
  Scope* scope = (Scope*) data;
  scope->buffer_size = n_frames;
  return 0;
}


void Scope::connect_to_port()
{
  std::cout << "connecting to " << connected_port_name << std::endl;
  if (int err = jack_connect(jack_client, connected_port_name.c_str(), jack_port_name(in_port)) != 0) {
    std::cerr << "couldn't connect to " << connected_port_name << " jack error: " << err;
  }
}

// Video

void opengl_debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam)
{
  if (severity == GL_DEBUG_SEVERITY_HIGH) {
    throw std::runtime_error(message);
  }
}


Glib::RefPtr<Gdk::GLContext> Scope::gl_create_context() {
  std::cout << "create context" << std::endl;
  Glib::RefPtr<Gdk::GLContext> context = get_context();
  if (GLEW_KHR_debug) {
    context->set_debug_enabled();
  }
  return context;
}

void Scope::gl_init()
{
  std::cout << "init video" << std::endl;
  make_current();
  if (GLenum err = glewInit() != GLEW_OK) {
    throw std::runtime_error((const char*) glewGetErrorString(err));
  }
  if (GLEW_KHR_debug) {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_debug_callback, nullptr);
  }
  gl_init_buffers();
  gl_init_shaders();
}

void Scope::gl_uninit()
{
  glDeleteProgram(program);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void Scope::gl_init_buffers()
{
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, buffer_size * n_buffers * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void Scope::gl_init_shaders()
{
  auto compile_shader = [](GLenum shader_type, const char * glsl_file) -> GLuint {
    GLuint shader = glCreateShader(shader_type);

    std::ifstream fs(glsl_file);
    std::stringstream shader_code_stream;
    shader_code_stream << fs.rdbuf();

    std::string shader_code = shader_code_stream.str();
    const GLchar * cstr = shader_code.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);

    return shader;
  };

  GLuint vert_shader = compile_shader(GL_VERTEX_SHADER, RESOURCES "scope.vert");
  GLuint frag_shader = compile_shader(GL_FRAGMENT_SHADER, RESOURCES "scope.frag");
  program = glCreateProgram();

  glAttachShader(program, vert_shader);
  glAttachShader(program, frag_shader);
  glBindAttribLocation(program, 0, "pos");
  glBindFragDataLocation(program, 0, "color");
  glLinkProgram(program);

  GLint link_status;
  glGetProgramiv(program, GL_LINK_STATUS, &link_status);
  if (link_status == GL_FALSE) {
    GLint log_size = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);

    std::vector<GLchar> log_contents(log_size);
    glGetProgramInfoLog(program, log_size, &log_size, log_contents.data());

    std::stringstream error_message;
    error_message << "error linking program";
    error_message << std::string(log_contents.data());

    throw std::runtime_error(error_message.str());
  }

  glDetachShader(program, vert_shader);
  glDeleteShader(vert_shader);
  glDetachShader(program, frag_shader);
  glDeleteShader(frag_shader);

  glUseProgram(program);
}

bool Scope::render(const Glib::RefPtr<Gdk::GLContext>& context)
{
  if (buffers.size() >= n_buffers) {
    auto begin = std::chrono::steady_clock::now();

    std::vector<Vertex> vertices;
    vertices.reserve(buffer_size * n_buffers);

    float x = -1.0;
    float x_inc = 2.0f / (float) (n_buffers * (buffer_size - 2));

    {
      int i = 0;
      for (std::vector<float> samples : buffers) {
        for (float sample : samples) {
          vertices.push_back({{x, sample}});
          x += x_inc;
        }
        if (++i == (int) n_buffers) {
          break;
        }
      }
    }
    for (int i = 0; i < (int) buffers_per_frame; i++) {
      buffers.pop_front();
    }

    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    glLineWidth(line_width);
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() - 1);

    auto render_duration = std::chrono::steady_clock::now() - begin;
    auto wait_duration = std::chrono::milliseconds(16) - render_duration;
    if (wait_duration.count() > 0) {
      std::this_thread::sleep_for(wait_duration);
    }
  }
  render_queued = false;
  return true;
}
