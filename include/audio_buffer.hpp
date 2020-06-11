#pragma once
#include <chrono>
#include <iostream>
#include <list>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <jack/jack.h>

class AudioBuffer {
public:
  jack_client_t* const& jack_client = m_jack_client;
  const int& sample_rate = m_sample_rate;
  const int& buffer_size = m_buffer_size;
  const std::list<std::vector<float>>& buffers = m_buffers;
  const std::string input_port_name = "in";
  const std::string client_name = "jack analyser";

  AudioBuffer(const std::string& default_connection) {
    jack_status_t jack_status;
    jack_options_t jack_options = JackOptions::JackNoStartServer;
    m_jack_client = jack_client_open(client_name.c_str(), jack_options, &jack_status);
    if (m_jack_client == NULL) {
      throw std::runtime_error("Failed to initialize Jack Client");
    }
    jack_set_buffer_size_callback(
        m_jack_client,
        [](jack_nframes_t nframes, void* self) {
          static_cast<AudioBuffer*>(self)->m_buffer_size = nframes;
          return 0;
        },
        this);
    jack_set_sample_rate_callback(
        m_jack_client,
        [](jack_nframes_t nframes, void* self) {
          static_cast<AudioBuffer*>(self)->m_sample_rate = nframes;
          return 0;
        },
        this);
    jack_set_process_callback(m_jack_client, process, this);

    if (jack_activate(m_jack_client) != 0) {
      throw std::runtime_error("failed to activate jack client");
    }

    unsigned long port_flags = JackPortFlags::JackPortIsInput | JackPortFlags::JackPortIsTerminal;
    in_port = jack_port_register(m_jack_client, input_port_name.c_str(), JACK_DEFAULT_AUDIO_TYPE, port_flags, 0);
    try {
      connect_to(default_connection);
    } catch (std::runtime_error e) {
      std::cerr << e.what() << std::endl;
    }
  }

  static int process(uint32_t nframes, void* arg) {
    AudioBuffer* self = static_cast<AudioBuffer*>(arg);
    // std::lock_guard<std::mutex> guard(self->m_buffer_mutex);
    if (self->in_port != 0) {
      float* buffer_ptr = static_cast<float*>(jack_port_get_buffer(self->in_port, nframes));
      std::vector<float> buffer(buffer_ptr, buffer_ptr + nframes);
      self->m_buffers.push_back(buffer);
    }
    return 0;
  }

  ~AudioBuffer() {
    jack_deactivate(m_jack_client);
    jack_client_close(m_jack_client);
  }

  void connect_to(const std::string& port) {
    int status = jack_connect(m_jack_client, port.c_str(), (client_name + ":" + input_port_name).c_str());
    if (status != 0) {
      throw std::runtime_error("couldn't connect to " + port + " error code: " + std::to_string(status));
    }
  }

  std::vector<float> pop() {
    // std::lock_guard<std::mutex> guard(m_buffer_mutex);
    if (m_buffers.size() == 0) {
      throw std::runtime_error("overrun");
    }
    std::vector<float> buffer = m_buffers.front(); // this fails sometimes with bad_alloc
    m_buffers.pop_front();
    return buffer;
  }

  void clear() {
    // std::lock_guard<std::mutex> guard(m_buffer_mutex);
    while (m_buffers.size() != 0) {
      m_buffers.pop_front();
    }
  }

private:
  int m_sample_rate;
  int m_buffer_size;
  jack_client_t* m_jack_client;
  jack_port_t* in_port;
  std::list<std::vector<float>> m_buffers;
  // std::mutex m_buffer_mutex;
};
