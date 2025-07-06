@echo on

set QT_DIR=C:/Qt/6.8.3
cd ..
cmake -Bbuild-qt6 -S. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% -DQT_DEFAULT_MAJOR_VERSION=6 -DCOMPILE_GUI=ON
cmake --build build-qt6 --config Release
cd build-qt6
cpack
pause