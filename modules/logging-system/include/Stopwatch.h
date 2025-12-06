#pragma once
// Stopwatch.h
#pragma once

#include "ILogger.h"
#include <chrono>
#include <string>

// 步骤 3.2：实现一个简单的 Stopwatch 类
class CORELOGGER_API Stopwatch {
public:
    Stopwatch();

    void Start();
    void Stop();

    // 获取持续时间（秒）
    double GetElapsedSeconds() const;
    // 获取持续时间（毫秒）
    long long GetElapsedMilliseconds() const;

private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::high_resolution_clock::time_point stopTime_;
    bool isRunning_;
};

// 步骤 3.2：自动计时器，在析构时自动记录日志
class CORELOGGER_API ScopedTimer {
public:
    ScopedTimer(ILogger* logger, const char* sourceClass, const char* contextMessage);
    ~ScopedTimer();

private:
    ILogger* logger_;
    std::string sourceClass_;
    std::string contextMessage_;
    Stopwatch stopwatch_;
};