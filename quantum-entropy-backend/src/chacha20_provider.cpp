#include "entropy/chacha20_provider.h"
#include <cstring>

namespace entropy {

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
    a += b; d ^= a; d = rotl32(d, 16);
    c += d; b ^= c; b = rotl32(b, 12);
    a += b; d ^= a; d = rotl32(d, 8);
    c += d; b ^= c; b = rotl32(b, 7);
}

ChaCha20Provider::ChaCha20Provider(const std::array<uint8_t, 32>& key, uint64_t nonce) {
    const char* constants = "expand 32-byte k";
    std::memcpy(state_.data(), constants, 16);
    std::memcpy(state_.data() + 4, key.data(), 32);
    state_[12] = static_cast<uint32_t>(nonce);
    state_[13] = static_cast<uint32_t>(nonce >> 32);
}

void ChaCha20Provider::generate_block() {
    std::array<uint32_t, 16> working = state_;

    for (int i = 0; i < 10; ++i) {
        quarter_round(working[0], working[4], working[8], working[12]);
        quarter_round(working[1], working[5], working[9], working[13]);
        quarter_round(working[2], working[6], working[10], working[14]);
        quarter_round(working[3], working[7], working[11], working[15]);
        quarter_round(working[0], working[5], working[10], working[15]);
        quarter_round(working[1], working[6], working[11], working[12]);
        quarter_round(working[2], working[7], working[8], working[13]);
        quarter_round(working[3], working[4], working[9], working[14]);
    }

    for (int i = 0; i < 16; ++i) {
        working[i] += state_[i];
    }

    std::memcpy(output_.data(), working.data(), 64);
    state_[12]++;
    position_ = 0;
}

uint64_t ChaCha20Provider::next_u64() {
    if (position_ + 8 > 64) {
        generate_block();
    }

    uint64_t value;
    std::memcpy(&value, output_.data() + position_, 8);
    position_ += 8;
    stats_.entropy_consumed += 8;
    return value;
}

double ChaCha20Provider::uniform01() {
    uint64_t raw = next_u64();
    return (raw >> 11) * (1.0 / 9007199254740992.0);
}

std::string ChaCha20Provider::name() const {
    return "chacha20";
}

Stats ChaCha20Provider::stats() const {
    return stats_;
}

void ChaCha20Provider::reset_stats() {
    stats_ = Stats{};
}

} // namespace entropy
