#include "read_file_tool.h"

std::string ReadFileTool::getName() const {
    return "read_file";
}

std::string ReadFileTool::getDescription() const {
    return "Read text from txt, csv, xml, md, json, log, ini, yaml, yml, cpp, h, py, c, cc, java, js, html, css, sql, sh, bat and pdf files.";
}
std::string ReadFileTool::execute(const std::string& args) {
    auto j = nlohmann::json::parse(args);
    auto result = _loader.load(j["path"].get<std::string>());
    // auto result = _loader.load(args);
    if (!result)
        return result.error();

    return result.value();
}

nlohmann::json ReadFileTool::get_schema() const {
    return {
        {"type", "function"},
        {"function", {
            {"name", getName()},
            {"description", getDescription()},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {
                        {"type", "string"},
                        {"description", "Path of file to read"}
                    }}
                }},
                {"required", {"path"}}
            }}
        }}
    };
}