// Logger.h
#pragma once

#include "ILogger.h"
#include "LogEntry.h"
#include "FileWriter.h"
#include "LogConfig.h" 
#include "Stopwatch.h" // 步骤 3.2: 引入 Stopwatch
#include <memory>
#include <string>

// 步骤 1.1, 3.2：实现 Logger 类
class CORELOGGER_API Logger : public ILogger {
public:
    Logger();
    ~Logger() override;

    // 步骤 1.2, 1.5：实现 ILogger 接口方法
    void Log(const char* message) override;
    void Log(const char* message, const char* sourceClass) override;

    // 步骤 3.1：实现 Fatal 级别日志记录
    void Fatal(const char* message, const char* sourceClass) override;

    // 辅助方法：实现带级别和上下文的日志记录（这是核心记录方法）
    void Log(LogLevel level, const char* message, const char* sourceClass);

    // 步骤 2.1：实现 Info/Warn/Error 辅助宏调用的方法
    void Info(const char* message, const char* sourceClass);
    void Warn(const char* message, const char* sourceClass);
    void Error(const char* message, const char* sourceClass);

    // 步骤 3.1：注册异常处理器
    void RegisterExceptionHandler();

private:
    std::unique_ptr<FileWriter> fileWriter_;

    // 辅助方法：获取当前线程 ID (Windows)
    unsigned long GetThreadId() const;

};

