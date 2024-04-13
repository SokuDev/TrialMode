cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
set(PROJECT_NAME TrialMode)
project("${PROJECT_NAME}" C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_definitions(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing -Wno-microsoft-cast")
endif ()
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Brepro")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /Brepro")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /Brepro")

# ZLIB
include_directories(${CMAKE_BINARY_DIR}/zlib src) # This is required, otherwise zlib can't find zconf.h
add_subdirectory(zlib)
add_library(ZLIB::ZLIB ALIAS zlibstatic)

# ZIP lib
SET(BUILD_SHARED_LIBS OFF)
SET(LIBZIP_DO_INSTALL OFF)
SET(ENABLE_COMMONCRYPTO OFF)
SET(ENABLE_GNUTLS OFF)
SET(ENABLE_OPENSSL OFF)
SET(ENABLE_WINDOWS_CRYPTO OFF)
SET(ENABLE_BZIP2 OFF)
SET(ENABLE_LZMA OFF)
SET(ENABLE_ZSTD OFF)
SET(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
SET(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/zlib")
add_subdirectory(libzip)
add_dependencies(zip zlibstatic)
target_include_directories(zip PRIVATE ${ZLIB_INCLUDE_DIR})

# SokuLib
add_subdirectory(SokuLib)

# Module
add_library(
        "${PROJECT_NAME}"
        MODULE
        src/TrialMode.rc
        src/main.cpp
        src/Menu.cpp
        src/Menu.hpp
        src/Pack.cpp
        src/Pack.hpp
        src/Trial/Trial.cpp
        src/Trial/Trial.hpp
        src/Trial/ComboTrial.cpp
        src/Trial/ComboTrial.hpp
        src/TrialEditor/TrialEditor.cpp
        src/TrialEditor/TrialEditor.hpp
        src/TrialEditor/ComboTrialEditor.cpp
        src/TrialEditor/ComboTrialEditor.hpp
        src/Actions.cpp
        src/Actions.hpp
        src/Animations/BattleAnimation.cpp
        src/Animations/BattleAnimation.hpp
        src/Animations/BattleAnimFramework.cpp
        src/Animations/LuaBattleAnimation.cpp
        src/Animations/LuaBattleAnimation.hpp
        src/Images.cpp
        src/Images.hpp
        src/Animations/FakeChrMgr.cpp
        src/Animations/FakeChrMgr.hpp
        src/Patches.cpp
        src/Patches.hpp
        src/TrialBase.hpp
        src/TrialBase.cpp
        src/PackOutro.cpp
        src/PackOutro.hpp
        src/InputBox.cpp
        src/InputBox.hpp
        src/Explorer.cpp
        src/Explorer.hpp
        src/Socket.cpp
        src/Socket.hpp
        src/ExplorerMenu.cpp
        src/ExplorerMenu.hpp
        src/Crc32.cpp
        src/Crc32.h
        src/version.rc
)
target_compile_options("${PROJECT_NAME}" PRIVATE /Zi /bigobj)
target_compile_definitions("${PROJECT_NAME}" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
target_include_directories("${PROJECT_NAME}" PRIVATE libraries/include SokuLib/directx "src/${PROJECT_NAME}/")
target_link_directories("${PROJECT_NAME}" PRIVATE libraries/lib)
target_link_libraries(
        "${PROJECT_NAME}"
        SokuLib
        shlwapi
        ws2_32
        user32
        lua54
        zip
        d3d9
        d3dx9
)
