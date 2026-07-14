#include "entropy/file_provider.h"
#include <stdexcept>

namespace entropy {

FileProvider::FileProvider(const std::string& file_path) {
    file_.open(file_path, std::ios::binary);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open entropy file: " + file_path);
    }
}

FileProvider::~FileProvider() {
    if (file_.is_open()) {
        file_.close();
    }
}

uint64_t FileProvider::next_u64() {
    uint64_t value;
    if (!file_.read(reinterpret_cast<char*>(&value), 8)) {
        file_.clear();
        file_.seekg(0);
        file_.read(reinterpret_cast<char*>(&value), 8);
    }

    stats_.entropy_consumed += 8;
    return value;
}

double FileProvider::uniform01() {
    uint64_t raw = next_u64();
    return (raw >> 11) * (1.0 / 9007199254740992.0);
}

std::string FileProvider::name() const {
    return "file";
}

Stats FileProvider::stats() const {
    return stats_;
}

void FileProvider::reset_stats() {
    stats_ = Stats{};
}

} // namespace entropy
