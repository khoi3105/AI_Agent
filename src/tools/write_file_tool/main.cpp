#include "write_file_tool.h"
#include <iostream>
using namespace std;
int main(){
    WriteFileTool tool;
    cout << tool.execute(R"({
    "path":"hello.txt",
    "content":"Hello from WriteFileTool!"})");
    return 0;
}