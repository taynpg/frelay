cmake_minimum_required(VERSION 3.16)

project(Util LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_library(Util STATIC Util.h Util.cpp LocalFile.h LocalFile.cpp)
target_link_libraries(Util PRIVATE Qt${QT_VERSION_MAJOR}::Core Struct)
target_include_directories(Util PUBLIC ${CMAKE_CURRENT_LIST_DIR})
