#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace entropy {

class QRNGHttpClient {
public:
    QRNGHttpClient(const std::string& base_url, int timeout_ms = 5000);
    ~QRNGHttpClient();

    bool fetch_random(uint64_t* data, size_t count);
    bool is_valid() const;

private:
    std::string base_url_;
    int timeout_ms_;
    void* curl_handle_;
};

} // namespace entropy
