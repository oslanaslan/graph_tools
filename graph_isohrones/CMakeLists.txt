cmake_minimum_required(VERSION 3.27)

project(
    graph_utils
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

add_subdirectory(${PROJECT_NAME})

option(BUILD_TESTS "build tests" ON)

if (${BUILD_TESTS})
    add_subdirectory(test)
endif()
