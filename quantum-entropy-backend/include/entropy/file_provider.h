#pragma once

#include "entropy_provider.h"
#include <cstdint>
#include <fstream>
#include <string>

namespace entropy {

class FileProvider : public EntropyProvider {
public:
    explicit FileProvider(const std::string& file_path);
    ~FileProvider();

    uint64_t next_u64() override;
    double uniform01() override;
    std::string name() const override;
    Stats stats() const override;
    void reset_stats() override;

private:
    std::ifstream file_;
    Stats stats_;
};

} // namespace entropy
