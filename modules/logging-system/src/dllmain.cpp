#include "pch.h"
#include <windows.h> // 确保定义了 APIENTRY 和 BOOL
#include <iostream>

// 注意：在 DllMain 中不应执行复杂的或可能导致加载器锁死的操作，例如文件 I/O 或 stdio
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 可以在这里进行模块级别的初始化
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        // 可以在这里进行模块级别的清理
        break;
    }
    return TRUE;
}