#include "calculator_tool.h"
std::string CalculatorTool::getName() const {
    return "calculator";
}

std::string CalculatorTool::getDescription() const{
    return "A simple calculator tool for performing basic arithmetic operations.";
}

std::string CalculatorTool::execute(const std::string& args) {
    auto result = evaluate(args);
    if (!result) {
        return "Calculator Error:\n" + result.error();
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
            oss << "Error at position " << err.token.position
                << ": " << err.diagnostic << '\n';
        }
        return std::unexpected(oss.str());
    }
    return std::to_string(expression.value());
}