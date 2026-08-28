#include "ollama_client.h"
#include "../utils/base64_encoder.h"
#include "../tools/tool_registry.h"
#include "../utils/http_client.h"
#include "../config/config.h"
#include <iostream>
#include <memory>

using namespace std;

std::expected<std::string, std::string> OllamaClient::chat(nlohmann::json& message, const std::vector<std::string>& image_paths) {
    // Nếu có danh sách ảnh, tiến hành chèn ảnh vào tin nhắn của user
    // std::cout << "[DEBUG] image_paths size = " << image_paths.size() << std::endl;
    if (!image_paths.empty()) {
        // 1. Tìm phần tử tin nhắn của User trong mảng message (thường là phần tử có "role": "user")
        nlohmann::json* user_msg_ptr = nullptr;
        for (auto& msg : message) {
            if (msg.contains("role") && msg["role"] == "user") {
                user_msg_ptr = &msg;
                break;
            }
        }

        // Nếu tìm thấy tin nhắn user, thực hiện biến đổi content sang dạng multimodal array
        if (user_msg_ptr != nullptr) {
            std::string original_text = "";
            if (user_msg_ptr->contains("content") && (*user_msg_ptr)["content"].is_string()) {
                original_text = (*user_msg_ptr)["content"].get<std::string>();
            }

            // Mảng content chứa text + images theo chuẩn OpenAI / NIM API
            nlohmann::json content_array = nlohmann::json::array();

            // Push prompt text hiện tại
            content_array.push_back({
                {"type", "text"},
                {"text", original_text}
            });

            // Push từng ảnh đã encode Base64
            for (const auto& img_path : image_paths) {
                auto b64_result = Base64Encoder::encodeFile(img_path);
                if (b64_result.has_value()) {
                    std::string base64_url = "data:image/jpeg;base64," + b64_result.value();
                    content_array.push_back({
                        {"type", "image_url"},
                        {"image_url", {
                            {"url", base64_url}
                        }}
                    });
                } else {
                    // Trả về lỗi nếu đọc/mã hóa file ảnh thất bại
                    return std::unexpected(b64_result.error());
                }
            }

            // Gán lại content đã được cập nhật thành mảng cho tin nhắn user
            (*user_msg_ptr)["content"] = content_array;
        }
    }

    nlohmann::json payload = {
        {"model", _modelName},
        {"messages", message},
        {"temperature", Config::instance()->llm().temperature},
        {"top_p", Config::instance()->llm().topP},
        {"max_tokens", Config::instance()->llm().maxTokens},
        {"stream", Config::instance()->llm().stream}
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
    // cout << response_string << endl;

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