cmake_minimum_required(VERSION 3.10)
project(simplehide-lib)



# create glob files for *.h, *.cpp
file (GLOB_RECURSE H_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file (GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file (GLOB_RECURSE C_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)


# concatenate the results (glob files) to variable
set  (SOURCES ${C_FILES} ${CPP_FILES} ${H_FILES})
# create lib from src
add_library(${PROJECT_NAME} STATIC ${SOURCES})


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
