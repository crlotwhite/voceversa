@echo off
setlocal enabledelayedexpansion
set PRESET=%1
if "%PRESET%"=="" set PRESET=debug-vs

set CTEST_CONFIG=
if /I "%PRESET%"=="debug-vs" set CTEST_CONFIG=-C Debug

cmake --preset %PRESET%
cmake --build --preset %PRESET% --parallel
ctest --preset %PRESET% %CTEST_CONFIG% --output-on-failure
