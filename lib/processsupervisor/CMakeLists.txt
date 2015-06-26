set(PS_TARGET processsupervisor)

if(NOT DEFINED PROJECT_NAME)
    project(${PS_TARGET})
    cmake_minimum_required(VERSION 2.8)

    # C++ options
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
endif()

include_directories(.)
include_directories(..)

file(GLOB_RECURSE PS_SOURCES "*.cpp")
file(GLOB_RECURSE PS_HEADERS "*.h" "*.hpp")

#include_directories(${Boost_INCLUDE_DIRS})

add_library(${PS_TARGET} STATIC ${PS_SOURCES})

#target_link_libraries(${SU_TARGET})
