@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SOURCE_DIR=%SCRIPT_DIR%zlib-1.3.2"
set "BUILD_BASE=%SCRIPT_DIR%zlib-1.3.2"

if not exist "%SOURCE_DIR%" (
    echo Error: Source directory not found: %SOURCE_DIR%
    pause
    exit /b 1
)

REM Set installation directory (relative to script)
set "INSTALL_DIR=%SCRIPT_DIR%binary\mingw64\zlib"

echo ========================================
echo Building zlib Library
echo ========================================
echo Source:      %SOURCE_DIR%
echo Install to:  %INSTALL_DIR%
echo.

REM --------------------------------------------------
REM Debug Build
REM --------------------------------------------------
echo ========================================
echo Building DEBUG configuration
echo ========================================

set "DEBUG_BUILD_DIR=%BUILD_BASE%\debug-build-mingw64"

echo Creating debug build directory...
if exist "%DEBUG_BUILD_DIR%" (
    echo Removing existing debug build directory...
    rmdir /s /q "%DEBUG_BUILD_DIR%"
)
mkdir "%DEBUG_BUILD_DIR%"

echo Configuring debug build...
cmake -B"%DEBUG_BUILD_DIR%" -S"%SOURCE_DIR%" ^
    -G "MinGW Makefiles" ^
    -DZLIB_BUILD_SHARED=OFF ^
    -DZLIB_BUILD_STATIC=ON ^
    -DZLIB_BUILD_TESTING=OFF ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DCMAKE_DEBUG_POSTFIX=d ^
    -Wno-dev

if errorlevel 1 (
    echo Error: CMake configuration failed for debug build
    pause
    exit /b 1
)

echo Building debug library...
cmake --build "%DEBUG_BUILD_DIR%" --config Debug

if errorlevel 1 (
    echo Error: Build failed for debug configuration
    pause
    exit /b 1
)

echo Installing debug library...
cmake --install "%DEBUG_BUILD_DIR%" --config Debug

if errorlevel 1 (
    echo Error: Install failed for debug configuration
    pause
    exit /b 1
)

echo Debug build completed successfully
echo.

REM --------------------------------------------------
REM Release Build
REM --------------------------------------------------
echo ========================================
echo Building RELEASE configuration
echo ========================================

set "RELEASE_BUILD_DIR=%BUILD_BASE%\release-build-mingw64"

echo Creating release build directory...
if exist "%RELEASE_BUILD_DIR%" (
    echo Removing existing release build directory...
    rmdir /s /q "%RELEASE_BUILD_DIR%"
)
mkdir "%RELEASE_BUILD_DIR%"

echo Configuring release build...
cmake -B"%RELEASE_BUILD_DIR%" -S"%SOURCE_DIR%" ^
    -G "MinGW Makefiles" ^
    -DZLIB_BUILD_SHARED=OFF ^
    -DZLIB_BUILD_STATIC=ON ^
    -DZLIB_BUILD_TESTING=OFF ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -Wno-dev

if errorlevel 1 (
    echo Error: CMake configuration failed for release build
    pause
    exit /b 1
)

echo Building release library...
cmake --build "%RELEASE_BUILD_DIR%" --config Release

if errorlevel 1 (
    echo Error: Build failed for release configuration
    pause
    exit /b 1
)

echo Installing release library...
cmake --install "%RELEASE_BUILD_DIR%" --config Release

if errorlevel 1 (
    echo Error: Install failed for release configuration
    pause
    exit /b 1
)

echo Release build completed successfully
echo.

echo ========================================
echo All builds completed successfully!
echo Libraries installed to: %INSTALL_DIR%
echo ========================================

pause