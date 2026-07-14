#include "entropy/entropy_provider.h"
#include "entropy/philox_provider.h"
#include "entropy/chacha20_provider.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

void benchmark_provider(entropy::EntropyProvider& provider, const std::string& name, size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();

    uint64_t dummy = 0;
    for (size_t i = 0; i < iterations; ++i) {
        dummy ^= provider.next_u64();
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    double tokens_per_sec = iterations / elapsed;
    double avg_latency_ns = elapsed / iterations * 1e9;

    std::cout << std::left << std::setw(20) << name
              << std::right << std::setw(15) << std::fixed << std::setprecision(0) << tokens_per_sec
              << std::setw(15) << std::fixed << std::setprecision(1) << avg_latency_ns
              << std::setw(15) << provider.stats().entropy_consumed
              << std::endl;

    (void)dummy;
}

int main() {
    const size_t iterations = 1000000;

    std::cout << std::left << std::setw(20) << "Provider"
              << std::right << std::setw(15) << "Tokens/sec"
              << std::setw(15) << "Avg latency(ns)"
              << std::setw(15) << "Bytes consumed"
              << std::endl;
    std::cout << std::string(65, '-') << std::endl;

    entropy::PhiloxProvider philox(42);
    benchmark_provider(philox, "Philox", iterations);

    std::array<uint8_t, 32> key{};
    entropy::ChaCha20Provider chacha(key, 0);
    benchmark_provider(chacha, "ChaCha20", iterations);

    return 0;
}
