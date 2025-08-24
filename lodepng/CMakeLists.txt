cmake_minimum_required(VERSION 3.10)
project(lodepng)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Set flags for Debug and Release
add_compile_options(-O3 -Wall -Wextra -Wpedantic -Werror)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g)
else()
    add_link_options(-s)
endif()


set(BUILD_SHARED_LIBS OFF)


file(GLOB SOURCES "*.cpp" "*.c")
