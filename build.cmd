@echo off
set CMAKE_GEN="Visual Studio 15 2017 Win64"
set BUILD_DIR="%~dp0\cmake_build"
set BIN_DIR="%~dp0\bin"

:: BUILD: generate project and build
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%
cmake "%~dp0\awe_app" -G %CMAKE_GEN%
cmake --build . --config Release
::cmake --open .
popd

:: INSTALL: copy bin files
if not exist %BIN_DIR% mkdir %BIN_DIR%
robocopy %BUILD_DIR%\awe_try\Release %BIN_DIR% *.exe
