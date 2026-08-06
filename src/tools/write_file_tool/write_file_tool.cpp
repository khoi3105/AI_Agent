#include "write_file_tool.h"
#include "../../utils/read_write_textfile.h"
#include <filesystem>
#include <fstream>
#include <expected>

std::string WriteFileTool::getName() const {
    return "write_file";
}

std::string WriteFileTool::getDescription() const {
    return "Write text content to a file. Creates the file if it does not exist and overwrites existing content.";
}

std::string WriteFileTool::execute(const std::string& args)
{
    auto j = nlohmann::json::parse(args);
    auto result = FileUtils::writeFile(j["path"].get<std::string>(),j["content"].get<std::string>());
    if (!result)return result.error();
    return "Ghi file thành công!";
}

nlohmann::json WriteFileTool::get_schema() const
{
    return {
        {"type", "function"},
        {"function",
            {
                {"name", getName()},
                {"description", getDescription()},
                {"parameters",
                    {
                        {"type", "object"},
                        {"properties",
                            {
                                {
                                    "path",
                                    {
                                        {"type", "string"},
                                        {"description", "Destination file path"}
                                    }
                                },
                                {
                                    "content",
                                    {
                                        {"type", "string"},
                                        {"description", "Text content to write into the file"}
                                    }
                                }
                            }
                        },
                        {"required", {"path", "content"}}
                    }
                }
            }
        }
    };
}