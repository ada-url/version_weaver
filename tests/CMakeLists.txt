include(CTest)
add_executable(basictests basictests.cpp)
target_link_libraries(basictests version_weaver)
add_test(basictests_test basictests)