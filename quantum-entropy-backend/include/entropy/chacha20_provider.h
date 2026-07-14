#pragma once

#include "entropy_provider.h"
#include <array>
#include <cstdint>

namespace entropy {

class ChaCha20Provider : public EntropyProvider {
public:
    explicit ChaCha20Provider(const std::array<uint8_t, 32>& key, uint64_t nonce = 0);
    uint64_t next_u64() override;
    double uniform01() override;
    std::string name() const override;
    Stats stats() const override;
    void reset_stats() override;

private:
    void generate_block();
    std::array<uint32_t, 16> state_;
    std::array<uint8_t, 64> output_;
    size_t position_ = 64;
    uint64_t counter_ = 0;
    Stats stats_;
};

} // namespace entropy
