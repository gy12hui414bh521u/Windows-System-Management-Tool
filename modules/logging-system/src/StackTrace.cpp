// StackTrace.cpp
#include "pch.h"
#include "StackTrace.h"
#include <windows.h>
#include <DbgHelp.h> // 需要链接 Dbghelp.lib
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Dbghelp.lib") // 自动链接 Dbghelp.lib

namespace StackTrace {

    // 步骤 3.1：全局 Logger 指针，用于在异常回调中记录日志
    static ILogger* s_exceptionLogger = nullptr;

    // 步骤 3.1：获取当前线程的堆栈信息
    std::string GetStackTrace() {
        // 初始化符号处理 (SymInitialize 只需要调用一次)
        static bool symInitialized = false;
        if (!symInitialized) {
            if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
                return "Error: SymInitialize failed.";
            }
            symInitialized = true;
        }

        void* stack[100];
        // 捕获堆栈帧指针
        USHORT frames = CaptureStackBackTrace(0, 100, stack, NULL);

        // 分配 SYMBOL_INFO 结构体所需的内存
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
        if (!symbol) return "Error: Could not allocate memory for symbol info.";

        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        std::stringstream ss;
        ss << "\n--- Full Stack Trace (Capturing " << frames << " Frames) ---\n";

        for (USHORT i = 0; i < frames; i++) {
            DWORD64 displacement = 0;

            // 获取符号信息
            if (SymFromAddr(GetCurrentProcess(), (DWORD64)stack[i], &displacement, symbol)) {
                ss << "Frame " << std::setw(2) << std::setfill('0') << i << ": "
                    << symbol->Name << " + 0x" << std::hex << displacement
                    << " (Address: " << (void*)stack[i] << ")\n";
            }
            else {
                ss << "Frame " << std::setw(2) << std::setfill('0') << i << ": <Unknown Symbol>\n";
            }
        }
        ss << "--------------------------------------------------\n";
        free(symbol);
        return ss.str();
    }

    // 步骤 3.1：未处理异常回调函数
    LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo) {
        if (s_exceptionLogger) {
            std::stringstream ss;
            ss << "UNHANDLED EXCEPTION (Code: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << "). "
                << "Attempting to generate stack trace before crash.";

            // 记录 FATAL 级别日志 (记录异常基本信息)
            s_exceptionLogger->Fatal(ss.str().c_str(), "CRASH_HANDLER");

            // 记录完整的堆栈信息
            s_exceptionLogger->Fatal(GetStackTrace().c_str(), "CRASH_HANDLER");

            // 关键：由于程序即将崩溃，日志必须立刻同步到磁盘
            // (在 FileWriter::Write 中已通过 flush() 保证)
        }

        // 返回 EXCEPTION_CONTINUE_SEARCH 允许其他处理程序继续或调用默认处理程序
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // 步骤 3.1：注册未处理异常过滤器
    void RegisterUnhandledExceptionHandler(ILogger* logger) {
        s_exceptionLogger = logger;
        SetUnhandledExceptionFilter(UnhandledExceptionFilter);
    }
}