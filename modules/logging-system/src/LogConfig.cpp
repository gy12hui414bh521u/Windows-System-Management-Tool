// LogConfig.cpp
#include "pch.h"
#include "LogConfig.h"

// 初始化静态成员
std::once_flag LogConfig::initFlag_;
LogConfig* LogConfig::instance_ = nullptr;

// 步骤 2.2 / 3.4：实现 LogConfig::GetInstance()
LogConfig& LogConfig::GetInstance() {
    std::call_once(initFlag_, []() {
        instance_ = new LogConfig();
        });
    // 确保在进程退出时释放内存
    static struct LogConfigCleaner {
        ~LogConfigCleaner() {
            delete instance_;
        }
    } cleaner;

    if (instance_ == nullptr) {
        throw std::runtime_error("LogConfig initialization failed.");
    }
    return *instance_;
}

// 步骤 2.2：实现 SetMinLogLevel
void LogConfig::SetMinLogLevel(LogLevel level) {
    minLevel_.store(level);
}

// 步骤 2.2：实现 GetMinLogLevel
LogLevel LogConfig::GetMinLogLevel() const {
    return minLevel_.load();
}

// 步骤 3.3：实现 SetRetentionDays
void LogConfig::SetRetentionDays(int days) {
    if (days >= 0) {
        retentionDays_.store(days);
    }
}

// 步骤 3.3：实现 GetRetentionDays
int LogConfig::GetRetentionDays() const {
    return retentionDays_.load();
}

// 步骤 3.4：实现 SetLogFilePath
void LogConfig::SetLogFilePath(const std::string& path) {
    // Note: 生产环境中应考虑路径规范化和线程安全
    logFilePath_ = path;
}

// 步骤 3.4：实现 GetLogFilePath
std::string LogConfig::GetLogFilePath() const {
    return logFilePath_;
}