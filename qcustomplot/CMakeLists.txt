cmake_minimum_required(VERSION 3.16)

project(qcusplot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    # MSVC编译器
    add_compile_options(/wd4996)  # 禁用C4996弃用声明警告
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Clang/GCC编译器
    add_compile_options(-Wno-deprecated-declarations)   # 禁用弃用声明警告
    add_compile_options(-Wno-deprecated)                # 禁用所有弃用警告
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core PrintSupport)

add_library(qcusplot SHARED qcustomplot.cpp qcustomplot.h)
target_compile_definitions(qcusplot PRIVATE QCUSTOMPLOT_COMPILE_LIBRARY)
target_include_directories(qcusplot PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(qcusplot PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::PrintSupport
)