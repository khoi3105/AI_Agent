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

std::string WriteFileTool::execute(const nlohmann::json& args)
{
    try
    {
        // Trích xuất trực tiếp các tham số từ JSON object args
        std::string path = args["path"].get<std::string>();
        std::string content = args["content"].get<std::string>();

        auto result = FileUtils::writeFile(path, content);
        if (!result)
        {
            return result.error();
        }

        return "Ghi file thành công!";
    }
    catch (const std::exception& e)
    {
        return std::string("WriteFileTool error: ") + e.what();
    }
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