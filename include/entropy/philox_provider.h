#pragma once

#include "entropy_provider.h"
#include <random>

namespace entropy {

class PhiloxProvider : public EntropyProvider {
public:
    explicit PhiloxProvider(uint64_t seed = 0);
    uint64_t next_u64() override;
    double uniform01() override;
    std::string name() const override;
    Stats stats() const override;
    void reset_stats() override;

private:
    std::mt19937_64 rng_;
    Stats stats_;
};

} // namespace entropy
