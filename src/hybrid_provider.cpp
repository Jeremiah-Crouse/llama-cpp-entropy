#include "entropy/hybrid_provider.h"
#include <chrono>

namespace entropy {

HybridProvider::HybridProvider(std::unique_ptr<EntropyProvider> primary, std::unique_ptr<EntropyProvider> secondary, double reseed_interval)
    : primary_(std::move(primary))
    , secondary_(std::move(secondary))
    , reseed_interval_(reseed_interval) {}

uint64_t HybridProvider::next_u64() {
    counter_++;
    if (counter_ % static_cast<uint64_t>(reseed_interval_ * 1000000) == 0) {
        // Periodically reseed from secondary
        secondary_->next_u64();
    }

    stats_.entropy_consumed += 8;
    return primary_->next_u64();
}

double HybridProvider::uniform01() {
    uint64_t raw = next_u64();
    return (raw >> 11) * (1.0 / 9007199254740992.0);
}

std::string HybridProvider::name() const {
    return "hybrid(" + primary_->name() + "," + secondary_->name() + ")";
}

Stats HybridProvider::stats() const {
    auto primary_stats = primary_->stats();
    auto secondary_stats = secondary_->stats();

    Stats combined;
    combined.bytes_downloaded = primary_stats.bytes_downloaded + secondary_stats.bytes_downloaded;
    combined.refill_count = primary_stats.refill_count + secondary_stats.refill_count;
    combined.refill_latency_ms = primary_stats.refill_latency_ms + secondary_stats.refill_latency_ms;
    combined.entropy_consumed = primary_stats.entropy_consumed;
    combined.starvation_count = primary_stats.starvation_count + secondary_stats.starvation_count;
    combined.fallback_count = primary_stats.fallback_count + secondary_stats.fallback_count;
    combined.avg_refill_time_ms = primary_stats.avg_refill_time_ms + secondary_stats.avg_refill_time_ms;

    return combined;
}

void HybridProvider::reset_stats() {
    primary_->reset_stats();
    secondary_->reset_stats();
    stats_ = Stats{};
}

} // namespace entropy
