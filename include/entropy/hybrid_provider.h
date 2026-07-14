#pragma once

#include "entropy_provider.h"
#include <memory>

namespace entropy {

class HybridProvider : public EntropyProvider {
public:
    HybridProvider(std::unique_ptr<EntropyProvider> primary, std::unique_ptr<EntropyProvider> secondary, double reseed_interval = 1.0);
    uint64_t next_u64() override;
    double uniform01() override;
    std::string name() const override;
    Stats stats() const override;
    void reset_stats() override;

private:
    std::unique_ptr<EntropyProvider> primary_;
    std::unique_ptr<EntropyProvider> secondary_;
    double reseed_interval_;
    uint64_t counter_ = 0;
    Stats stats_;
};

} // namespace entropy
