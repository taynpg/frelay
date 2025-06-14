cmake_minimum_required(VERSION 3.16)

project(frelayTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MSOURCES
msgTest.h msgTest.cpp
protocolTest.cpp infoTest.h infoTest.cpp
)

add_executable(frelayTest ${MSOURCES})
target_link_libraries(frelayTest PRIVATE Protocol Util)