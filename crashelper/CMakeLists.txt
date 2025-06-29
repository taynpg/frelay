cmake_minimum_required(VERSION 3.16)

project(crashelper LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
message(FATAL_ERROR "Unsupported MinGW Currently.")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(CRASHELPER_SOURCES
src/crashelper.cxx
)

include_directories(include)
add_library(crashelper STATIC ${CRASHELPER_SOURCES})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_link_libraries(crashelper PUBLIC pthread dl bfd Qt${QT_VERSION_MAJOR}::Core)
target_compile_options(crashelper PUBLIC -g)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
target_link_libraries(crashelper PUBLIC pthread dl bfd Qt${QT_VERSION_MAJOR}::Core)
target_compile_options(crashelper PUBLIC -g)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_link_libraries(crashelper PRIVATE DbgHelp Qt${QT_VERSION_MAJOR}::Core)
target_compile_options(crashelper PUBLIC $<$<OR:$<STREQUAL:$<CXX_COMPILER_ID>,MSVC>,$<STREQUAL:$<C_COMPILER_ID>,MSVC>>:/Zi>)
target_link_options(crashelper PUBLIC $<$<OR:$<STREQUAL:$<CXX_COMPILER_ID>,MSVC>,$<STREQUAL:$<C_COMPILER_ID>,MSVC>>:/DEBUG>)
else()
message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}. This project only supports Linux, macOS, and Windows.")
endif()
#message(STATUS "crashelper:${CMAKE_CURRENT_LIST_DIR}")
target_include_directories(crashelper PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)