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

std::string CalculatorTool::execute(const std::string& args) {
    auto result = evaluate(args);
    if (!result) {
        return "Phép tính lỗi: " + result.error();
    }
    return *result;
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
        {"type", "function"},
        {"function", {
            {"name", getName()},
            {"description", getDescription()},
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
    };
}