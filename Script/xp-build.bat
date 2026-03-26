@echo on

set QT_DIR=C:/Qt/Qt5.7.1/5.7/mingw53_32
set COMPILE_DIR=C:/Qt/Qt5.7.1/Tools/mingw530_32
set PATH=%COMPILE_DIR%/bin;%PATH%

cd ..
cmake -Bbuild-xp -S. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% -DQT_DEFAULT_MAJOR_VERSION=5 -DXP_PLATFORM_SUPPORT=ON -DCOMPILE_GUI=ON -DRELEASE_MARK=ON
cmake --build build-xp --config Release
cd build-xp
cpack
pause