#include <iostream>

#include "memory_save_tool/memory_save_tool.h"
#include "memory_search_tool/memory_search_tool.h"

int main()
{
    MemorySaveTool saveTool;
    MemorySearchTool searchTool;

    // =========================
    // SAVE MEMORY
    // =========================

    std::cout << "=== SAVE ===\n";

    std::cout << saveTool.execute(
        R"({
            "content": "Tôi đang học C++"
        })"
    ) << '\n';

    std::cout << saveTool.execute(
        R"({
            "content": "Tôi đang xây dựng một AI Agent"
        })"
    ) << '\n';

    std::cout << saveTool.execute(
        R"({
            "content": "AI Agent của tôi có ToolRegistry"
        })"
    ) << '\n';


    // =========================
    // SEARCH MEMORY
    // =========================

    std::cout << "\n=== SEARCH C++ ===\n";

    std::cout << searchTool.execute(
        R"({
            "query": "C++"
        })"
    ) << '\n';


    std::cout << "\n=== SEARCH AI Agent ===\n";

    std::cout << searchTool.execute(
        R"({
            "query": "AI Agent",
            "limit": 5
        })"
    ) << '\n';


    std::cout << "\n=== SEARCH ToolRegistry ===\n";

    std::cout << searchTool.execute(
        R"({
            "query": "ToolRegistry"
        })"
    ) << '\n';


    // =========================
    // SEARCH NOT FOUND
    // =========================

    std::cout << "\n=== SEARCH NOT FOUND ===\n";

    std::cout << searchTool.execute(
        R"({
            "query": "Python"
        })"
    ) << '\n';


    return 0;
}