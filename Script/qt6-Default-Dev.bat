@echo on

set QT_DIR=C:/local/Qt6
cd ..
cmake -Bbuild-qt6-dev -S. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% -DQT_DEFAULT_MAJOR_VERSION=6 -DCOMPILE_GUI=ON -DQT_ROOT_PATH=%QT_DIR%
cmake --build build-qt6-dev --config Release
cd build-qt6-dev
cpack
pause