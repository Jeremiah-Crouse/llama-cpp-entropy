#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace entropy {

struct Stats {
    uint64_t bytes_downloaded = 0;
    uint64_t refill_count = 0;
    double   refill_latency_ms = 0.0;
    uint64_t entropy_consumed = 0;
    uint64_t starvation_count = 0;
    uint64_t fallback_count = 0;
    double   avg_refill_time_ms = 0.0;
};

class EntropyProvider {
public:
    virtual ~EntropyProvider() = default;
    virtual uint64_t next_u64() = 0;
    virtual double uniform01() = 0;
    virtual std::string name() const = 0;
    virtual Stats stats() const = 0;
    virtual void reset_stats() = 0;
};

std::unique_ptr<EntropyProvider> create_provider(const std::string& type, const std::string& config_path = "");

} // namespace entropy
