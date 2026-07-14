#include "entropy/ring_buffer.h"
#include <algorithm>
#include <cstring>

namespace entropy {

RingBuffer::RingBuffer(size_t capacity)
    : buffer_(capacity)
    , capacity_(capacity) {}

bool RingBuffer::push(const uint64_t* data, size_t count) {
    size_t head = head_.load(std::memory_order_relaxed);
    size_t tail = tail_.load(std::memory_order_acquire);

    size_t free_space = (tail + capacity_ - head - 1) % capacity_;
    if (count > free_space) {
        return false;
    }

    size_t to_end = capacity_ - head;
    if (count <= to_end) {
        std::memcpy(buffer_.data() + head, data, count * sizeof(uint64_t));
    } else {
        std::memcpy(buffer_.data() + head, data, to_end * sizeof(uint64_t));
        std::memcpy(buffer_.data(), data + to_end, (count - to_end) * sizeof(uint64_t));
    }

    head_.store((head + count) % capacity_, std::memory_order_release);
    return true;
}

bool RingBuffer::pop(uint64_t* data, size_t count) {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_relaxed);

    size_t available = (head + capacity_ - tail) % capacity_;
    if (count > available) {
        return false;
    }

    size_t to_end = capacity_ - tail;
    if (count <= to_end) {
        std::memcpy(data, buffer_.data() + tail, count * sizeof(uint64_t));
    } else {
        std::memcpy(data, buffer_.data() + tail, to_end * sizeof(uint64_t));
        std::memcpy(data + to_end, buffer_.data(), (count - to_end) * sizeof(uint64_t));
    }

    tail_.store((tail + count) % capacity_, std::memory_order_release);
    return true;
}

size_t RingBuffer::available() const {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_acquire);
    return (head + capacity_ - tail) % capacity_;
}

size_t RingBuffer::space() const {
    return capacity_ - 1 - available();
}

bool RingBuffer::empty() const {
    return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
}

bool RingBuffer::full() const {
    return space() == 0;
}

} // namespace entropy
