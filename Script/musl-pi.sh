cmake -B../build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/yun/local/musl-Qt6 -S../ -DRELEASE_MARK=OFF -DQT_DEFAULT_MAJOR_VERSION=6
cmake --build ../build --config Release