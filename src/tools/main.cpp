#include "calculator_tool.h"
#include <print>
#include <string>
#include <iostream>
using namespace std;
int main(){
    Tool* tool = new CalculatorTool();
    string s;
    print("Nhập một biểu thức toán học cơ bản với các phép +, -, *, /: ");
    getline(cin,s);
    string result = tool->execute(s);
    println("Đáp án của kết quả trên là {}", result);
}