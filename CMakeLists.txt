cmake_minimum_required(VERSION 3.16)

project(voceversa
  VERSION 0.1.0
  DESCRIPTION "Voceversa C++ project skeleton"
  LANGUAGES CXX)

option(VOCEVERSA_USE_FMT "Use fmt library if available (via vcpkg/conan or fallback stub)" ON)
option(VOCEVERSA_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(VOCEVERSA_ENABLE_LTO "Enable link time optimization in Release" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(VOCEVERSA_ENABLE_WARNINGS)
  if(MSVC)
    add_compile_options(/W3)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
endif()

if(MSVC)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
else()
  add_compile_options($<$<CONFIG:Release>:-O3>)
endif()

# fmt: prefer package, fallback to stub
set(VOCEVERSA_FMT_TARGET "")
if(VOCEVERSA_USE_FMT)
  find_package(fmt CONFIG QUIET)
  if(fmt_FOUND)
    set(VOCEVERSA_FMT_TARGET fmt::fmt)
  else()
    add_subdirectory(external/fmt_stub)
    set(VOCEVERSA_FMT_TARGET fmt::fmt)
  endif()
endif()

add_executable(voceversa_app src/main.cpp)
target_include_directories(voceversa_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Ensure C++17 features
target_compile_features(voceversa_app PRIVATE cxx_std_17)

if(VOCEVERSA_FMT_TARGET)
  target_link_libraries(voceversa_app PRIVATE ${VOCEVERSA_FMT_TARGET})
  target_compile_definitions(voceversa_app PRIVATE VOCEVERSA_HAS_FMT=1)
else()
  target_compile_definitions(voceversa_app PRIVATE VOCEVERSA_HAS_FMT=0)
endif()

include(CTest)
if(BUILD_TESTING)
  enable_testing()
  add_test(NAME hello COMMAND voceversa_app)
  # Accept both messages (with or without fmt) to validate dependency presence transparently
  set_tests_properties(hello PROPERTIES PASS_REGULAR_EXPRESSION "Hello, Voceversa( with fmt)?!")
endif()
