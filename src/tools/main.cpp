#include "tool_registry.h"
#include <print>
#include <string>
#include <iostream>
using namespace std;
int main(){
    ToolRegistry registry;
    string s;
    print("Nhập một biểu thức toán học cơ bản với các phép +, -, *, /: ");
    getline(cin,s);
    string result = registry.executeTool("calculator", s);
    println("Đáp án của kết quả trên là {}", result);
}