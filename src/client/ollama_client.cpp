#include "ollama_client.h"
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
        cerr << "Loi khoi tao libcurl!" << endl;
        return "";
    }

    // std::string system_prompt = 
    // "Bạn là trợ lý AI có khả năng sử dụng các công cụ sau:\n" + 
    // registry.getToolDescriptions() + "\n" +
    // "Nhiệm vụ: Phân tích yêu cầu người dùng. Nếu cần dùng tool, CHỈ trả về JSON:\n"
    // "{\"tool\": \"tên_tool\", \"args\": \"tham_số\"}\n"
    // "Nếu không cần tool, trả về câu trả lời trực tiếp.";

    std::string system_prompt = R"(Bạn là trợ lý AI có khả năng sử dụng các công cụ sau:
    calculator, công cụ này nhận vào operator (+,-,*,/), 2 operand tương ứng và trả về kết quả phép tính

    Nhiệm vụ: Phân tích yêu cầu người dùng. Nếu cần dùng tool, CHỈ trả về JSON:
    {"tool": "tên_tool", "args": "tham_số"}

    Nếu không cần tool, trả về câu trả lời trực tiếp.)";
    // 1. Create payload
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