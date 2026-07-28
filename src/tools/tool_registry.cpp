#include "tool_registry.h"
#include <stdexcept>

ToolRegistry::ToolRegistry(){
    auto result = registerTool(std::make_shared<CalculatorTool>());
    if (!result)
        throw std::runtime_error(result.error());
}

ToolRegistry::~ToolRegistry(){
    _tools.clear();
}

std::expected<void, std::string> ToolRegistry::registerTool(std::shared_ptr<Tool> tool)
{
    if (!tool) return std::unexpected("Tool rỗng.");
    std::string name = tool->getName();
    if (_tools.contains(name)) return std::unexpected("Tool đã tồn tại.");
    _tools.emplace(name, std::move(tool));
    return {};
}

std::expected<void, std::string> ToolRegistry::unregisterTool(const std::string& name)
{
    auto it = _tools.find(name);
    if (it == _tools.end()) return std::unexpected("Không tìm thấy tool." + name);
    _tools.erase(it);
    return {};
}

Tool* ToolRegistry::getTool(const std::string& name){
    auto it = _tools.find(name);
    if (it != _tools.end())
        return it->second.get();
    return nullptr;
}

std::string ToolRegistry::executeTool(const std::string& name, const std::string& args) {
    Tool* tool = getTool(name);
    if (tool != nullptr) {
        return tool->execute(args);
    }
    return "Lỗi: Không tìm thấy tool " + name;
}

nlohmann::json ToolRegistry::get_all_schemas() const {
    auto schemas = nlohmann::json::array();
    for (const auto& [name, tool] : _tools) {
        schemas.push_back(tool->get_schema());
    }
    return schemas;
}