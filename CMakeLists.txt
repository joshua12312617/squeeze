cmake_minimum_required(VERSION 3.16)
project(squeeze VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(squeeze_lib
  src/bitio.cpp
  src/rle.cpp
  src/huffman.cpp
  src/lz77.cpp
  src/mtf.cpp
  src/bwt.cpp
  src/pipeline.cpp
)
target_include_directories(squeeze_lib PUBLIC src)
target_compile_options(squeeze_lib PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_executable(squeeze src/main.cpp)
target_link_libraries(squeeze PRIVATE squeeze_lib)

option(SQUEEZE_BUILD_TESTS "Build the test suite" ON)
if(SQUEEZE_BUILD_TESTS)
  enable_testing()
  add_executable(squeeze_tests tests/test_main.cpp)
  target_link_libraries(squeeze_tests PRIVATE squeeze_lib)
  target_compile_options(squeeze_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
  )
  add_test(NAME unit COMMAND squeeze_tests)
endif()
