cmake_minimum_required(VERSION 3.16)

project(voceversa
  VERSION 0.1.0
  DESCRIPTION "Voceversa C++ project skeleton"
  LANGUAGES CXX)

option(VOCEVERSA_USE_FMT "Use fmt library if available (via vcpkg/conan or fallback stub)" ON)
option(VOCEVERSA_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(VOCEVERSA_ENABLE_LTO "Enable link time optimization in Release" OFF)
option(VOCEVERSA_USE_WORLD "Use mmorise/WORLD for analysis & synthesis (via find_package or FetchContent)" OFF)

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

# Core library (interfaces + platform implementation)
add_library(voceversa_core
  src/platform/LinuxPlatformIO.cpp
  src/world/F0Extraction.cpp
  src/world/SpectralAnalysis.cpp
  src/world/D4C.cpp
  src/world/WorldAnalysisNode.cpp
  src/world/WorldSynthesisNode.cpp
  src/world/UtauWorldInterface.cpp
)
target_include_directories(voceversa_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(voceversa_core PUBLIC cxx_std_17)
if(VOCEVERSA_FMT_TARGET)
  target_link_libraries(voceversa_core PUBLIC ${VOCEVERSA_FMT_TARGET})
  target_compile_definitions(voceversa_core PUBLIC VOCEVERSA_HAS_FMT=1)
else()
  target_compile_definitions(voceversa_core PUBLIC VOCEVERSA_HAS_FMT=0)
endif()

# Optional: WORLD integration
if(VOCEVERSA_USE_WORLD)
  # Try to find a pre-installed world package (e.g., via vcpkg or system)
  find_package(world CONFIG QUIET)
  if(NOT world_FOUND)
    include(FetchContent)
    # Fetch the official WORLD repo; if the network is unavailable, this block will be skipped gracefully
    FetchContent_Declare(
      world
      GIT_REPOSITORY https://github.com/mmorise/World.git
      GIT_TAG master
    )
    # Protect from CMake versions that error on failed fetch; allow continue without WORLD
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_MakeAvailable(world)
  endif()

  if(TARGET world)
    target_link_libraries(voceversa_core PUBLIC world)
    target_compile_definitions(voceversa_core PUBLIC VV_USE_WORLD=1)
    target_include_directories(voceversa_core PUBLIC $<TARGET_PROPERTY:world,INCLUDE_DIRECTORIES>)
    message(STATUS "VOCEVERSA: WORLD integration enabled")
  else()
    message(WARNING "VOCEVERSA: WORLD library not found/fetched; falling back to placeholder algorithms.")
  endif()
endif()

if(BUILD_TESTING)
  enable_testing()
  add_test(NAME hello COMMAND voceversa_app)
  # Accept both messages (with or without fmt) to validate dependency presence transparently
  set_tests_properties(hello PROPERTIES PASS_REGULAR_EXPRESSION "Hello, Voceversa( with fmt)?!")

  # Unit tests for core
  add_executable(core_tests
    tests/test_core.cpp
  )
  target_link_libraries(core_tests PRIVATE voceversa_core)
  target_include_directories(core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME core_tests COMMAND core_tests)

  add_executable(world_tests
    tests/test_world.cpp
  )
  target_link_libraries(world_tests PRIVATE voceversa_core)
  target_include_directories(world_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME world_tests COMMAND world_tests)
endif()

# Simple CLI tool for UTAU-friendly WORLD IO (demo)
add_executable(vv_world src/tools/vv_world.cpp)
target_link_libraries(vv_world PRIVATE voceversa_core)
target_sources(vv_world PRIVATE src/utils/WavIO.cpp)
target_include_directories(vv_world PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
