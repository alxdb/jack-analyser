#pragma once
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class Window {
  static int instances;

  class GLEWError : public std::runtime_error {

  public:
    GLEWError(GLenum err) : std::runtime_error(reinterpret_cast<const char*>(glewGetErrorString(err))) {}
  };

  class GLFWError : public std::runtime_error {
    std::string form_message(int code, const char* description) {
      std::stringstream message_stream;
      message_stream << "GLFW Error: { code: " << code << ", description: " << description << " }";
      return message_stream.str();
    }

  public:
    GLFWError(int code, const char* description) : std::runtime_error(form_message(code, description)) {}
  };

  static void glfw_error_callback(int code, const char* description) { throw GLFWError(code, description); }

  GLFWwindow* m_glfw_window;

  int m_width;
  int m_height;

  bool m_should_close = false;
  bool m_glew_init = false;
  bool m_imgui_init = false;

  static void glfw_close_callback(GLFWwindow* glfw_window) {
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    window->m_should_close = true;
  }

  static void glfw_size_callback(GLFWwindow* glfw_window, int new_width, int new_height) {
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    window->m_width = new_width;
    window->m_height = new_height;
  }

public:
  const std::string name;
  const bool& should_close = m_should_close;
  const int& height = m_height;
  const int& width = m_width;

  Window(const std::string& name, int default_width = 1024, int default_height = 768, GLFWmonitor* monitor = nullptr)
      : name(name), m_width(default_width), m_height(default_height) {
    if (instances == 0) {
      glfwSetErrorCallback(glfw_error_callback);
      glfwInit();
    }
    m_glfw_window = glfwCreateWindow(default_width, default_height, name.c_str(), monitor, nullptr);
    glfwSetWindowUserPointer(m_glfw_window, this);
    glfwSetWindowSizeCallback(m_glfw_window, glfw_size_callback);
    glfwSetWindowCloseCallback(m_glfw_window, glfw_close_callback);
    instances++;
  }

  void init_imgui() {
    if (!m_imgui_init) {
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGui_ImplGlfw_InitForOpenGL(m_glfw_window, true);
      ImGui_ImplOpenGL3_Init();
      m_imgui_init = true;
    }
  }

  void imgui_new_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
  }

  void swap_buffers() { glfwSwapBuffers(m_glfw_window); }

  void make_current() {
    glfwMakeContextCurrent(m_glfw_window);
    if (!m_glew_init) {
      if (GLenum err = glewInit() != GLEW_OK) {
        throw GLEWError(err);
      };
      m_glew_init = true;
    }
  }

  ~Window() {
    if (m_imgui_init) {
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
    }
    glfwDestroyWindow(m_glfw_window);
    if (--instances == 0) {
      glfwTerminate();
    }
  }
};

int Window::instances = 0;
