cmake_minimum_required(VERSION 3.20)
project(quantum-entropy-backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(entropy
    src/entropy_provider.cpp
    src/philox_provider.cpp
    src/chacha20_provider.cpp
    src/qrng_provider.cpp
    src/qrng_http_client.cpp
    src/hybrid_provider.cpp
    src/file_provider.cpp
    src/ring_buffer.cpp
    src/config.cpp
)

target_include_directories(entropy PUBLIC include)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
target_link_libraries(entropy PUBLIC Threads::Threads CURL::libcurl)

option(ENTROPY_BUILD_TESTS "Build tests" ON)
option(ENTROPY_BUILD_BENCH "Build benchmarks" ON)

if(ENTROPY_BUILD_TESTS)
    enable_testing()
    add_executable(test_ring_buffer tests/test_ring_buffer.cpp)
    target_link_libraries(test_ring_buffer PRIVATE entropy)
    add_test(NAME test_ring_buffer COMMAND test_ring_buffer)

    add_executable(test_providers tests/test_providers.cpp)
    target_link_libraries(test_providers PRIVATE entropy)
    add_test(NAME test_providers COMMAND test_providers)
endif()

if(ENTROPY_BUILD_BENCH)
    add_executable(entropy_bench bench/bench_providers.cpp)
    target_link_libraries(entropy_bench PRIVATE entropy)
endif()
