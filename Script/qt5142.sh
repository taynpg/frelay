#!/bin/bash
current_user=$(whoami)
qt_path="/home/${current_user}/Qt5.14.2/5.14.2/gcc_64"
build_dir="build-linux/linux/x64/release"

if [ -d "$qt_path" ]; then

    echo "Found Qt directory: $qt_path"

    cd ..
    xmake f -a x64 -m release --qt="$qt_path" --gui=y --qt5=y -o build-linux -v
    xmake

    if [ $? -eq 0 ]; then
        find "$build_dir" -name "*.a" -delete 2>/dev/null
        find "$build_dir" -name "*Test*" -type f -delete 2>/dev/null
        echo "xmake command executed successfully"
    else
        echo "xmake command failed"
        exit 1
    fi
else
    echo "Error: Qt directory not found - $qt_path"
    echo "Please ensure Qt5.14.2 is properly installed in the default location"
    exit 1
fi