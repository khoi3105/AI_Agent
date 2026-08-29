#include "gemini_client.h"
#include "../utils/base64_encoder.h"
#include "../utils/http_client.h"
#include "../config/config.h"
#include <iostream>
#include <algorithm>

using namespace std;

GeminiClient::GeminiClient(std::string modelName, std::string baseURL, std::string APIKey)
    : _modelName(std::move(modelName)), _baseURL(std::move(baseURL)), _APIKey(std::move(APIKey)) {
    if (_modelName.empty()) {
        _modelName = "gemini-2.0-flash";
    }
    if (_baseURL.empty()) {
        _baseURL = "https://generativelanguage.googleapis.com/v1beta/models";
    }
}

std::string GeminiClient::getMimeType(const std::string& filepath) {
    std::string ext = "";
    auto dot_pos = filepath.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = filepath.substr(dot_pos);
        for (auto& c : ext) c = std::tolower(c);
    }
    if (ext == ".png") return "image/png";
    if (ext == ".webp") return "image/webp";
    if (ext == ".gif") return "image/gif";
    if (ext == ".bmp") return "image/bmp";
    return "image/jpeg";
}

std::expected<std::string, std::string> GeminiClient::chat(nlohmann::json& message, const std::vector<std::string>& image_paths) {
    // Nếu URL chứa chat/completions -> Sử dụng OpenAI-compatible mode
    if (_baseURL.find("chat/completions") != std::string::npos) {
        return chatOpenAICompatible(message, image_paths);
    }
    
    // Mặc định: Sử dụng Google Gemini Native REST API (generateContent)
    return chatNative(message, image_paths);
}

std::expected<std::string, std::string> GeminiClient::chatNative(nlohmann::json& message, const std::vector<std::string>& image_paths) {
    // 1. Xây dựng Endpoint URL
    std::string request_url = _baseURL;
    if (request_url.find(":generateContent") == std::string::npos) {
        if (request_url.back() == '/') {
            request_url.pop_back();
        }
        if (request_url.find(_modelName) == std::string::npos) {
            request_url += "/" + _modelName;
        }
        request_url += ":generateContent";
    }

    // 2. Chuyển đổi OpenAI Messages sang chuẩn Gemini Contents & System Instruction
    nlohmann::json contents = nlohmann::json::array();
    nlohmann::json system_instruction_parts = nlohmann::json::array();

    for (const auto& msg : message) {
        if (!msg.contains("role") || !msg.contains("content")) continue;

        std::string role = msg["role"].get<std::string>();
        std::string text_content = "";
        
        if (msg["content"].is_string()) {
            text_content = msg["content"].get<std::string>();
        } else if (msg["content"].is_array()) {
            for (const auto& item : msg["content"]) {
                if (item.contains("type") && item["type"] == "text" && item.contains("text")) {
                    text_content += item["text"].get<std::string>() + "\n";
                }
            }
        }

        if (role == "system") {
            system_instruction_parts.push_back({{"text", text_content}});
        } else {
            std::string gemini_role = (role == "assistant") ? "model" : "user";
            nlohmann::json parts = nlohmann::json::array();
            parts.push_back({{"text", text_content}});
            
            contents.push_back({
                {"role", gemini_role},
                {"parts", parts}
            });
        }
    }

    // 3. Nếu có ảnh, chèn vào message role user cuối cùng dưới dạng inline_data
    if (!image_paths.empty() && !contents.empty()) {
        for (auto it = contents.rbegin(); it != contents.rend(); ++it) {
            if ((*it)["role"] == "user") {
                for (const auto& img_path : image_paths) {
                    auto b64_res = Base64Encoder::encodeFile(img_path);
                    if (!b64_res.has_value()) {
                        return std::unexpected("Lỗi đọc file ảnh '" + img_path + "': " + b64_res.error());
                    }
                    (*it)["parts"].push_back({
                        {"inline_data", {
                            {"mime_type", getMimeType(img_path)},
                            {"data", b64_res.value()}
                        }}
                    });
                }
                break;
            }
        }
    }

    // 4. Xây dựng JSON payload
    nlohmann::json payload = {
        {"contents", contents},
        {"generationConfig", {
            {"temperature", Config::instance()->llm().temperature},
            {"topP", Config::instance()->llm().topP},
            {"maxOutputTokens", Config::instance()->llm().maxTokens}
        }}
    };

    if (!system_instruction_parts.empty()) {
        payload["system_instruction"] = {
            {"parts", system_instruction_parts}
        };
    }

    // 5. Chuẩn bị headers
    std::vector<std::string> headers = {
        "Content-Type: application/json"
    };
    if (!_APIKey.empty()) {
        headers.push_back("x-goog-api-key: " + _APIKey);
    }

    // 6. Gửi request qua HttpClient
    auto http_res = agent::utils::HttpClient::postJson(request_url, payload.dump(), headers);
    if (!http_res.has_value()) {
        return std::unexpected(http_res.error());
    }

    // 7. Parse kết quả trả về từ Gemini
    try {
        auto response_json = nlohmann::json::parse(http_res.value());

        if (response_json.contains("error")) {
            std::string err_msg = response_json["error"].contains("message") 
                ? response_json["error"]["message"].get<std::string>() 
                : response_json["error"].dump();
            return std::unexpected("Gemini API Error: " + err_msg);
        }

        if (!response_json.contains("candidates") || response_json["candidates"].empty()) {
            return std::unexpected("Gemini API response thiếu 'candidates' hoặc rỗng.");
        }

        const auto& candidate = response_json["candidates"][0];
        if (!candidate.contains("content") || !candidate["content"].contains("parts") || candidate["content"]["parts"].empty()) {
            return std::unexpected("Gemini candidate thiếu 'parts' text nội dung.");
        }

        std::string result = "";
        for (const auto& part : candidate["content"]["parts"]) {
            if (part.contains("text")) {
                result += part["text"].get<std::string>();
            }
        }
        return result;

    } catch (const std::exception& e) {
        return std::unexpected(std::string("Lỗi parse Gemini JSON response: ") + e.what());
    }
}

