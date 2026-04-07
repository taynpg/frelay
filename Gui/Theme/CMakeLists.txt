cmake_minimum_required(VERSION 3.16)

project(frelayTheme LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg)

set(PROJECT_SOURCES frelayTheme.h frelayTheme.cpp)

add_library(frelayTheme SHARED ${PROJECT_SOURCES})
target_compile_definitions(frelayTheme PRIVATE THEME_EXPORT)
target_link_libraries(frelayTheme PUBLIC Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg qlementine)
target_include_directories(frelayTheme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS frelayTheme
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
)