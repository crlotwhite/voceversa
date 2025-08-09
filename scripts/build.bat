@echo off
setlocal enabledelayedexpansion
set PRESET=%1
if "%PRESET%"=="" set PRESET=debug-vs

cmake --preset %PRESET%
cmake --build --preset %PRESET% -m
ctest --preset %PRESET% --output-on-failure
