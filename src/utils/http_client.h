#pragma once
#include <string>
#include <vector>
#include <expected>
#include <map>

namespace agent::utils {

class HttpClient {
public:
    // Cung cấp hàm POST hỗ trợ truyền Headers tuỳ chỉnh
    static std::expected<std::string, std::string> postJson(
        const std::string& url, 
        const std::string& json_payload,
        const std::vector<std::string>& extra_headers = {},
        long timeout_seconds = 60
    );

    // Hàm GET tiện ích cho Search Tool
    static std::expected<std::string, std::string> get(
        const std::string& url,
        const std::vector<std::string>& extra_headers = {},
        long timeout_seconds = 10
    );
};

} // namespace agent::utils