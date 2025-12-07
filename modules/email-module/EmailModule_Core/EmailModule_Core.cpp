// EmailModule_Core.cpp : 程序入口，只负责调用菜单模块的 RunMenu
#include <windows.h>   // 新增
#include "Menu.h"

// 初始化控制台为 UTF-8 编码，避免中文乱码
void InitConsoleUtf8()
{
    // 输入输出都设成 UTF-8 代码页 65001
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

int main()
{
    InitConsoleUtf8();   // 在任何输出之前先调用
    RunMenu();
    return 0;
}
