cmake_minimum_required(VERSION 3.3)
project(jack-analyer)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(glfw3 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)

add_subdirectory(lib)

add_executable(main main.cpp)
target_link_libraries(main gloo glfw imgui PkgConfig::JACK)
target_include_directories(main PRIVATE include)
target_compile_options(main PRIVATE -Wall -Wextra -pedantic)
