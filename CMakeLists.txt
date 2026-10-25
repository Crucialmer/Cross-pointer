cmake_minimum_required(VERSION 3.24)
project(Cross-pointer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT MSVC)
  message(FATAL_ERROR "This project strictly requires the MSVC compiler.")
endif()

# EXECUTABLE_OUTPUT_PATH
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

# 闈欐€佺紪璇戜粎閽堝 Release (/MT)锛孌ebug 淇濇寔榛樿鍔ㄦ€侀摼鎺?(/MDd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

add_executable(
  Cross-pointer WIN32 main.cpp src/config_file_util.cpp src/crosshair.cpp
  src/config.cpp src/hotkey.cpp res/resource.rc)

# 杞欢鍚嶄弗鏍奸噰鐢ㄩ」鐩悕锛屼笉鍔犱换浣曞叾浠栨爣璁?set_target_properties(Cross-pointer PROPERTIES
  OUTPUT_NAME "Cross-pointer"
)

target_compile_definitions(Cross-pointer PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
)

target_compile_options(Cross-pointer PRIVATE
  /utf-8 /W4 /permissive- /EHsc
  $<$<CONFIG:Debug>:/Od /Zi>
  $<$<CONFIG:Release>:/GL>
)

target_link_options(Cross-pointer PRIVATE
  $<$<CONFIG:Debug>:/DEBUG>
  $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
)

# 閾炬帴 Windows 绯荤粺搴?target_link_libraries(Cross-pointer PRIVATE user32 gdi32 d2d1)