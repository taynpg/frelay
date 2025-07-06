#!/bin/bash
cd ..
xmake f -a x64 -m release --qt="/data/data/com.termux/files/usr/lib" --qt5=y -o build-linux -v
xmake

if [ $? -eq 0 ]; then
    echo "xmake command executed successfully"
else
    echo "xmake command failed"
    exit 1
fi