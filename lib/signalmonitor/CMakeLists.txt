if(NOT DEFINED PROJECT_NAME)
    project(signalmonitor)
    cmake_minimum_required(VERSION 2.8)

    # C++ options
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
endif()

include_directories(.)
include_directories(..)

file(GLOB_RECURSE SM_SOURCES "*.cpp")
file(GLOB_RECURSE SM_HEADERS "*.h" "*.hpp")

set(SM_TARGET signalmonitor)

#include_directories(${Boost_INCLUDE_DIRS})

add_library(${SM_TARGET} STATIC ${SM_SOURCES})

#target_link_libraries(${SM_TARGET})
