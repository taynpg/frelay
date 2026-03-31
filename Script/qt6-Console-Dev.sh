#!/bin/bash
cmake -B../build -S../ -DCMAKE_BUILD_TYPE=Release -DQT_DEFAULT_MAJOR_VERSION=5
cmake --build ../build --config Release