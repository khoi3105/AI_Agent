#include "ollama_client.h"
#include "../utils/base64_encoder.h"
#include "../tools/tool.h"
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

string OllamaClient::chat(const string& user_prompt) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();

    if (!curl) {
        curl_global_cleanup();
        return std::unexpected("Lỗi khởi tạo session curl easy!");
    }

    

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

    // Lấy Schema ( Không hardcode ) 
    // nlohmann::json tools_schema = registry.get_all_schemas();

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

    std::string system_prompt = R"(Bạn là một Trợ lý AI hệ thống thông minh, hoạt động theo cơ chế chọn lọc công cụ chính xác.

    === DANH SÁCH CÔNG CỤ CÓ SẴN (TOOLS) ===
    - calculator: Thực hiện các phép tính số học (cộng, trừ, nhân, chia) trên các con số cụ thể.
    {{TOOLS_SCHEMA_PLACEHOLDER}}

    === QUY TẮC XỬ LÝ ĐẦU VÀO ===
    Bạn cần kiểm tra ý định của người dùng và tuân thủ chặt chẽ 2 định dạng đầu ra sau:

    1. ĐỊNH DẠNG 1: GỌI CÔNG CỤ (Khi và chỉ khi yêu cầu chứa phép tính toán số học cụ thể)
    JSON Output:
    {
    "type": "tool_call",
    "tool": "<tên_tool>",
    "args": { <các_tham_số> }
    }

    2. ĐỊNH DẠNG 2: TRẢ LỜI TRỰC TIẾP (Mặc định cho mọi câu hỏi kiến thức, trò chuyện, văn bản)
    JSON Output:
    {
    "type": "response",
    "text": "<nội_dung_trả_lời_dựa_trên_kiến_thức_của_bạn>"
    }

    === RÀNG BUỘC LOẠI TRỪ NGHIÊM NGẶT (NEGATIVE CONSTRAINTS) ===
    - KHÔNG gọi tool 'calculator' nếu câu hỏi KHÔNG chứa số liệu hoặc KHÔNG có yêu cầu tính toán rõ ràng.
    - KHÔNG tự bịa ra các con số hoặc phép tính ngẫu nhiên (như 10 + 5) khi người dùng hỏi các câu hỏi chữ/kiến thức (như nhân vật, địa danh, trò chuyện).
    - KHÔNG gán cả biểu thức toán học phức tạp vào một tham số đơn lẻ; hãy tách thành từng bước tính hoặc từng tham số số học cụ thể.
    - KHÔNG trả về văn bản tự do ngoài cấu trúc JSON quy định.

    === CƠ CHẾ TRẢ LỜI KIẾN THỨC ===
    - Đối với các câu hỏi về nhân vật, khái niệm, kiến thức chung (như nhân vật hoạt hình, lịch sử, khoa học...): Hãy sử dụng kiến thức có sẵn của bạn để trả lời ngắn gọn, chính xác trong ĐỊNH DẠNG 2.
    - Chỉ trả lời "Hiện tại tôi chưa có đủ thông tin về vấn đề này" nếu đó là một thông tin riêng tư, mật hoặc thực sự nằm ngoài tri thức của bạn.)";

    nlohmann::json payload = {
        {"model", _modelName},
        {"messages", nlohmann::json::array({
            {
                {"role", "system"},
                {"content", system_prompt
                }
            },
            {
                {"role", "user"},
                {"content", user_prompt}
            }
        })},
        {"response_format", {{"type", "json_object"}}},
        {"max_tokens", 128},
        {"temperature", 0.00}
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

    // 4. Exec & Extract Content string
    string content_str = "";
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        try {
            auto response_json = nlohmann::json::parse(response_string);
            content_str = response_json["choices"][0]["message"]["content"];
        } catch (const exception& e) {
            cerr << "Loi parse API JSON response: " << e.what() << std::endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // cout << content_str << endl;

    return content_str; // Trả về duy nhất chuỗi text JSON do AI sinh ra
}