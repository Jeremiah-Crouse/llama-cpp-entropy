#include "entropy/entropy_provider.h"
#include "entropy/philox_provider.h"
#include "entropy/chacha20_provider.h"
#include "entropy/file_provider.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

void test_philox_provider() {
    entropy::PhiloxProvider provider(42);

    uint64_t v1 = provider.next_u64();
    uint64_t v2 = provider.next_u64();
    assert(v1 != v2);

    double d1 = provider.uniform01();
    double d2 = provider.uniform01();
    assert(d1 >= 0.0 && d1 < 1.0);
    assert(d2 >= 0.0 && d2 < 1.0);

    assert(provider.name() == "philox");
    assert(provider.stats().entropy_consumed == 16);
}

void test_chacha20_provider() {
    std::array<uint8_t, 32> key{};
    key[0] = 1;
    key[1] = 2;
    entropy::ChaCha20Provider provider(key, 0);

    uint64_t v1 = provider.next_u64();
    uint64_t v2 = provider.next_u64();
    assert(v1 != v2);

    double d1 = provider.uniform01();
    assert(d1 >= 0.0 && d1 < 1.0);

    assert(provider.name() == "chacha20");
}

void test_file_provider() {
    const char* path = "/tmp/entropy_test.bin";
    {
        std::ofstream f(path, std::ios::binary);
        std::vector<uint64_t> data = {12345, 67890, 11111};
        f.write(reinterpret_cast<const char*>(data.data()), data.size() * 8);
    }

    entropy::FileProvider provider(path);
    assert(provider.next_u64() == 12345);
    assert(provider.next_u64() == 67890);
    assert(provider.next_u64() == 11111);
    assert(provider.next_u64() == 12345); // wraps around

    assert(provider.name() == "file");
}

int main() {
    test_philox_provider();
    test_chacha20_provider();
    test_file_provider();

    std::cout << "All provider tests passed!" << std::endl;
    return 0;
}
