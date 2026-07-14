#include "entropy/philox_provider.h"
#include <chrono>

namespace entropy {

PhiloxProvider::PhiloxProvider(uint64_t seed)
    : rng_(seed == 0 ? std::chrono::steady_clock::now().time_since_epoch().count() : seed) {}

uint64_t PhiloxProvider::next_u64() {
    stats_.entropy_consumed += 8;
    return rng_();
}

double PhiloxProvider::uniform01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    stats_.entropy_consumed += 8;
    return dist(rng_);
}

std::string PhiloxProvider::name() const {
    return "philox";
}

Stats PhiloxProvider::stats() const {
    return stats_;
}

void PhiloxProvider::reset_stats() {
    stats_ = Stats{};
}

} // namespace entropy
