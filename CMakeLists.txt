cmake_minimum_required(VERSION 3.16)

project(frelay VERSION 0.2.3 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_URL "https://github.com/taynpg/frelay")
set(DEVELOP_MARK "dev")
#set(DEVELOP_MARK "release")

if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
set(QT_DEFAULT_MAJOR_VERSION 6)
endif()

set(QAPPLICATION_CLASS QApplication)

if (MSVC)
add_compile_options(/utf-8)
endif()

if(DEFINED CMAKE_PREFIX_PATH)
message(STATUS "CMAKE_PREFIX_PATH => ${CMAKE_PREFIX_PATH}.")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
message(STATUS "frelay use MINGW compiler.")
set(COMPILER_USE_MINGW ON)
add_definitions(-DCOMPILER_USE_MINGW)
get_filename_component(CXX_COMPILER_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
set(MINGW_DLLS
    "${CXX_COMPILER_PATH}/libgcc_s_seh-1.dll"
    "${CXX_COMPILER_PATH}/libstdc++-6.dll"
    "${CXX_COMPILER_PATH}/libwinpthread-1.dll"
)
else()
set(MINGW_DLLS
    "${CXX_COMPILER_PATH}/libgcc_s_dw2-1.dll"
    "${CXX_COMPILER_PATH}/libstdc++-6.dll"
    "${CXX_COMPILER_PATH}/libwinpthread-1.dll"
)
endif()
endif()

if(WIN32)
if(DEFINED XP_PLATFORM_SUPPORT)
message(STATUS "Support Windows XP platform => ${XP_PLATFORM_SUPPORT}.")
add_definitions(-D_WIN32_WINNT=0x0501)
else()
#add_definitions(-D_WIN32_WINNT=0x0601)
endif()
set(QT_DEP_FILES
"${CMAKE_PREFIX_PATH}/bin/Qt${QT_DEFAULT_MAJOR_VERSION}Core.dll"
"${CMAKE_PREFIX_PATH}/bin/Qt${QT_DEFAULT_MAJOR_VERSION}Gui.dll"
"${CMAKE_PREFIX_PATH}/bin/Qt${QT_DEFAULT_MAJOR_VERSION}Widgets.dll"
"${CMAKE_PREFIX_PATH}/bin/Qt${QT_DEFAULT_MAJOR_VERSION}Network.dll"
)
set(QT_DEP_DIRS
"${CMAKE_PREFIX_PATH}/plugins/platforms/qwindows.dll"
)
endif()

set(CMAKE_DEBUG_POSTFIX "d")
include_directories(${CMAKE_SOURCE_DIR}/Gui/Control)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/) 
include_directories(${PROJECT_BINARY_DIR})

add_definitions(-DFMT_HEADER_ONLY)
include_directories(3rd)

add_subdirectory(Protocol)
add_subdirectory(Server)
add_subdirectory(ClientCore)

add_subdirectory(Util)
if(DEFINED COMPILE_GUI)
message(STATUS "Support Gui Part => ${COMPILE_GUI}.")
add_subdirectory(3rd/SingleApplication-3.5.2)
add_subdirectory(Gui)
install(TARGETS frelayGUI DESTINATION bin)
endif()

add_subdirectory(Console)
add_subdirectory(Struct)
add_subdirectory(Test)

install(TARGETS frelayConsole DESTINATION bin)
install(TARGETS frelayServer DESTINATION bin)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
install(FILES ${MINGW_DLLS} DESTINATION bin)
endif()
if (WIN32)
install(FILES ${QT_DEP_FILES} DESTINATION bin)
install(FILES ${QT_DEP_DIRS} DESTINATION bin/platforms)
endif()

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

set(CPACK_PACKAGE_NAME "frelay")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "taynpg <taynpg@163.com>")
set(CPACK_PACKAGE_DESCRIPTION "A simple tool that uses a server as a relay center to indirectly transfer files between multiple clients.")
set(CPACK_PACKAGE_VENDOR "taynpg")
if(WIN32)
set(CPACK_GENERATOR "NSIS;ZIP")
set(CPACK_NSIS_DISPLAY_NAME "frelay")
set(CPACK_NSIS_INSTALL_ROOT "$PROFILE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "frelay")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "
    CreateShortCut '$DESKTOP\\\\frelayGUI.lnk' '$INSTDIR\\\\bin\\\\frelayGUI.exe'
    CreateShortCut '$DESKTOP\\\\frelayServer.lnk' '$INSTDIR\\\\bin\\\\frelayServer.exe'
")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "
    Delete '$DESKTOP\\\\frelayGUI.lnk'
    Delete '$DESKTOP\\\\frelayServer.lnk'
")
else()
set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${DEVELOP_MARK}-v${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ID}")
include(CPack)
