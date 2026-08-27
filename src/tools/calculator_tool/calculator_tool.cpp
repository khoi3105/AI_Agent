#include "calculator_tool.h"
#include <sstream>
#include <expected>
#include <cmath>

std::string CalculatorTool::getName() const {
    return "calculator";
}

std::string CalculatorTool::getDescription() const{
    return "Evaluates arithmetic expressions provided as infix strings with support for parentheses and the operators +, -, *, and /, returning the computed numeric result.";
}

std::string CalculatorTool::execute(const nlohmann::json& args) {
    if (!args.contains("expression") || !args["expression"].is_string()) {
        return "[Lỗi Calculator]: Thiếu hoặc sai định dạng tham số 'expression'.";
    }

    std::string expression = args["expression"].get<std::string>();
    
    auto result = evaluate(expression); // Hàm tự viết hoặc gọi thư viện tính toán
    if (!result.has_value()) {
        return "Phép tính lỗi: " + result.error();
    }
    return result.value();
}

std::expected<std::string, std::string> CalculatorTool::evaluate(const std::string& expr)
{
    exprtk::expression<double> expression;
    exprtk::parser<double> parser;
    if (!parser.compile(expr, expression))
    {
        std::ostringstream oss;
        for (std::size_t i = 0; i < parser.error_count(); ++i)
        {
            auto err = parser.get_error(i);
            oss << "Lỗi tại vị trí: " << err.token.position
                << ": " << err.diagnostic << '\n';
        }
        return std::unexpected(oss.str());
    }
    double result = expression.value();
    if (std::isnan(result) || std::isinf(result)){
        return std::unexpected("Không xác định");
    }
    return std::to_string(expression.value());
}

nlohmann::json CalculatorTool::get_schema() const {
    return {
        {"type", "tool_call"},
        {"tool_call", {
            {"name", getName()},
            {"description", "Đánh giá biểu thức toán học phức tạp bằng ExprTk (hỗ trợ +, -, *, /, mũ ^, ngoặc (), hàm lượng giác, v.v.)"},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"expression", {
                        {"type", "string"},
                        {"description", "Biểu thức toán học cần tính toán, ví dụ: '3.5 * (2 + 4.5)' hoặc 'sqrt(16) + sin(3.14)'"}
                    }}
                }},
                {"required", {"expression"}}
            }}
        }}
    };
}