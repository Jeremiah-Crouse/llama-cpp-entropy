#include "entropy/entropy_provider.h"
#include "entropy/philox_provider.h"
#include "entropy/chacha20_provider.h"
#include "entropy/qrng_provider.h"
#include "entropy/hybrid_provider.h"
#include "entropy/file_provider.h"
#include "entropy/config.h"
#include <stdexcept>

namespace entropy {

std::unique_ptr<EntropyProvider> create_provider(const std::string& type, const std::string& config_path) {
    Config config;
    if (!config_path.empty()) {
        config = load_config(config_path);
    } else {
        config = load_config_from_env();
    }

    if (!type.empty()) {
        config.provider = type;
    }

    if (config.provider == "philox") {
        return std::make_unique<PhiloxProvider>(config.seed);
    }

    if (config.provider == "chacha20") {
        std::array<uint8_t, 32> key{};
        if (config.seed != 0) {
            std::memcpy(key.data(), &config.seed, 8);
        }
        return std::make_unique<ChaCha20Provider>(key, config.seed);
    }

    if (config.provider == "qrng") {
        FallbackMode fallback = FallbackMode::PHILOX;
        if (config.fallback_mode == "block") {
            fallback = FallbackMode::BLOCK;
        } else if (config.fallback_mode == "throw") {
            fallback = FallbackMode::THROW;
        }
        return std::make_unique<QRNGProvider>(
            config.qrng_endpoint,
            config.qrng_buffer_mb,
            config.qrng_low_water_mb,
            fallback,
            config.qrng_timeout_ms
        );
    }

    if (config.provider == "hybrid") {
        auto primary = std::make_unique<PhiloxProvider>(config.seed);
        auto secondary = std::make_unique<PhiloxProvider>(config.seed + 1);
        return std::make_unique<HybridProvider>(std::move(primary), std::move(secondary));
    }

    if (config.provider == "file") {
        return std::make_unique<FileProvider>(config.file_path);
    }

    throw std::runtime_error("Unknown entropy provider: " + config.provider);
}

} // namespace entropy
