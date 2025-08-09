// When built with vcpkg (fmt available), prefer fmt; otherwise fallback to iostream.
#include <iostream>

#if VOCEVERSA_HAS_FMT
  #include <fmt/core.h>
#endif

int main() {
#if VOCEVERSA_HAS_FMT
    fmt::print("Hello, Voceversa with fmt!\n");
#else
    std::cout << "Hello, Voceversa!\n";
#endif
    return 0;
}
