#include "ollama_client.h"
#include "../utils/base64_encoder.h"
#include "../tools/tool_registry.h"
#include "../utils/http_client.h"
#include <iostream>
#include <memory>

using namespace std;

std::expected<std::string, std::string> OllamaClient::chat(const nlohmann::json& message, const std::vector<std::string>& image_paths) {

    nlohmann::json user_msg;
    // if (image_paths.empty()) {

        //TODO: NEU CO ANH CHI APPEND VAO message, khong tao moi

        // Nếu không có ảnh, content chỉ cần là string đơn thuần
    //     user_msg = {
    //         {"role", "user"},
    //         {"content", user_prompt}
    //     };
    // } else {
    //     // Nếu có ảnh, content sẽ là mảng chứa cả text và các image_url
    //     nlohmann::json content_array = nlohmann::json::array();
        
    //     // 1. Thêm prompt text vào mảng
    //     content_array.push_back({
    //         {"type", "text"},
    //         {"text", user_prompt}
    //     });

    //     // 2. Thêm từng ảnh base64 vào mảng theo chuẩn OpenAI / NIM
    //     for (const auto& img_path : image_paths) {
    //         auto b64_result = Base64Encoder::encodeFile(img_path);
    //         if (b64_result.has_value()) {
    //             // Giả định định dạng ảnh (jpeg/png). Bạn có thể đổi sang png nếu dùng png.
    //             std::string base64_url = "data:image/jpeg;base64," + b64_result.value();
                
    //             content_array.push_back({
    //                 {"type", "image_url"},
    //                 {"image_url", {
    //                     {"url", base64_url}
    //                 }}
    //             });
    //         } else {
    //             return std::unexpected(b64_result.error());
    //         }
    //     }

    //     user_msg = {
    //         {"role", "user"},
    //         {"content", content_array}
    //     };
    // }

    nlohmann::json payload = {
        {"model", _modelName},
        {"messages", message},
        {"temperature", 0.1},
        {"top_p", 1.0},
        {"max_tokens", 16384},
        {"stream", false}
    };
    
    // 1. Chuẩn bị custom headers chứa API Key
    vector<string> headers;
    if (!_APIKey.empty()) {
        headers.push_back("Authorization: Bearer " + _APIKey);
    }

    // 2. Gửi request qua HttpClient (1 DÒNG DUY NHẤT!)
    auto http_res = agent::utils::HttpClient::postJson(_baseURL, payload.dump(), headers);

    if (!http_res.has_value()) {
        return std::unexpected(http_res.error());
    }

    std::string response_string = http_res.value();

    // 6. Parse JSON Response và bắt lỗi định dạng
    try {
        auto response_json = nlohmann::json::parse(response_string);

        // Kiểm tra xem trường dữ liệu mong muốn có tồn tại không
        if (!response_json.contains("choices") || response_json["choices"].empty()) {
            return std::unexpected("Response JSON thiếu trường 'choices' hoặc rỗng.");
        }

        std::string message_str = response_json["choices"][0]["message"]["content"];
        
        // Thành công: Trả về trực tiếp chuỗi kết quả
        return message_str; 

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(string("Lỗi parse API JSON response: ") + e.what());
    }
}