cmake_minimum_required(VERSION 3.16)

project(Protocol LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(SOURCES
Protocol.cxx
Protocol.h
InfoClient.h
InfoClient.cpp
InfoPack.hpp
)

add_library(Protocol STATIC ${SOURCES})
target_link_libraries(Protocol Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(Protocol PUBLIC ${CMAKE_CURRENT_LIST_DIR})
