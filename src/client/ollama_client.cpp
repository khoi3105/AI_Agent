#include "ollama_client.h"
#include "../utils/base64_encoder.h"
#include "../tools/tool_registry.h"
#include "../utils/http_client.h"
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

using namespace std;

std::expected<std::string, std::string> OllamaClient::chat(const std::string& user_prompt, const std::vector<std::string>& image_paths) {

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
                return std::unexpected(b64_result.error());
            }
        }

        user_msg = {
            {"role", "user"},
            {"content", content_array}
        };
    }

    // Lấy Schema ( Không hardcode ) 
    ToolRegistry registry;
    nlohmann::json tools_schema = registry.get_all_schemas();

    std::string system_prompt = R"(Bạn là một Trợ lý AI hệ thống thông minh, hoạt động theo cơ chế chọn lọc công cụ chính xác.

    === DANH SÁCH CÔNG CỤ ĐƯỢC PHÉP SỬ DỤNG (JSON SCHEMA) ===
    )" + tools_schema.dump(2) + R"(

    === QUY TẮC XỬ LÝ ĐẦU VÀO ===
    Bạn cần kiểm tra ý định của người dùng và tuân thủ chặt chẽ 2 định dạng đầu ra sau:

    1. ĐỊNH DẠNG 1: GỌI CÔNG CỤ (Sử dụng khi và chỉ khi câu hỏi yêu cầu thực thi hoặc tính toán liên quan đến các công cụ trong danh sách trên)
    JSON Output:
    {
    "type": "tool_call",
    "tool": "<tên_tool_chính_xác_trong_schema>",
    "args": { <các_tham_số_đúng_định_dạng_properties_trong_schema> }
    }

    2. ĐỊNH DẠNG 2: TRẢ LỜI TRỰC TIẾP (Mặc định cho mọi câu hỏi kiến thức, trò chuyện, hoặc khi KHÔNG CÓ công cụ nào phù hợp)
    JSON Output:
    {
    "type": "response",
    "text": "<nội_dung_trả_lời_dựa_trên_kiến_thức_của_bạn>"
    }

    === RÀNG BUỘC LOẠI TRỪ NGHIÊM NGẶT (NEGATIVE CONSTRAINTS) ===
    - KHÔNG tự ý gọi công cụ nếu câu hỏi KHÔNG chứa số liệu hoặc KHÔNG có yêu cầu xử lý rõ ràng liên quan đến công cụ đó.
    - KHÔNG tự bịa ra các con số, tên công cụ hoặc phép tính ngẫu nhiên khi người dùng hỏi các câu hỏi kiến thức thông thường.
    - KHÔNG trả về văn bản tự do ngoài cấu trúc JSON quy định.
    - Khi sử dụng ĐỊNH DẠNG 1, trường "args" phải là một JSON Object chứa các key-value đúng theo định dạng 'properties' khai báo trong Schema của công cụ đó.)";

    nlohmann::json payload = {
        {"model", _modelName},
        {"messages", nlohmann::json::array({
            {{"role", "system"}, {"content", system_prompt}},
            // Câu hỏi thực tế của User
            user_msg
        })},
        // {"tools", tools_schema},
        // {"tool_choice", "auto"},
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