#!/bin/bash

# 设置字符编码
export LANG=en_US.UTF-8

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}/zlib-1.3.2"
BUILD_BASE="${SCRIPT_DIR}/zlib-1.3.2"

if [ ! -d "${SOURCE_DIR}" ]; then
    echo "Error: Source directory not found: ${SOURCE_DIR}"
    read -p "Press any key to exit..."
    exit 1
fi

# 设置安装目录
INSTALL_DIR="${SCRIPT_DIR}/binary/common/zlib"

echo "========================================"
echo "Building zlib Library"
echo "========================================"
echo "Source:      ${SOURCE_DIR}"
echo "Install to:  ${INSTALL_DIR}"
echo ""

# --------------------------------------------------
# Debug Build
# --------------------------------------------------
echo "========================================"
echo "Building DEBUG configuration"
echo "========================================"

DEBUG_BUILD_DIR="${BUILD_BASE}/debug-build"

echo "Creating debug build directory..."
if [ -d "${DEBUG_BUILD_DIR}" ]; then
    echo "Removing existing debug build directory..."
    rm -rf "${DEBUG_BUILD_DIR}"
fi
mkdir -p "${DEBUG_BUILD_DIR}"

echo "Configuring debug build..."
cmake -B"${DEBUG_BUILD_DIR}" -S"${SOURCE_DIR}" \
    -DZLIB_BUILD_SHARED=OFF \
    -DZLIB_BUILD_STATIC=ON \
    -DZLIB_BUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DCMAKE_DEBUG_POSTFIX=d \
    -Wno-dev

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed for debug build"
    read -p "Press any key to exit..."
    exit 1
fi

echo "Building debug library..."
cmake --build "${DEBUG_BUILD_DIR}" --config Debug

if [ $? -ne 0 ]; then
    echo "Error: Build failed for debug configuration"
    read -p "Press any key to exit..."
    exit 1
fi

echo "Installing debug library..."
cmake --install "${DEBUG_BUILD_DIR}" --config Debug

if [ $? -ne 0 ]; then
    echo "Error: Install failed for debug configuration"
    read -p "Press any key to exit..."
    exit 1
fi

echo "Debug build completed successfully"
echo ""

# --------------------------------------------------
# Release Build
# --------------------------------------------------
echo "========================================"
echo "Building RELEASE configuration"
echo "========================================"

RELEASE_BUILD_DIR="${BUILD_BASE}/release-build"

echo "Creating release build directory..."
if [ -d "${RELEASE_BUILD_DIR}" ]; then
    echo "Removing existing release build directory..."
    rm -rf "${RELEASE_BUILD_DIR}"
fi
mkdir -p "${RELEASE_BUILD_DIR}"

echo "Configuring release build..."
cmake -B"${RELEASE_BUILD_DIR}" -S"${SOURCE_DIR}" \
    -DZLIB_BUILD_SHARED=OFF \
    -DZLIB_BUILD_STATIC=ON \
    -DZLIB_BUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -Wno-dev

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed for release build"
    read -p "Press any key to exit..."
    exit 1
fi

echo "Building release library..."
cmake --build "${RELEASE_BUILD_DIR}" --config Release

if [ $? -ne 0 ]; then
    echo "Error: Build failed for release configuration"
    read -p "Press any key to exit..."
    exit 1
fi

echo "Installing release library..."
cmake --install "${RELEASE_BUILD_DIR}" --config Release

if [ $? -ne 0 ]; then
    echo "Error: Install failed for release configuration"
    read -p "Press any key to exit..."
    exit 1
fi

echo "Release build completed successfully"
echo ""

echo "========================================"
echo "All builds completed successfully!"
echo "Libraries installed to: ${INSTALL_DIR}"
echo "========================================"

