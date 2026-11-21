cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "0.1.1")
set(COMPONENTS main)
# PlatformIO needs its generated helper component to obtain C/C++ build settings.
if(BOAT_PLATFORMIO)
    list(APPEND COMPONENTS __pio_env)
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(boat_tach)
