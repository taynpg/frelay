@echo on

set QT_DIR=C:/local/Qt6
cd ..
cmake -Bbuild-qt6-clang -S. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% -DQT_DEFAULT_MAJOR_VERSION=6 -DCOMPILE_GUI=ON -DRELEASE_MARK=ON
cmake --build build-qt6-clang --config Release
cd build-qt6-clang
cpack
pause