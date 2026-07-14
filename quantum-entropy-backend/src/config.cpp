#include "entropy/config.h"
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace entropy {

Config load_config(const std::string& path) {
    Config config;
    std::ifstream file(path);
    if (!file.is_open()) {
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "provider") {
            config.provider = value;
        } else if (key == "qrng_endpoint") {
            config.qrng_endpoint = value;
        } else if (key == "qrng_buffer_mb") {
            config.qrng_buffer_mb = std::stoull(value);
        } else if (key == "qrng_low_water_mb") {
            config.qrng_low_water_mb = std::stoull(value);
        } else if (key == "qrng_timeout_ms") {
            config.qrng_timeout_ms = std::stoi(value);
        } else if (key == "fallback_mode") {
            config.fallback_mode = value;
        } else if (key == "file_path") {
            config.file_path = value;
        } else if (key == "seed") {
            config.seed = std::stoull(value);
        }
    }

    return config;
}

Config load_config_from_env() {
    Config config;

    if (const char* provider = std::getenv("ENTROPY_PROVIDER")) {
        config.provider = provider;
    }
    if (const char* endpoint = std::getenv("ENTROPY_QRNG_ENDPOINT")) {
        config.qrng_endpoint = endpoint;
    }
    if (const char* buffer_mb = std::getenv("ENTROPY_QRNG_BUFFER_MB")) {
        config.qrng_buffer_mb = std::stoull(buffer_mb);
    }
    if (const char* low_water_mb = std::getenv("ENTROPY_QRNG_LOW_WATER_MB")) {
        config.qrng_low_water_mb = std::stoull(low_water_mb);
    }
    if (const char* timeout_ms = std::getenv("ENTROPY_QRNG_TIMEOUT_MS")) {
        config.qrng_timeout_ms = std::stoi(timeout_ms);
    }
    if (const char* fallback = std::getenv("ENTROPY_FALLBACK_MODE")) {
        config.fallback_mode = fallback;
    }
    if (const char* file_path = std::getenv("ENTROPY_FILE_PATH")) {
        config.file_path = file_path;
    }
    if (const char* seed = std::getenv("ENTROPY_SEED")) {
        config.seed = std::stoull(seed);
    }

    return config;
}

} // namespace entropy
