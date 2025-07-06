cmake_minimum_required(VERSION 3.16)

project(ClientCore LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

set(SOURCES
ClientCore.cpp
ClientCore.h
RemoteFile.h
RemoteFile.cpp
FileTrans.h 
FileTrans.cpp
)

add_library(ClientCore STATIC ${SOURCES})
target_link_libraries(ClientCore PRIVATE Protocol 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Network
    Struct
    Util
)
target_include_directories(ClientCore PUBLIC ${CMAKE_CURRENT_LIST_DIR})
