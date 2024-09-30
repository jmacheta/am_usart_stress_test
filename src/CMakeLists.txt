cmake_minimum_required(VERSION 3.24)

add_compile_options(-Os)

include(FetchContent)

FetchContent_Declare(fmt URL https://github.com/fmtlib/fmt/archive/refs/tags/11.0.2.zip)
FetchContent_Declare(argparse URL https://github.com/p-ranav/argparse/archive/refs/tags/v3.1.zip)

FetchContent_MakeAvailable(fmt argparse)

add_executable(usart_stress_test main.cpp)
target_include_directories(usart_stress_test PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(usart_stress_test PRIVATE cxx_std_20 c_std_11)
target_compile_definitions(usart_stress_test PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_compile_options(
  usart_stress_test
  PRIVATE -Wall
          -Wextra
          -pedantic
          -Werror
          -Wno-unused-parameter
          -Wno-unused-variable
          -Wno-unused-function
          -Wno-unused-but-set-variable
          -Wno-unused-but-set-parameter
          -Wno-unused-value
          -Wno-unused-result
          -Wno-unused-label
          -Wno-unused-local-typedefs
          -Wno-unused-macros
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message("Build for Windows")
  target_sources(usart_stress_test PRIVATE serial_windows.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message("Build for Linux")
  target_sources(usart_stress_test PRIVATE serial_linux.cpp)
else ()
  message(FATAL_ERROR "Target platform \"${CMAKE_SYSTEM_NAME}\" not supported")
endif ()

target_link_libraries(usart_stress_test PRIVATE fmt::fmt argparse::argparse)
target_link_options(usart_stress_test PRIVATE -static)
