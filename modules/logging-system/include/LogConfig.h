// LogConfig.h
#pragma once

#include "ILogger.h"
#include "LogEntry.h"
#include <atomic>
#include <mutex>
#include <string> // 步骤 3.4: 引入 string

// 步骤 2.2 / 3.4：定义 LogConfig 类
class CORELOGGER_API LogConfig {
private:
    // 步骤 3.4: 默认路径为 ./logs，默认 MinLevel 为 INFO，默认保留 7 天
    LogConfig() : minLevel_(LogLevel::INFO), retentionDays_(7), logFilePath_("./logs") {}
    ~LogConfig() = default;
    LogConfig(const LogConfig&) = delete;
    LogConfig& operator=(const LogConfig&) = delete;

    std::atomic<LogLevel> minLevel_;
    std::atomic<int> retentionDays_; // 步骤 3.3: 日志保留天数
    std::string logFilePath_;        // 步骤 3.4: 日志文件存储路径

    static std::once_flag initFlag_;
    static LogConfig* instance_;

public:
    static LogConfig& GetInstance();

    // 步骤 2.2：最低记录级别
    void SetMinLogLevel(LogLevel level);
    LogLevel GetMinLogLevel() const;

    // 步骤 3.3：日志保留天数
    void SetRetentionDays(int days);
    int GetRetentionDays() const;

    // 步骤 3.4：日志文件存储路径
    void SetLogFilePath(const std::string& path);
    std::string GetLogFilePath() const;
};