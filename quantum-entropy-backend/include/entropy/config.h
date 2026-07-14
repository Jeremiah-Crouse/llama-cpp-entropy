#pragma once

#include "entropy_provider.h"
#include <string>

namespace entropy {

struct Config {
    std::string provider = "philox";
    std::string qrng_endpoint = "";
    size_t qrng_buffer_mb = 128;
    size_t qrng_low_water_mb = 32;
    int qrng_timeout_ms = 5000;
    std::string fallback_mode = "philox";
    std::string file_path = "";
    uint64_t seed = 0;
};

Config load_config(const std::string& path);
Config load_config_from_env();

} // namespace entropy
