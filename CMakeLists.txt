cmake_minimum_required(VERSION 3.10)
project(version_weaver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()


add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)