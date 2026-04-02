cmake_minimum_required(VERSION 3.14)
project(ThreadsafeQueueLib)

set(CMAKE_CXX_STANDARD 17)
enable_testing()

find_package(GTest REQUIRED)
include_directories(include)

# SPSC tests
add_executable(test_spsc tests/test_spsc.cpp)
target_link_libraries(test_spsc GTest::GTest GTest::Main pthread)

# MPSC tests
add_executable(test_mpsc tests/test_mpsc.cpp)
target_link_libraries(test_mpsc GTest::GTest GTest::Main pthread)

# MPMC tests
add_executable(test_mpmc tests/test_mpmc.cpp)
target_link_libraries(test_mpmc GTest::GTest GTest::Main pthread)

# Register tests
add_test(NAME SPSC COMMAND test_spsc)
add_test(NAME MPSC COMMAND test_mpsc)
add_test(NAME MPMC COMMAND test_mpmc)