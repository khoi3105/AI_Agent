#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include "tool.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <expected>

class ToolRegistry {
public:
    using Factory = std::function<std::unique_ptr<Tool>()>;
    void registerBuiltInTools();
    void registerGuiTools();
    ToolRegistry();
    std::expected<void, std::string>
    registerTool(const std::string& name, Factory factory);
    std::expected<void, std::string>
    unregisterTool(const std::string& name);
    std::expected<std::unique_ptr<Tool>, std::string> getTool(const std::string& name) const;
    std::expected<std::string, std::string> execute(const std::string& name, const nlohmann::json& args) const;
    nlohmann::json getAllSchemas() const;
private:
    std::unordered_map<std::string, Factory> _factories;
};

#endif