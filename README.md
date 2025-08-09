# Voceversa C++ Project

Cross-platform CMake C++17 skeleton with basic test via CTest.

## Build (macOS/Linux)

```bash
./scripts/build.sh debug
```

or manually:

```bash
cmake --preset debug
cmake --build --preset debug -j
ctest --preset debug --output-on-failure
```

## Build (Windows)

```bat
scripts\build.bat debug
```

## Notes
- Uses C++17 and enables warnings.
- If `fmt` is available via vcpkg/conan, it will be used; otherwise a local stub target satisfies linking.
- Test checks stdout contains `Hello, Voceversa!`.
