#include "ollama_client.h"
#include "../utils/base64_encoder.h"
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using namespace std;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    string* response = static_cast<string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::expected<std::string, std::string> OllamaClient::chat(const std::string& user_prompt, const std::vector<std::string>& image_paths) {
    // 0. Khởi tạo libcurl
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return std::unexpected("Lỗi khởi tạo libcurl global!");
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return std::unexpected("Lỗi khởi tạo session curl easy!");
    }

    // std::string system_prompt = 
    // "Bạn là trợ lý AI có khả năng sử dụng các công cụ sau:\n" + 
    // registry.getToolDescriptions() + "\n" +
    // "Nhiệm vụ: Phân tích yêu cầu người dùng. Nếu cần dùng tool, CHỈ trả về JSON:\n"
    // "{\"tool\": \"tên_tool\", \"args\": \"tham_số\"}\n"
    // "Nếu không cần tool, trả về câu trả lời trực tiếp.";

    nlohmann::json user_msg;
    if (image_paths.empty()) {
        // Nếu không có ảnh, content chỉ cần là string đơn thuần
        user_msg = {
            {"role", "user"},
            {"content", user_prompt}
        };
    } else {
        // Nếu có ảnh, content sẽ là mảng chứa cả text và các image_url
        nlohmann::json content_array = nlohmann::json::array();
        
        // 1. Thêm prompt text vào mảng
        content_array.push_back({
            {"type", "text"},
            {"text", user_prompt}
        });

        // 2. Thêm từng ảnh base64 vào mảng theo chuẩn OpenAI / NIM
        for (const auto& img_path : image_paths) {
            auto b64_result = Base64Encoder::encodeFile(img_path);
            if (b64_result.has_value()) {
                // Giả định định dạng ảnh (jpeg/png). Bạn có thể đổi sang png nếu dùng png.
                std::string base64_url = "data:image/jpeg;base64," + b64_result.value();
                
                content_array.push_back({
                    {"type", "image_url"},
                    {"image_url", {
                        {"url", base64_url}
                    }}
                });
            } else {
                // Đừng quên dọn dẹp curl nếu return sớm giữa chừng
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return std::unexpected(b64_result.error());
            }
        }

        user_msg = {
            {"role", "user"},
            {"content", content_array}
        };
    }

    nlohmann::json tools_schema = nlohmann::json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "calculator"},
                {"description", "Thực hiện phép tính số học giữa 2 số"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"operand_1", {{"type", "number"}}},
                        {"operator", {{"type", "string"}, {"enum", {"+", "-", "*", "/"}}}},
                        {"operand_2", {{"type", "number"}}}
                    }},
                    {"required", {"operand_1", "operator", "operand_2"}}
                }}
            }}
        }
    });

    std::string system_prompt = 
    "Bạn là một trợ lý AI thông minh và hữu ích.\n"
    "Hãy sử dụng các công cụ được cung cấp khi cần thiết để trả lời câu hỏi của người dùng một cách chính xác.\n"
    "Nếu câu hỏi không yêu cầu công cụ, hãy trả lời trực tiếp bằng văn bản rõ ràng, ngắn gọn.";

    // 1. Create payload
    nlohmann::json payload = {
        {"model", _modelName},
        {"messages", nlohmann::json::array({
            {
                {"role", "system"},
                {"content", system_prompt}
            },
            user_msg
        })},
        {"tools", tools_schema},
        {"temperature", 0.1},
        {"top_p", 1.0},
        {"max_tokens", 16384},
        {"stream", false}
    };
    string json_str = payload.dump();

    // 2. Headers
    struct curl_slist* headers = nullptr;
    string auth_header = "Authorization: Bearer " + _APIKey;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    // 3. Libcurl Setup
    string response_string;
    curl_easy_setopt(curl, CURLOPT_URL, _baseURL.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    // 5. Thực thi Request
    CURLcode res = curl_easy_perform(curl);
    
    // Đảm bảo luôn dọn dẹp tài nguyên curl sau khi request xong
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // Bắt lỗi kết nối HTTP / Curl
    if (res != CURLE_OK) {
        return std::unexpected(string("Lỗi curl request: ") + curl_easy_strerror(res));
    }

    cout << response_string << endl;

    // 6. Parse JSON Response và bắt lỗi định dạng
    try {
        auto response_json = nlohmann::json::parse(response_string);

        // Kiểm tra xem trường dữ liệu mong muốn có tồn tại không
        if (!response_json.contains("choices") || response_json["choices"].empty()) {
            return std::unexpected("Response JSON thiếu trường 'choices' hoặc rỗng.");
        }

        std::string content_str = response_json["choices"][0]["message"]["content"];
        
        // Thành công: Trả về trực tiếp chuỗi kết quả
        return content_str; 

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(string("Lỗi parse API JSON response: ") + e.what());
    }
}