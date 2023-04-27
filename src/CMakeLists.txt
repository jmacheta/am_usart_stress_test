cmake_minimum_required(VERSION 3.24)

include(FetchContent)

FetchContent_Declare(termcolor GIT_REPOSITORY https://github.com/ikalnytskyi/termcolor.git GIT_TAG v2.1.0)
FetchContent_MakeAvailable(termcolor)

add_executable(usart_stress_test main.cpp)
target_include_directories(usart_stress_test PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(usart_stress_test PRIVATE cxx_std_20 c_std_11)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message("Build for Windows")
  target_sources(usart_stress_test PRIVATE serial_windows.cpp)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message("Build for Linux")
  target_sources(usart_stress_test PRIVATE serial_linux.cpp)
else ()
  message(FATAL_ERROR "Target platform \"${CMAKE_SYSTEM_NAME}\" not supported")
endif ()

target_link_libraries(usart_stress_test PRIVATE termcolor)
target_link_options(usart_stress_test PRIVATE -static)
