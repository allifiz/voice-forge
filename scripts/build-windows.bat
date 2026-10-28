@echo off
setlocal
cd /d "%~dp0.."
where cmake >nul 2>&1
if errorlevel 1 (
  echo CMake not found. Install CMake or add Visual Studio CMake to PATH.
  exit /b 1
)
where git >nul 2>&1
if errorlevel 1 (
  echo Git not found. Install Git for Windows and reopen your terminal.
  exit /b 1
)
cmake -S . -B build -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
echo.
echo Build OK: build\Release\voiceforge.exe --list-devices
endlocal
