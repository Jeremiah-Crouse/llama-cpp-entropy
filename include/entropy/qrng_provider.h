#pragma once

#include "entropy_provider.h"
#include "ring_buffer.h"
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace entropy {

enum class FallbackMode {
    BLOCK,
    PHILOX,
    THROW
};

class QRNGProvider : public EntropyProvider {
public:
    QRNGProvider(const std::string& endpoint, size_t buffer_mb = 128, size_t low_water_mb = 32, FallbackMode fallback = FallbackMode::PHILOX, int timeout_ms = 5000);
    ~QRNGProvider();

    uint64_t next_u64() override;
    double uniform01() override;
    std::string name() const override;
    Stats stats() const override;
    void reset_stats() override;

private:
    void refill_thread();
    bool fetch_entropy(uint64_t* data, size_t count);
    uint64_t next_with_fallback();

    std::string endpoint_;
    int timeout_ms_;
    FallbackMode fallback_mode_;
    std::unique_ptr<RingBuffer> buffer_;
    std::unique_ptr<class EntropyProvider> fallback_;
    std::thread refill_thread_;
    std::atomic<bool> running_{true};
    std::atomic<size_t> low_water_;
    Stats stats_;
};

} // namespace entropy
