cmake_minimum_required(VERSION 3.16)

project(Struct LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(SOURCES
InfoClient.h
InfoClient.cpp
InfoPack.hpp
InfoDirFile.h
InfoDirFile.cpp
InfoMsg.h
InfoMsg.cpp
)

add_library(Struct STATIC ${SOURCES})
target_link_libraries(Struct Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(Struct PUBLIC ${CMAKE_CURRENT_LIST_DIR})
