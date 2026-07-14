#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace entropy {

class RingBuffer {
public:
    explicit RingBuffer(size_t capacity);
    bool push(const uint64_t* data, size_t count);
    bool pop(uint64_t* data, size_t count);
    size_t available() const;
    size_t space() const;
    bool empty() const;
    bool full() const;

private:
    std::vector<uint64_t> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    size_t capacity_;
};

} // namespace entropy
