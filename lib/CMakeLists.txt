set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_EXAMPLES_DIR ${IMGUI_DIR}/examples)
set(IMGUI_INCLUDES ${IMGUI_DIR} ${IMGUI_EXAMPLES_DIR})
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_EXAMPLES_DIR}/imgui_impl_glfw.cpp
  ${IMGUI_EXAMPLES_DIR}/imgui_impl_opengl3.cpp)

add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDES})

add_subdirectory(gloo)
