cmake_minimum_required(VERSION 3.16)

project(frelayTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(frelayTest protocolTest.cpp)
target_link_libraries(frelayTest PRIVATE Protocol)