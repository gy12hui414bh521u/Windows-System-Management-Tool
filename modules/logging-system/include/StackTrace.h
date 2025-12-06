#pragma once
// StackTrace.h
#pragma once

#include <string>
#include "ILogger.h"
#include <windows.h> // 需要定义 _EXCEPTION_POINTERS

// 步骤 3.1：定义堆栈追踪辅助函数
namespace StackTrace {
    // 注册未处理异常过滤器
    CORELOGGER_API void RegisterUnhandledExceptionHandler(ILogger* logger);

    // 获取当前线程的堆栈信息
    std::string GetStackTrace();

    // 未处理异常回调函数
    LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo);
}