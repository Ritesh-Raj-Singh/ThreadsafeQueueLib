cmake_minimum_required(VERSION 3.14)
project(ThreadsafeQueueLib)

set(CMAKE_CXX_STANDARD 17)
enable_testing()

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
include_directories(include)

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -g -O1)
    add_link_options(-fsanitize=thread)
endif()

# Google Benchmark
include(FetchContent)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable benchmark testing" FORCE)
FetchContent_MakeAvailable(benchmark)

# SPSC tests
add_executable(test_spsc tests/test_spsc.cpp)
target_link_libraries(test_spsc GTest::GTest GTest::Main Threads::Threads)

# MPSC tests
add_executable(test_mpsc tests/test_mpsc.cpp)
target_link_libraries(test_mpsc GTest::GTest GTest::Main Threads::Threads)

# MPMC tests
add_executable(test_mpmc tests/test_mpmc.cpp)
target_link_libraries(test_mpmc GTest::GTest GTest::Main Threads::Threads)

# Benchmarks
add_executable(bench_spsc benchmarks/bench_spsc.cpp)
target_link_libraries(bench_spsc benchmark::benchmark benchmark::benchmark_main Threads::Threads)

add_executable(bench_mpsc benchmarks/bench_mpsc.cpp)
target_link_libraries(bench_mpsc benchmark::benchmark benchmark::benchmark_main Threads::Threads)

add_executable(bench_mpmc benchmarks/bench_mpmc.cpp)
target_link_libraries(bench_mpmc benchmark::benchmark benchmark::benchmark_main Threads::Threads)

# Register tests
add_test(NAME SPSC COMMAND test_spsc)
add_test(NAME MPSC COMMAND test_mpsc)
add_test(NAME MPMC COMMAND test_mpmc)

# Examples
add_executable(example_spsc_logger examples/01_spsc_logger.cpp)
target_link_libraries(example_spsc_logger Threads::Threads)

add_executable(example_mpmc_threadpool examples/02_mpmc_threadpool.cpp)
target_link_libraries(example_mpmc_threadpool Threads::Threads)

add_executable(example_mpsc_aggregator examples/03_mpsc_event_aggregator.cpp)
target_link_libraries(example_mpsc_aggregator Threads::Threads)

add_executable(example_spscb_audio examples/04_spscb_audio_buffer.cpp)
target_link_libraries(example_spscb_audio Threads::Threads)