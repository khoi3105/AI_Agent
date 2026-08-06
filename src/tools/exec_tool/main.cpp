#include "exec_tool.h"
#include <iostream>
int main(){
    ExecTool tool;

    std::string args = R"(
    {
        "command":"ls -la"
    }
    )";
    std::cout << tool.execute(args);
}