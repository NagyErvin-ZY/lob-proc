cmake_minimum_required(VERSION 3.14)
project(buni)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static library for order book parser
add_library(buni_lib STATIC
    src/utils.cpp
    src/order_factory.cpp
    src/snapshot_parser.cpp
)
target_include_directories(buni_lib PUBLIC ${CMAKE_SOURCE_DIR})

# Main executable
add_executable(buni main.cxx)
target_link_libraries(buni buni_lib)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(buni_tests
    tests/constructor_test.cpp
    tests/double_compare_test.cpp
    tests/buy_book_test.cpp
    tests/sell_book_test.cpp
    tests/market_order_test.cpp
    tests/negative_test.cpp
    tests/seeker_bounds_test.cpp
    tests/edge_case_test.cpp
    tests/multi_pair_test.cpp
)
target_link_libraries(buni_tests buni_lib GTest::gtest_main)
target_include_directories(buni_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)

include(GoogleTest)
gtest_discover_tests(buni_tests)

# Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# Performance Benchmark
add_executable(buni_benchmark
    benchmarks/perf_benchmark.cpp
)
target_include_directories(buni_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
target_link_libraries(buni_benchmark buni_lib benchmark::benchmark)

# nats.c client
FetchContent_Declare(
  nats
  GIT_REPOSITORY https://github.com/nats-io/nats.c.git
  GIT_TAG v3.9.1
)
set(NATS_BUILD_STREAMING OFF CACHE BOOL "" FORCE)
set(NATS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nats)

# NATS processor example
add_executable(nats_processor
    examples/nats_processor.cpp
)
target_include_directories(nats_processor PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
target_link_libraries(nats_processor buni_lib nats)

# NATS E2E benchmark
add_executable(nats_e2e_benchmark
    benchmarks/nats_e2e_benchmark.cpp
)
target_include_directories(nats_e2e_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
target_link_libraries(nats_e2e_benchmark buni_lib nats)
