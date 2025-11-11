cmake_minimum_required(VERSION 3.25)

project(poker VERSION 1.0.0 LANGUAGES CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Option to build tests
option(BUILD_TESTS "Build the test suite" OFF)

# Create executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Add tests subdirectory if requested
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
