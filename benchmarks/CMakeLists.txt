add_executable(benchmark benchmark.cpp)
target_include_directories(version_weaver
  PUBLIC
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/benchmarks>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(benchmark version_weaver)