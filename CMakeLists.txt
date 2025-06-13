cmake_minimum_required(VERSION 3.16)

project(frelay VERSION 0.1.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
add_compile_options(/utf-8)
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

add_subdirectory(Protocol)
add_subdirectory(Test)
