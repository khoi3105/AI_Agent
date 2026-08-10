#ifndef WEB_SEARCH_TOOL_H
#define WEB_SEARCH_TOOL_H

#include "../tool.h"

#include <string>
#include <nlohmann/json.hpp>

class WebSearchTool : public Tool {
public:
    WebSearchTool() = default;
    ~WebSearchTool() override = default;
    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const std::string& args) override;
    nlohmann::json get_schema() const override;
};

#endif // WEB_SEARCH_TOOL_H