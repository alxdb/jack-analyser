#include <iostream>

#include <audio_buffer.hpp>
#include <gloo.hpp>
#include <window.hpp>

const char* vert_shader = "#version 410\n"
                          "layout(location = 0) in vec2 pos;"
                          "void main() {"
                          "  gl_Position = vec4(pos, 0.0, 1.0);"
                          "}";

const char* frag_shader = "#version 410\n"
                          "layout(location = 0) out vec4 col;"
                          "void main() {"
                          "  col = vec4(1.0, 1.0, 1.0, 1.0);"
                          "}";

void glDebug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
             const void* userParam) {
  if (GL_DEBUG_TYPE_ERROR == type) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
  }
}

using namespace gloo;

class App {

  Window m_window;

public:
  App(const std::string& name) : m_window(name){};

  void run() {
    m_window.make_current();
    m_window.init_imgui();
    AudioBuffer ab("PulseAudio JACK Sink:front-left");

    glfwSwapInterval(1);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebug, nullptr);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("/usr/share/fonts/ttf-iosevka/iosevka-regular.ttf", 24.0f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(1.5);

    struct Param {
      std::string name;
      int val, min, max, step = 1, step_fast = 5;
      void draw() {
        ImGui::InputInt(name.c_str(), &val, step, step_fast);
        if (val > max)
          val = max;
        else if (val < min)
          val = min;
      }
    };

    Param n_buffers = {"n_buffers", 1, 1, 256};

    Program program(Shader(vert_shader, GL_VERTEX_SHADER), Shader(frag_shader, GL_FRAGMENT_SHADER));
    VBO vertex_buffer(GL_ARRAY_BUFFER, (float*)nullptr, (ab.buffer_size * (n_buffers.max + 1)) * 2, GL_STREAM_DRAW);
    VAO vertex_array_object({{vertex_buffer, 0, GL_FLOAT, 2}});
    glUseProgram(program.get_id());

    while (!m_window.should_close) {
      m_window.imgui_new_frame();

      ImGui::NewFrame();

      ImGui::SetNextWindowPos({5, 5});
      auto window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
      if (ImGui::Begin("Controls", NULL, window_flags)) {
        ImGui::PushItemWidth(120);
        n_buffers.draw();
      }
      ImGui::End();

      ImGui::SetNextWindowSize({275, 0});
      ImGui::SetNextWindowPos({(float)(m_window.width - 280), 5});
      window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;
      if (ImGui::Begin("Stats", NULL, window_flags)) {
        ImGui::PushItemWidth(120);
        ImGui::Text("stored buffers: %ld", ab.buffers.size());
        ImGui::Text("buffer size: %d", ab.buffer_size);
        ImGui::Text("sample rate: %d", ab.sample_rate);
      }
      ImGui::End();

      ImGui::Render();

      glViewport(0, 0, m_window.width, m_window.height);
      glClear(GL_COLOR_BUFFER_BIT);

      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      float x = -1.0;
      float dx = 2.0 / (n_buffers.val * ab.buffer_size);
      std::vector<std::array<float, 2>> coords;
      for (int i = 0; i <= n_buffers.val; i++) {
        try {
          std::vector<float> buffer = ab.pop();
          for (float sample : buffer) {
            coords.push_back({x, sample});
            x += dx;
          }
        } catch (std::runtime_error e) {
          if (std::string("overrun").compare(e.what()) == 0) {
            for (int i = 0; i < ab.buffer_size; i++) {
              coords.push_back({x, 0.0});
              x += dx;
            }
          } else {
            throw e;
          }
        }
      }
      // ab.clear();
      vertex_buffer.update_data(coords);
      glDrawArrays(GL_LINE_STRIP, 0, n_buffers.val * ab.buffer_size + 1);

      m_window.swap_buffers();
      glfwPollEvents();
    }
  }
};

int main() {
  App app("jack analyser");
  app.run();
  return 0;
}
