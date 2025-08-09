# Minimal fmt stub to satisfy target fmt::fmt when package not available
add_library(fmt INTERFACE)
add_library(fmt::fmt ALIAS fmt)
# Optionally expose include directory if later needed
# target_include_directories(fmt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
