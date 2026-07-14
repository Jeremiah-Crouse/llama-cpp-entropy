#include "entropy/qrng_http_client.h"
#include <curl/curl.h>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace entropy {

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total);
    return total;
}

QRNGHttpClient::QRNGHttpClient(const std::string& base_url, int timeout_ms)
    : base_url_(base_url)
    , timeout_ms_(timeout_ms) {
    curl_handle_ = curl_easy_init();
}

QRNGHttpClient::~QRNGHttpClient() {
    if (curl_handle_) {
        curl_easy_cleanup(static_cast<CURL*>(curl_handle_));
    }
}

bool QRNGHttpClient::fetch_random(uint64_t* data, size_t count) {
    if (!curl_handle_) {
        return false;
    }

    size_t bytes_needed = count * 8;
    std::string url = base_url_ + "/qrng?length=" + std::to_string(bytes_needed) + "&format=HEX";

    std::string response;
    CURL* curl = static_cast<CURL*>(curl_handle_);

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_ms_ / 1000);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        return false;
    }

    // Parse JSON response
    size_t qrn_pos = response.find("\"qrn\":\"");
    if (qrn_pos == std::string::npos) {
        return false;
    }
    qrn_pos += 7;

    size_t qrn_end = response.find("\"", qrn_pos);
    if (qrn_end == std::string::npos) {
        return false;
    }

    std::string hex_str = response.substr(qrn_pos, qrn_end - qrn_pos);

    // Convert hex to bytes
    if (hex_str.size() < bytes_needed * 2) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        uint64_t value = 0;
        for (int j = 0; j < 8; ++j) {
            char byte_str[3] = {hex_str[(i * 8 + j) * 2], hex_str[(i * 8 + j) * 2 + 1], 0};
            value = (value << 8) | static_cast<uint64_t>(std::stoul(byte_str, nullptr, 16));
        }
        data[i] = value;
    }

    return true;
}

bool QRNGHttpClient::is_valid() const {
    return curl_handle_ != nullptr;
}

} // namespace entropy
