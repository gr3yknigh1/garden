cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(garden
  VERSION 0.0.0
  LANGUAGES C CXX
)


set(GLM_BUILD_TESTS OFF)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(glm)

add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp
  imgui/backends/imgui_impl_win32.cpp
  imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC imgui imgui/backends imgui/misc/cpp)

add_library(glad STATIC
  glad/glad.c
  glad/glad_wgl.c
)

target_include_directories(glad PUBLIC glad)

#
# Runtime:
#

add_executable(garden WIN32
  code/garden_runtime.cpp
)

target_link_libraries(garden PRIVATE
  glm glad imgui
  kernel32.lib user32.lib gdi32.lib
)

target_compile_definitions(garden PRIVATE
  GARDEN_GAMEPLAY_DLL_NAME=garden_gameplay.dll
  GARDEN_ASSETS_FOLDER="${PROJECT_SOURCE_DIR}/assets" # TODO(gr3yknigh1): Expose to the tasks.py level [2025/05/02] #buildsystem
  _CRT_SECURE_NO_WARNINGS=1
)

target_compile_features(garden PRIVATE cxx_std_20)

#
# Gameplay:
#

add_library(garden_gameplay SHARED
  code/garden_runtime.cpp
)

target_link_libraries(garden_gameplay PRIVATE
  glm glad imgui
  kernel32.lib user32.lib gdi32.lib
)
target_compile_definitions(garden_gameplay PRIVATE
  GARDEN_GAMEPLAY_CODE=1
  GARDEN_GAMEPLAY_DLL_NAME=garden_gameplay.dll
  _CRT_SECURE_NO_WARNINGS=1
)

target_compile_features(garden_gameplay PRIVATE cxx_std_20)
