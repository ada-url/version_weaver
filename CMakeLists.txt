cmake_minimum_required(VERSION 3.10)
project(version_weaver)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

include(GNUInstallDirs)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb810.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)

install(
  FILES include/version_weaver.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT version_weaver_development
)

install(
  TARGETS version_weaver
  EXPORT version_weaver_targets
  RUNTIME COMPONENT version_weaver_runtime
  LIBRARY COMPONENT version_weaver_runtime
  NAMELINK_COMPONENT version_weaver_development
  ARCHIVE COMPONENT version_weaver_development
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)