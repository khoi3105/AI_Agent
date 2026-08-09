#include "tool_registry.h"

#include "calculator_tool/calculator_tool.h"
#include "read_file_tool/read_file_tool.h"
#include "write_file_tool/write_file_tool.h"
#include "exec_tool/exec_tool.h"
#include <stdexcept>

void ToolRegistry::registerBuiltInTools() {
    //Calculator
    registerTool(
        "calculator",
        []()
        {
            return std::make_unique<CalculatorTool>();
        });
    //ReadFile
    registerTool(
        "read_file",
        []()
        {
            return std::make_unique<ReadFileTool>();
        }); 
    //WriteFile
    registerTool(
        "write_file",
        []()
        {
            return std::make_unique<WriteFileTool>();
        });
    //Exec
    registerTool(
        "exec",
        []()
        {
            return std::make_unique<ExecTool>();
        });
}

ToolRegistry::ToolRegistry(){
    registerBuiltInTools();
}

std::expected<void, std::string> ToolRegistry::registerTool(const std::string& name, Factory factory) {
    if (_factories.contains(name))
        return std::unexpected("Tool đã có sẵn!");
    _factories[name] = std::move(factory);
    return {};
}

std::expected<void, std::string> ToolRegistry::unregisterTool(const std::string& name){
    if (!_factories.contains(name))
        return std::unexpected("Không tìm thấy tool " + name);
    _factories.erase(name);
    return {};
}

std::expected<std::unique_ptr<Tool>, std::string> ToolRegistry::getTool(const std::string& name) const {
    auto it = _factories.find(name);
    if (it == _factories.end())
        return std::unexpected("Tool not found.");
    return it->second();
}

std::expected<std::string, std::string> ToolRegistry::execute(const std::string& name, const std::string& args) const {
    auto tool = getTool(name);
    if (!tool) return std::unexpected(tool.error());
    return tool.value()->execute(args);
}

nlohmann::json ToolRegistry::getAllSchemas() const {
    nlohmann::json schemas = nlohmann::json::array();
    for (const auto& [name, factory] : _factories) {
        auto tool = factory();          // tạo Tool
        schemas.push_back(tool->get_schema());
    }
    return schemas;
}