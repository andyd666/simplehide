cmake_minimum_required(VERSION 3.10)
project(simplehide)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()



# Set compile and link options
add_compile_options(-O3 -Wall -Wextra -Wpedantic)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE ON)
    add_compile_options(-g -DDEBUG)
else()
    add_compile_options(-Werror)
    add_link_options(-s)
endif()



# Include subdirectories
add_subdirectory(src)
add_subdirectory(libs)
