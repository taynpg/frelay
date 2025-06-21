cmake_minimum_required(VERSION 3.16)

project(frelay VERSION 0.1.3 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_URL "https://github.com/taynpg/frelay")
set(QT_DEFAULT_MAJOR_VERSION 6)
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
execute_process(COMMAND cmd /c ver
    OUTPUT_VARIABLE VER_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(VER_OUTPUT MATCHES "XP")
    message(STATUS "Windows XP platform.")
    add_definitions(-D_WIN32_WINNT=0x0501)
else()
    message(STATUS "Windows NT platform.")
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()
endif()

set(CMAKE_DEBUG_POSTFIX "d")
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/) 
include_directories(${PROJECT_BINARY_DIR})

add_definitions(-DFMT_HEADER_ONLY)
include_directories(3rd)
add_subdirectory(3rd/SingleApplication-3.5.2)
add_subdirectory(crashelper)
add_subdirectory(Protocol)
add_subdirectory(Server)
add_subdirectory(ClientCore)
add_subdirectory(Util)
add_subdirectory(Gui)
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
