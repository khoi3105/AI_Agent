#include "tool_registry.h"
ToolRegistry::ToolRegistry() {
    // Hardcode tạo sẵn CalculatorTool
    CalculatorTool* calc = new CalculatorTool();
    tools[calc->getName()] = calc;
}

ToolRegistry::~ToolRegistry() {
    for (auto& pair : tools) {
        delete pair.second;
    }
    tools.clear();
}

std::expected<void, std::string> ToolRegistry::registerTool(std::shared_ptr<Tool> tool)
{
    if (!tool) return std::unexpected("Tool rỗng.");
    std::string name = tool->getName();
    if (tools.contains(name)) return std::unexpected("Tool đã tồn tại.");
    tools.emplace(name, std::move(tool));
    return {};
}

std::expected<void, std::string> ToolRegistry::unregisterTool(const std::string& name)
{
    auto it = tools.find(name);
    if (it == tools.end())
        return std::unexpected("Không tìm thấy tool." + name);
    tools.erase(it);
    return {};
}

Tool* ToolRegistry::getTool(const std::string& name) {
    if (tools.find(name) != tools.end()) {
        return tools[name];
    }
    return nullptr;
}

std::string ToolRegistry::executeTool(const std::string& name, const std::string& args) {
    Tool* tool = getTool(name);
    if (tool != nullptr) {
        return tool->execute(args);
    }
    return "Lỗi: Không tìm thấy tool " + name;
}