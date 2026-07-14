#include "entropy/qrng_provider.h"
#include "entropy/qrng_http_client.h"
#include "entropy/philox_provider.h"
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace entropy {

QRNGProvider::QRNGProvider(const std::string& endpoint, size_t buffer_mb, size_t low_water_mb, FallbackMode fallback, int timeout_ms)
    : endpoint_(endpoint)
    , timeout_ms_(timeout_ms)
    , fallback_mode_(fallback)
    , buffer_(std::make_unique<RingBuffer>(buffer_mb * 1024 * 1024 / 8))
    , low_water_(low_water_mb * 1024 * 1024 / 8) {

    if (fallback == FallbackMode::PHILOX) {
        fallback_ = std::make_unique<PhiloxProvider>();
    }

    refill_thread_ = std::thread(&QRNGProvider::refill_thread, this);
}

QRNGProvider::~QRNGProvider() {
    running_ = false;
    if (refill_thread_.joinable()) {
        refill_thread_.join();
    }
}

void QRNGProvider::refill_thread() {
    while (running_) {
        if (buffer_->available() < low_water_) {
            size_t to_fetch = buffer_->space();
            std::vector<uint64_t> data(to_fetch);

            auto start = std::chrono::steady_clock::now();
            if (fetch_entropy(data.data(), to_fetch)) {
                auto end = std::chrono::steady_clock::now();
                double latency = std::chrono::duration<double, std::milli>(end - start).count();

                buffer_->push(data.data(), to_fetch);

                stats_.bytes_downloaded += to_fetch * 8;
                stats_.refill_count++;
                stats_.refill_latency_ms += latency;
                stats_.avg_refill_time_ms = stats_.refill_latency_ms / stats_.refill_count;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool QRNGProvider::fetch_entropy(uint64_t* data, size_t count) {
    QRNGHttpClient client(endpoint_, timeout_ms_);
    return client.fetch_random(data, count);
}

uint64_t QRNGProvider::next_with_fallback() {
    uint64_t value;
    if (buffer_->pop(&value, 1)) {
        return value;
    }

    stats_.starvation_count++;

    switch (fallback_mode_) {
        case FallbackMode::BLOCK:
            while (!buffer_->pop(&value, 1)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return value;

        case FallbackMode::PHILOX:
            if (fallback_) {
                stats_.fallback_count++;
                return fallback_->next_u64();
            }
            throw std::runtime_error("QRNG provider starvation with no fallback");

        case FallbackMode::THROW:
            throw std::runtime_error("QRNG provider entropy starvation");
    }

    return 0;
}

uint64_t QRNGProvider::next_u64() {
    stats_.entropy_consumed += 8;
    return next_with_fallback();
}

double QRNGProvider::uniform01() {
    uint64_t raw = next_u64();
    return (raw >> 11) * (1.0 / 9007199254740992.0);
}

std::string QRNGProvider::name() const {
    return "qrng";
}

Stats QRNGProvider::stats() const {
    return stats_;
}

void QRNGProvider::reset_stats() {
    stats_ = Stats{};
}

} // namespace entropy
