cmake -B../build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/data/data/com.termux/files/usr/lib -S../ -DRELEASE_MARK=ON
cmake --build ../build --config Release