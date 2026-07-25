#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <cstdlib> //std::env
#include <nlohmann/json.hpp>

// Include các file header trong dự án của bạn
#include "client/ollama_client.h"

std::string getEnvVar(const char* name, const std::string& defaultValue = "") {
    const char* val = std::getenv(name);
    return (val != nullptr) ? std::string(val) : defaultValue;
}

int main() {
    // 1. Khai báo thông tin API
    std::string model = "meta/llama-3.2-11b-vision-instruct";
    std::string base_url = "https://integrate.api.nvidia.com/v1/chat/completions";
    std::string api_key = getEnvVar("LLAMA_API_KEY");

    // 2. Khởi tạo LLM Client (Sử dụng con trỏ Lớp cơ sở - Abstraction)
    std::cout << "[1] Dang khoi tao LLM Client..." << std::endl;
    std::unique_ptr<LLMClient> client = std::make_unique<OllamaClient>(model, base_url, api_key);

    // 3. Khởi tạo ToolRegistry (Phiên bản POC đã hardcode CalculatorTool)
    std::cout << "[2] Dang khoi tao ToolRegistry..." << std::endl;
    // ToolRegistry registry;

    // 4. Chuẩn bị câu hỏi (Task) từ người dùng
    std::string user_task = "Tinh giup toi phep tinh 15 * 17";
    // std::string user_task = "thực hiện";

    std::cout << "[3] User Task: \"" << user_task << "\"" << std::endl << std::endl;
    std::vector<std::string> images_path = {
        // "build/a.jpg",
    };
    // 5. Gửi câu hỏi sang LLM Client
    std::cout << "[4] Dang gui Request sang AI API..." << std::endl;
    auto llm_response = client->chat(user_task, images_path);

    if (llm_response.has_value()) {
        std::cout << "--> AI Raw Response (String):\n" << *llm_response << std::endl;
    } else {
        std::cout << llm_response.error() << std::endl;
    }

    // 6. Bóc tách JSON response từ AI trực tiếp tại main (Thử nghiệm cho POC)
    // if (llm_response.empty()) {
    //     std::cerr << "[X] Loi: AI khong tra ve du lieu!" << std::endl;
    //     return 1;
    // }

    // try {
    //     std::cout << "[5] Dang boc tach (parse) JSON tu AI response..." << std::endl;
    //     auto j = nlohmann::json::parse(llm_response);

    //     // Lấy toán tử và 2 toán hạng từ JSON AI trả về
    //     std::string op = j.at("operator").get<std::string>();
    //     double num1 = j.at("operand_1").get<double>();
    //     double num2 = j.at("operand_2").get<double>();

    //     // Định dạng lại tham số để gửi cho CalculatorTool (ví dụ: "15 * 17")
    //     std::string tool_args = std::to_string(num1) + " " + op + " " + std::to_string(num2);

    //     std::cout << "    + Toan tu: " << op << std::endl;
    //     std::cout << "    + Tham so trich xuat: " << tool_args << std::endl << std::endl;

    //     // 7. Thực thi Tool thông qua ToolRegistry
    //     std::cout << "[6] Kich hoat CalculatorTool..." << std::endl;
    //     std::string tool_result = "helloWorld";

    //     std::cout << "\n==========================================" << std::endl;
    //     std::cout << " ==> KET QUA CUOI CUNG: " << tool_result << std::endl;
    //     std::cout << "==========================================" << std::endl;

    // } catch (const std::exception& e) {
    //     std::cerr << "[X] Loi parse JSON hoac khong dung dinh dang: " << e.what() << std::endl;
    //     std::cout << "Raw response tu AI: " << llm_response << std::endl;
    // }

    return 0;
}