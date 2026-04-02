cmake_minimum_required(VERSION 3.16)

project(frelayServer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

add_library(Server STATIC Server.cpp Server.h)
target_include_directories(Server PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(Server PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Protocol Util)

add_executable(frelayServer main.cpp ../Res/server.rc)
target_link_libraries(frelayServer PRIVATE Server)
