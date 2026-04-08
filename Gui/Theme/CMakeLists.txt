cmake_minimum_required(VERSION 3.16)

project(externTheme LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg)

set(PROJECT_SOURCES externTheme.h externTheme.cpp)

add_library(externTheme SHARED ${PROJECT_SOURCES})
target_compile_definitions(externTheme PRIVATE THEME_EXPORT)
target_link_libraries(externTheme PUBLIC Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg qlementine)
target_include_directories(externTheme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS externTheme
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
)