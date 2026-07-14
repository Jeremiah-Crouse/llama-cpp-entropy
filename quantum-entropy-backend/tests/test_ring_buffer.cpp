#include "entropy/ring_buffer.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_basic_push_pop() {
    entropy::RingBuffer buf(1024);
    std::vector<uint64_t> data = {1, 2, 3, 4, 5};

    assert(buf.push(data.data(), 5));
    assert(buf.available() == 5);

    std::vector<uint64_t> out(5);
    assert(buf.pop(out.data(), 5));
    assert(out == data);
    assert(buf.empty());
}

void test_wraparound() {
    entropy::RingBuffer buf(8);
    std::vector<uint64_t> data1 = {1, 2, 3, 4, 5, 6};
    std::vector<uint64_t> data2 = {7, 8, 9, 10, 11, 12};

    assert(buf.push(data1.data(), 6));
    std::vector<uint64_t> out1(6);
    assert(buf.pop(out1.data(), 6));

    assert(buf.push(data2.data(), 6));
    std::vector<uint64_t> out2(6);
    assert(buf.pop(out2.data(), 6));
    assert(out2 == data2);
}

void test_full_buffer() {
    entropy::RingBuffer buf(8);
    std::vector<uint64_t> data = {1, 2, 3, 4, 5, 6, 7};

    assert(buf.push(data.data(), 7));
    assert(buf.full());
    assert(!buf.push(data.data(), 1));
}

int main() {
    test_basic_push_pop();
    test_wraparound();
    test_full_buffer();

    std::cout << "All ring buffer tests passed!" << std::endl;
    return 0;
}
