#include "exprtk.hpp"
#include <expected>
#include <sstream>

#include "calculator_tool.h"
// Merge thêm hàm này vào
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