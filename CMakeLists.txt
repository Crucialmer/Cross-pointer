cmake_minimum_required(VERSION 3.24)
project(CrossPointer VERSION 1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT MSVC)
  message(FATAL_ERROR "This project strictly requires the MSVC compiler.")
endif()

# Set output path to project source directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# Static runtime for Release (/MT), dynamic for Debug (/MDd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

add_executable(
  CrossPointer WIN32
  main.cpp
  src/config_file_util.cpp
  src/crosshair.cpp
  src/config.cpp
  src/hotkey.cpp
  src/tray.cpp
  src/autostart.cpp
  src/settings_window.cpp
  src/updater.cpp
  res/resource.rc
  res/CrossPointer.manifest
)

# Software output name strictly matches project name
set_target_properties(CrossPointer PROPERTIES
  OUTPUT_NAME "CrossPointer"
)

target_compile_definitions(CrossPointer PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

target_compile_options(CrossPointer PRIVATE
  /utf-8 /W4 /permissive- /EHsc
  $<$<CONFIG:Debug>:/Od /Zi>
  $<$<CONFIG:Release>:/GL>
)

target_link_options(CrossPointer PRIVATE
  $<$<CONFIG:Debug>:/DEBUG>
  $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
)

# Link Windows system libraries
target_link_libraries(CrossPointer PRIVATE
  user32
  gdi32
  d2d1
  winmm
  shell32
  advapi32
  uxtheme
  dwmapi
  comctl32
  comdlg32
  wininet
)
