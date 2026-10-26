cmake_minimum_required(VERSION 3.21)
project(FRSFastTest LANGUAGES CXX)

# The games are 32-bit, so is the plugin:
#
#   cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
#   cmake --build build --config Release [--target nfsu2]
#
# Each game builds build/<game>/FRSFastTest.asi.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FRSFASTTEST_GAMES nfsu1 nfsu2 nfsmw nfsc nfsps)

foreach(game IN LISTS FRSFASTTEST_GAMES)
    add_library(${game} SHARED games/${game}/FastTest.cpp common/fasttest.h)
    target_include_directories(${game} PRIVATE common)
    target_compile_definitions(${game} PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
    target_link_libraries(${game} PRIVATE user32 advapi32)
    if(MSVC)
        target_compile_options(${game} PRIVATE /W4 /utf-8)
        # static CRT: nothing to ship beside the plugin
        set_property(TARGET ${game} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
    set_target_properties(${game} PROPERTIES
        OUTPUT_NAME "FRSFastTest"
        SUFFIX ".asi"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${game}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${game}")
endforeach()
