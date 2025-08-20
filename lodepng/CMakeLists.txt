cmake_minimum_required(VERSION 3.10)
project(lodepng)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Set flags for Debug and Release
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wpedantic")


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG=true)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-s")
set(BUILD_SHARED_LIBS OFF)


file(GLOB SOURCES "*.cpp" "*.c")
