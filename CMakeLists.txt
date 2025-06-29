cmake_minimum_required(VERSION 3.16)

project(frelay VERSION 0.2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_URL "https://github.com/taynpg/frelay")

if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
set(QT_DEFAULT_MAJOR_VERSION 6)
endif()

set(QAPPLICATION_CLASS QApplication)

if (MSVC)
add_compile_options(/utf-8)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
message(STATUS "frelay use MINGW compiler.")
set(COMPILER_USE_MINGW ON)
add_definitions(-DCOMPILER_USE_MINGW)
endif()

if(WIN32)
if(DEFINED XP_PLATFORM_SUPPORT)
message(STATUS "Support Windows XP platform => ${XP_PLATFORM_SUPPORT}.")
include_directories(${CMAKE_SOURCE_DIR}/Gui/Control)
add_definitions(-D_WIN32_WINNT=0x0501)
else()
add_definitions(-D_WIN32_WINNT=0x0601)
endif()
endif()

set(CMAKE_DEBUG_POSTFIX "d")
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/) 
include_directories(${PROJECT_BINARY_DIR})

add_definitions(-DFMT_HEADER_ONLY)
include_directories(3rd)

if(NOT DEFINED COMPILER_USE_MINGW)
add_subdirectory(crashelper)
endif()

add_subdirectory(Protocol)
add_subdirectory(Server)
add_subdirectory(ClientCore)
add_subdirectory(Util)
if(DEFINED COMPILE_GUI)
add_subdirectory(3rd/SingleApplication-3.5.2)
add_subdirectory(Gui)
endif()
add_subdirectory(Console)
add_subdirectory(Struct)
add_subdirectory(Test)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(version.h.in fversion.h)
message(STATUS "${CMAKE_SYSTEM_NAME} build dir:${PROJECT_BINARY_DIR}")
message(STATUS "VERSION_GIT_BRANCH: ${VERSION_GIT_BRANCH}")
message(STATUS "VERSION_GIT_HASH: ${VERSION_GIT_HASH}")
