project(supervise)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

# C++ options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Skip RPATH
set(CMAKE_SKIP_RPATH TRUE)

# Threads
find_package(Threads REQUIRED)

# pkg-config
find_package(PkgConfig)

# internal lib
add_subdirectory(lib/signalmonitor)
add_subdirectory(lib/processsupervisor)

include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}
    processsupervisor
    signalmonitor
    ${CMAKE_THREAD_LIBS_INIT})
