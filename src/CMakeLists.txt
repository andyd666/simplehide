cmake_minimum_required(VERSION 3.10)
project(simplehide)

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

# create executable from src
add_executable(${PROJECT_NAME} ${SOURCES})


# Link libraries
target_link_libraries(${PROJECT_NAME} simplehide-lib)
target_link_libraries(${PROJECT_NAME} lodepng-lib)
