cmake_minimum_required(VERSION 3.16)

project(frelayHttp LANGUAGES CXX)

if (MSVC)
add_compile_options(/utf-8)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(frelayHttp
  main.cpp HttpServer.h HttpServer.cpp ../Res/ico.rc
)
target_link_libraries(frelayHttp PRIVATE Qt${QT_VERSION_MAJOR}::Core)

include(GNUInstallDirs)
install(TARGETS frelayHttp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
