#include <fstream>
#include <sstream>
#include <iostream>

#include "scope.hpp"
#include "resources.h"
#include "GL/glew.h"

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

int Scope::jack_process_callback(jack_nframes_t n_frames, void* data)
{
  Scope* scope = (Scope*) data;

  if (scope->buffers.size() < scope->n_buffers) {
    float* raw_ptr = (float*) jack_port_get_buffer(scope->in_port, n_frames);

    std::vector<float> buffer(raw_ptr, raw_ptr + n_frames);

    scope->buffers.push_front(buffer);
  }

  if (scope->buffers.size() == scope->n_buffers) {
    scope->gl_area->queue_render();
  }

  return 0;
}

int Scope::jack_buffer_size_callback(jack_nframes_t n_frames, void* data)
{
  Scope* scope = (Scope*) data;
  scope->buffer_size = n_frames;
  return 0;
}

Scope::Scope(std::string connected_port_name, Gtk::GLArea* gl_area) :
  connected_port_name(connected_port_name),
  gl_area(gl_area)
{
  gl_area->signal_realize().connect(sigc::mem_fun(this, &Scope::init_gl));
  gl_area->signal_unrealize().connect(sigc::mem_fun(this, &Scope::deinit_gl));
  gl_area->signal_render().connect(sigc::mem_fun(this, &Scope::render));
  init_audio();
}

Scope::~Scope() {
  jack_deactivate(jack_client);
}

void Scope::init_audio()
{
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

  if (int err = jack_connect(jack_client, connected_port_name.c_str(), jack_port_name(in_port)) != 0) {
    std::cerr << "couldn't connect to " + connected_port_name;
  }
}

void Scope::init_gl()
{
  gl_area->make_current();
  if (GLenum err = glewInit() != GLEW_OK) {
    throw std::runtime_error((const char*) glewGetErrorString(err));
  }
  if (GLEW_KHR_debug) {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_debug_callback, nullptr);
  }
  init_gl_buffers();
  init_shaders();
}

void Scope::deinit_gl()
{
  glDeleteProgram(program);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void Scope::init_gl_buffers()
{
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, buffer_size * n_buffers * sizeof(Vertex), nullptr, GL_STREAM_DRAW);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void Scope::init_shaders()
{
  auto compile_shader = [](GLenum shader_type, const char * glsl_file) -> GLuint {
    GLuint shader = glCreateShader(shader_type);

    std::ifstream fs(glsl_file);
    std::stringstream shader_code_stream;
    shader_code_stream << fs.rdbuf();

    std::string shader_code = shader_code_stream.str();
    const GLchar * cstr = shader_code.c_str();
    glShaderSource(shader, 1, &cstr, 0);
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
  long n_buffers = this->n_buffers;
  if (buffers.size() >= n_buffers) {
    std::vector<Vertex> vertices;
    vertices.reserve(buffer_size * n_buffers * sizeof(float));

    float x = -1.0;
    float x_inc = 2.0 / (n_buffers * (buffer_size - 1));

    for (int i = 0; i < n_buffers; i++) {
      std::vector<float> samples = buffers.back();
      for (float sample : samples) {
        vertices.push_back({{x, sample}});
        x += x_inc;
      }
      buffers.pop_back();
    }

    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    glLineWidth(5.0);
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size() - 1);
  }
  return true;
}
