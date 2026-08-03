#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <format>

#include "AgentLoop.h"
#include "tool_call_parser.h"
#include "../tools/tool_registry.h"

std::expected<std::string, std::string> AgentLoop::run(const std::string& user_task, const std::shared_ptr<LLMClient>& client, const std::vector<std::string>& image_paths){ 
    ToolRegistry registry;
    nlohmann::json tools_schema = registry.get_all_schemas();

    _conversationHistory.clear();

    std::string system_prompt = R"(Bạn là một Trợ lý AI hệ thống thông minh, hoạt động theo cơ chế chọn lọc công cụ chính xác.

    === DANH SÁCH CÔNG CỤ ĐƯỢC PHÉP SỬ DỤNG (JSON SCHEMA) ===
    )" + tools_schema.dump(2) + R"(

    === QUY TẮC RÀNG BUỘC NGHIÊM NGẶT (CRITICAL RULES) ===
    1. MỖI LƯỢT CHỈ ĐƯỢC TRẢ VỀ DUY NHẤT 01 KHỐI JSON. KHÔNG VIẾT BẤT KỲ LỜI VĂN MỞ ĐẦU HAY GIẢI THÍCH NÀO KHÁC.
    2. KHÔNG TỰ TÍNH TOÁN HAY GIẢ LẬP KẾT QUẢ TOOL. Nếu tác vụ có phép tính hoặc tra cứu, BẮT BUỘC trả về "tool_call" cho bước đó.
    3. Nếu người dùng hỏi nhiều câu: Hãy gọi 01 Tool cho câu hỏi đầu tiên. Sau khi nhận được kết quả (Observation), bạn mới tiếp tục gọi Tool cho câu tiếp theo hoặc tổng hợp câu trả lời.
    
    Bạn cần kiểm tra ý định của người dùng và tuân thủ chặt chẽ 2 định dạng đầu ra sau:

    1. ĐỊNH DẠNG 1: GỌI CÔNG CỤ (Sử dụng khi và chỉ khi câu hỏi yêu cầu thực thi hoặc tính toán liên quan đến các công cụ trong danh sách trên)
    {
    "type": "tool_call",
    "tool": "<tên_tool_chính_xác_trong_schema>",
    "args": { <các_tham_số_đúng_định_dạng_properties_trong_schema> }
    }

    2. ĐỊNH DẠNG 2: TRẢ LỜI TRỰC TIẾP (Mặc định cho mọi câu hỏi kiến thức, trò chuyện, hoặc khi KHÔNG CÓ công cụ nào phù hợp)
    {
    "type": "response",
    "text": "<nội_dung_trả_lời_dựa_trên_kiến_thức_của_bạn>"
    }

    === RÀNG BUỘC NGHIÊM NGẶT (CRITICAL RULES) ===
    - KHÔNG TỰ TÍNH TOÁN HAY GIẢI BÀI TOÁN. Khi phát hiện phép tính, PHẢI tạo "tool_call" ngay lập tức để Tool xử lý.
    - KHÔNG viết lời mở đầu, KHÔNG giải thích các bước tính toán (ví dụ: "Tôi sẽ dùng calculator...", "20 + 160 = 180...").
    )";

    // Lưu lại lịch sử 
    _conversationHistory.push_back({{"role", "system"}, {"content", system_prompt }});
    _conversationHistory.push_back({{"role", "user"}, {"content", user_task}});

    int step = 0;
    ToolCallRequest request;
    while ( step < AgentLoop::MAXSTEP ) {  
        step++;

        std::expected<std::string,std::string> llm_response = client->chat(_conversationHistory, image_paths);
        if (llm_response.has_value()) {
            std::cout << std::format("--> Cau tra loi goc tu AI (Buoc {}):\n{}\n", step, *llm_response);
        } else {
            return std::unexpected(std::format("[ERROR]: Khong nhan phan hoi tu LLM - {} !", llm_response.error()));
        }

        request = ToolCallParser::parse(*llm_response);
        _conversationHistory.push_back({{"role","assistant"},{"content",*llm_response}});

        if (request.is_valid && request.tool_name != "null" && !request.tool_name.empty()) {

            std::cout << "[Act]: Goi cong cu '" << request.tool_name << "'...\n";

            std::string tool_result = registry.executeTool(request.tool_name,request.args["expression"].get<std::string>());
            std::cout << "[Observe]: Ket qua Tool -> " << tool_result << std::endl;

            // Đưa kết quả Tool (Observation) ngược lại hội thoại cho LLM đọc ở bước tiếp theo
            std::string observation_msg = std::format("Ket qua tu cong cu '{}': {}", request.tool_name, tool_result);
            _conversationHistory.push_back({{"role", "user"}, {"content", observation_msg}});

            continue; 
        }

        break;
    }

    if (step >= AgentLoop::MAXSTEP) {
        return std::unexpected("[ERROR]: Da dat so buoc toi da!");
    }

    try {
        // Trường hợp 1: args là JSON Object chứa key "text"
        if (request.args.is_object() && request.args.contains("text")) {
            return std::format("[4] Tra loi: {}\n", request.args["text"].get<std::string>());
        }
        // Trường hợp 2: args đã được parser đưa về dạng string
        else if (request.args.is_string()) {
            return std::format("[4] Tra loi: {}\n", request.args.get<std::string>());
        }
    }
    catch (const nlohmann::json::exception& e) {
        return std::unexpected(std::format("[JSON Exception - Output Fallback]: {}", e.what()));
    }

    // Fallback: Lấy chuỗi thô cuối cùng của AI trong lịch sử nếu parse không khớp định dạng
    if (!_conversationHistory.empty()) {
        std::string raw_fallback = _conversationHistory.back()["content"].get<std::string>();
        return std::format("[4] Tra loi: {}\n", raw_fallback);
    }

    return std::unexpected("[ERROR]: Lịch sử hội thoại rỗng, không thể lấy phản hồi.");
}