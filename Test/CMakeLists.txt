cmake_minimum_required(VERSION 3.16)

project(frelayTest LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(MSOURCES
msgTest.h msgTest.cpp
protocolTest.cpp infoTest.h infoTest.cpp
)

add_executable(frelayTest ${MSOURCES})
target_link_libraries(frelayTest PRIVATE Protocol Util)

add_executable(frelayBaseTest BaseTest.cpp)
target_link_libraries(frelayBaseTest Qt${QT_VERSION_MAJOR}::Core)