std::expected<std::string, std::string> GeminiClient::chatOpenAICompatible(nlohmann::json& message, const std::vector<std::string>& image_paths) {
    if (!image_paths.empty()) {
        nlohmann::json* user_msg_ptr = nullptr;
        for (auto& msg : message) {
            if (msg.contains("role") && msg["role"] == "user") {
                user_msg_ptr = &msg;
                break;
            }
        }

        if (user_msg_ptr != nullptr) {
            std::string original_text = "";
            if (user_msg_ptr->contains("content") && (*user_msg_ptr)["content"].is_string()) {
                original_text = (*user_msg_ptr)["content"].get<std::string>();
            }

            nlohmann::json content_array = nlohmann::json::array();
            content_array.push_back({
                {"type", "text"},
                {"text", original_text}
            });

            for (const auto& img_path : image_paths) {
                auto b64_result = Base64Encoder::encodeFile(img_path);
                if (b64_result.has_value()) {
                    std::string base64_url = "data:" + getMimeType(img_path) + ";base64," + b64_result.value();
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
        {"messages", message},
        {"temperature", Config::instance()->llm().temperature},
        {"top_p", Config::instance()->llm().topP},
        {"max_tokens", Config::instance()->llm().maxTokens},
        {"stream", false}
    };

    std::vector<std::string> headers = {
        "Content-Type: application/json"
    };
    if (!_APIKey.empty()) {
        headers.push_back("Authorization: Bearer " + _APIKey);
    }

    auto http_res = agent::utils::HttpClient::postJson(_baseURL, payload.dump(), headers);
    if (!http_res.has_value()) {
        return std::unexpected(http_res.error());
    }

    try {
        auto response_json = nlohmann::json::parse(http_res.value());
        if (response_json.contains("error")) {
            std::string err_msg = response_json["error"].contains("message") 
                ? response_json["error"]["message"].get<std::string>() 
                : response_json["error"].dump();
            return std::unexpected("Gemini API Error: " + err_msg);
        }

        if (!response_json.contains("choices") || response_json["choices"].empty()) {
            return std::unexpected("Response JSON thiếu trường 'choices' hoặc rỗng.");
        }

        return response_json["choices"][0]["message"]["content"].get<std::string>();

    } catch (const std::exception& e) {
        return std::unexpected(std::string("Lỗi parse OpenAI JSON response: ") + e.what());
    }
}
