@echo off
setlocal EnableExtensions
pushd "%~dp0.." || exit /b 1

rem Prefer CMake on PATH; otherwise use the copy bundled with VS Build Tools.
set "CMAKE_EXE="
where cmake >nul 2>&1
if not errorlevel 1 set "CMAKE_EXE=cmake"

if not defined CMAKE_EXE (
  for %%E in (BuildTools Community Professional Enterprise) do (
    if not defined CMAKE_EXE if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  )
)
if not defined CMAKE_EXE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"

if not defined CMAKE_EXE (
  echo ERROR: CMake not found in PATH, Visual Studio 2022, or Program Files.
  echo Install CMake or check Visual Studio Installer - Individual components.
  popd
  exit /b 1
)
where git >nul 2>&1
if errorlevel 1 (
  echo ERROR: Git for Windows is not in PATH.
  popd
  exit /b 1
)
echo Using CMake: %CMAKE_EXE%
"%CMAKE_EXE%" -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
  popd
  exit /b 1
)
"%CMAKE_EXE%" --build build --config Release
if errorlevel 1 (
  popd
  exit /b 1
)
echo.
echo Build OK: build\Release\voiceforge.exe
popd
endlocal
