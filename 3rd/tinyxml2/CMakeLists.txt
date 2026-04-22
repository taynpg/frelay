cmake_minimum_required(VERSION 3.16)

project(tinyxml2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(tinyxml2 tinyxml2.cpp tinyxml2.h)
target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})