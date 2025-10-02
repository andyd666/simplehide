cmake_minimum_required(VERSION 3.10)
project(simplehide-lib-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# creating output directory architecture in accordance with GNU guidelines
set(BINARY_DIR "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BINARY_DIR}/lib")



# create glob files for *.h, *.cpp
file (GLOB_RECURSE H_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file (GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file (GLOB_RECURSE C_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# concatenate the results (glob files) to variable
set  (SOURCES ${C_FILES} ${CPP_FILES} ${H_FILES})

# create lib from src
if (NOT TARGET ${PROJECT_NAME})
    add_library(${PROJECT_NAME} STATIC ${SOURCES})
endif()
add_executable(run_test test.c)
add_executable(speed_test speed_test.cpp)

#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


