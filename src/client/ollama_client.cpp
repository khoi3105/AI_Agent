#include "ollama_client.h"
#include "../utils/base64_encoder.h"
#include "../tools/tool_registry.h"
#include "../utils/http_client.h"
#include "../config/config.h"
#include <iostream>
#include <memory>

using namespace std;

std::expected<std::string, std::string> OllamaClient::chat(nlohmann::json& message, const std::vector<std::string>& image_paths) {
    // 1. Tạo bản sao payload messages để không làm thay đổi trực tiếp conversationHistory
    nlohmann::json messages_payload = message;

    // 2. Nếu có danh sách ảnh, chèn ảnh vào tin nhắn user CUỐI CÙNG (tin nhắn mới nhất)
    if (!image_paths.empty()) {
        nlohmann::json* user_msg_ptr = nullptr;
        for (auto it = messages_payload.rbegin(); it != messages_payload.rend(); ++it) {
            if (it->contains("role") && (*it)["role"] == "user") {
                user_msg_ptr = &(*it);
                break;
            }
        }

        if (user_msg_ptr != nullptr) {
            std::string original_text = "";
            if (user_msg_ptr->contains("content")) {
                if ((*user_msg_ptr)["content"].is_string()) {
                    original_text = (*user_msg_ptr)["content"].get<std::string>();
                } else if ((*user_msg_ptr)["content"].is_array()) {
                    for (const auto& item : (*user_msg_ptr)["content"]) {
                        if (item.contains("type") && item["type"] == "text" && item.contains("text")) {
                            original_text = item["text"].get<std::string>();
                            break;
                        }
                    }
                }
            }

            // Mảng content chứa text + images theo chuẩn OpenAI / NIM API
            nlohmann::json content_array = nlohmann::json::array();
            content_array.push_back({
                {"type", "text"},
                {"text", original_text}
            });

            for (const auto& img_path : image_paths) {
                auto b64_result = Base64Encoder::encodeFile(img_path);
                if (b64_result.has_value()) {
                    std::string mime = "image/png";
                    if (img_path.ends_with(".jpg") || img_path.ends_with(".jpeg")) {
                        mime = "image/jpeg";
                    }
                    std::string base64_url = "data:" + mime + ";base64," + b64_result.value();
                    content_array.push_back({
                        {"type", "image_url"},
                        {"image_url", {
                            {"url", base64_url}
                        }}
                    });
                } else {
                    return std::unexpected(b64_result.error());
                }
            }

            (*user_msg_ptr)["content"] = content_array;
        }
    }

    nlohmann::json payload = {
        {"model", _modelName},
        {"messages", messages_payload},
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

    // 2. Gửi request qua HttpClient (Timeout 120s)
    int timeoutSec = Config::instance()->http().postTimeout;                                         
    if (timeoutSec <= 0) timeoutSec = 120;
    auto http_res = agent::utils::HttpClient::postJson(_baseURL, payload.dump(), headers, timeoutSec);

    if (!http_res.has_value()) {
        return std::unexpected(http_res.error());
    }

    std::string response_string = http_res.value();

    // 3. Parse JSON Response và trích xuất câu trả lời kèm token usage
    try {
        auto response_json = nlohmann::json::parse(response_string);

        // Trích xuất token usage
        _lastTokensUsed = 0;
        if (response_json.contains("usage") && response_json["usage"].is_object()) {
            _lastTokensUsed = response_json["usage"].value("total_tokens", 0);
            if (_lastTokensUsed == 0) {
                int prompt_tokens = response_json["usage"].value("prompt_tokens", 0);
                int comp_tokens = response_json["usage"].value("completion_tokens", 0);
                _lastTokensUsed = prompt_tokens + comp_tokens;
            }
        } else if (response_json.contains("prompt_eval_count") || response_json.contains("eval_count")) {
            _lastTokensUsed = response_json.value("prompt_eval_count", 0) + response_json.value("eval_count", 0);
        }

        std::string message_str = "";
        if (response_json.contains("choices") && !response_json["choices"].empty() &&
            response_json["choices"][0].contains("message") && response_json["choices"][0]["message"].contains("content")) {
            message_str = response_json["choices"][0]["message"]["content"].get<std::string>();
        } else if (response_json.contains("message") && response_json["message"].contains("content")) {
            message_str = response_json["message"]["content"].get<std::string>();
        } else {
            return std::unexpected("Response JSON thiếu trường 'choices' hoặc 'message'.");
        }

        if (_lastTokensUsed <= 0) {
            _lastTokensUsed = static_cast<int>((payload.dump().length() + message_str.length()) / 4);
            if (_lastTokensUsed <= 0) _lastTokensUsed = 1;
        }

        return message_str; 

    } catch (const nlohmann::json::exception& e) {
        return std::unexpected(string("Lỗi parse API JSON response: ") + e.what());
    }
}