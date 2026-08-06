#include <iostream>
#include "read_file_tool.h"

int main()
{
    ReadFileTool tool;
    std::string path;
    std::cout << "Nhap duong dan file: ";
    std::getline(std::cin, path);

    std::string result = tool.execute(path);

    std::cout << "\n===== NOI DUNG FILE =====\n";
    std::cout << result << '\n';

    return 0;
}