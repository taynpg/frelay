#!/bin/bash
current_user=$(whoami)
qt_path="/home/${current_user}/Qt5.14.2/5.14.2/gcc_64"

if [ -d "$qt_path" ]; then
    echo "Found Qt directory: $qt_path"

    cmake -B../build -S../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$qt_path" -DQT_DEFAULT_MAJOR_VERSION=5 -DCOMPILE_GUI=ON
    cmake --build ../build --config Release
    
    if [ $? -eq 0 ]; then
        echo "cmake command executed successfully"
    else
        echo "cmake command failed"
        exit 1
    fi
else
    echo "Error: Qt directory not found - $qt_path"
    echo "Please ensure Qt5.14.2 is properly installed in the default location"
    exit 1
fi