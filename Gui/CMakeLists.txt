cmake_minimum_required(VERSION 3.16)

project(frelayGUI VERSION ${PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUSE_QT_GUI)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Concurrent)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_SOURCES main.cpp
frelayGUI.cpp frelayGUI.h frelayGUI.ui
Control/LogControl.h Control/LogControl.cpp Control/LogControl.ui
Control/FileControl.h Control/FileControl.cpp Control/FileControl.ui
Control/ConnectControl.h Control/ConnectControl.cpp Control/ConnectControl.ui
Control/CompareControl.h Control/CompareControl.cpp Control/CompareControl.ui
GuiUtil/Public.h GuiUtil/Public.cpp
Form/Transform.h Form/Transform.cpp Form/Transform.ui
../Res/frelay.qrc ../Res/ico.rc ../Res/QtTheme.qrc ../Res/qss.qrc
Control/cusTableWidget.cpp Control/cusTableWidget.h
Control/cpTableWidget.h Control/cpTableWidget.cpp
GuiUtil/Config.h GuiUtil/Config.cpp
Form/Loading.h Form/Loading.cpp
Control/Common.h Control/Common.cpp
Form/FileInfoForm.h Form/FileInfoForm.cpp Form/FileInfoForm.ui
Form/Notice.h Form/Notice.cpp Form/Notice.ui
Form/settings.cpp Form/settings.ui Form/settings.h
)

add_executable(frelayGUI WIN32 ${PROJECT_SOURCES})
target_link_libraries(frelayGUI PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    ClientCore Protocol 
    Util Struct SingleApplication::SingleApplication
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(frelayGUI PRIVATE ws2_32 wsock32)
endif()
