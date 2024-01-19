cmake_minimum_required(VERSION 3.25)
project(etceterapp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O0 -DFORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS -fasynchronous-unwind-tables -fexceptions -fstack-clash-protection -fstack-protector-all -fstack-protector-strong -grecord-gcc-switches -fcf-protection -pipe -Werror=format-security")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

find_package(argparse REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Catch2 REQUIRED)
find_package(pugixml REQUIRED)

# add version information
find_package(Git)
if (GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD OUTPUT_VARIABLE VERSION_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE VERSION_GIT_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${GIT_EXECUTABLE} branch --show-current OUTPUT_VARIABLE VERSION_GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags OUTPUT_VARIABLE VERSION_GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
  set(VERSION_GIT "unknown")
  set(VERSION_GIT_SHORT "unknown")
  set(VERSION_GIT_BRANCH "unknown")
  set(VERSION_GIT_TAG "unknown")
endif ()

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/version.h)

add_library(etceterapp INTERFACE)
target_link_libraries(etceterapp INTERFACE argparse::argparse spdlog::spdlog fmt::fmt pugixml::pugixml)
target_include_directories(etceterapp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(tests
  tests/basic_tests.cpp
  tests/conditional_tests.cpp
  tests/number_tests.cpp
  tests/special_tests.cpp
  tests/string_tests.cpp
  tests/offset_tests.cpp
  )
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain etceterapp)

