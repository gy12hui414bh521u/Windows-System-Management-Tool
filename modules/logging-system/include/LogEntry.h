// LogEntry.h
#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <stdexcept>

// 步骤 1.5：引入 LogEntry，用于格式化日志内容
// 步骤 2.1：添加对线程 ID 和类名的支持

#pragma push_macro("ERROR")
#ifdef ERROR
#undef ERROR
#endif

// 步骤 3.1：新增 FATAL 级别，用于未处理异常
// 步骤 2.2 补充：新增 NONE 级别，用于禁用所有日志
enum class LogLevel {
    INFO,    // 0 - 默认级别 (最低严重度)
    WARNING, // 1
    ERROR_LEVEL, // 2 - 避免与宏冲突
    FATAL,   // 3 - 异常捕获 (最高严重度)
    NONE     // 4 - 仅用于配置 MinLogLevel，表示不记录任何日志 (数值最高)
};

#pragma pop_macro("ERROR")

// LogEntry 结构体
struct LogEntry {
    std::chrono::system_clock::time_point timestamp; // 时间戳
    LogLevel level;                                 // 日志级别
    std::string message;                            // 消息体
    unsigned long threadId;                         // 线程 ID (步骤 2.3)
    std::string sourceClass;                        // 来源类名 (步骤 2.3)

    // 将 LogLevel 转换为字符串
    static const char* LevelToString(LogLevel level) {
        switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN"; // 使用 WARN 缩写
        case LogLevel::ERROR_LEVEL: return "ERROR";
        case LogLevel::FATAL: return "FATAL"; // 步骤 3.1: FATAL
        case LogLevel::NONE: return "NONE";   // 步骤 2.2 补充: NONE
        default: return "UNKNOWN";
        }
    }
};