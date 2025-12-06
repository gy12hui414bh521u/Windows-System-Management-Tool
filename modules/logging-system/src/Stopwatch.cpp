// Stopwatch.cpp
#include "pch.h"
#include "Stopwatch.h"
#include <sstream>

// 步骤 3.2：Stopwatch 实现
Stopwatch::Stopwatch() : isRunning_(false) {}

void Stopwatch::Start() {
    startTime_ = std::chrono::high_resolution_clock::now();
    isRunning_ = true;
}

void Stopwatch::Stop() {
    stopTime_ = std::chrono::high_resolution_clock::now();
    isRunning_ = false;
}

double Stopwatch::GetElapsedSeconds() const {
    if (isRunning_) {
        // 如果仍在运行，计算到现在的时间
        return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime_).count();
    }
    else {
        return std::chrono::duration<double>(stopTime_ - startTime_).count();
    }
}

long long Stopwatch::GetElapsedMilliseconds() const {
    if (isRunning_) {
        // 如果仍在运行，计算到现在的时间
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime_).count();
    }
    else {
        return std::chrono::duration_cast<std::chrono::milliseconds>(stopTime_ - startTime_).count();
    }
}


// 步骤 3.2：ScopedTimer 实现
ScopedTimer::ScopedTimer(ILogger* logger, const char* sourceClass, const char* contextMessage)
    : logger_(logger),
    sourceClass_(sourceClass),
    contextMessage_(contextMessage)
{
    stopwatch_.Start();
}

ScopedTimer::~ScopedTimer() {
    stopwatch_.Stop();

    // 自动记录日志
    if (logger_) {
        std::stringstream ss;
        // 记录毫秒，更精确
        ss << contextMessage_ << " executed in " << stopwatch_.GetElapsedMilliseconds() << "ms.";
        // 使用 INFO 级别记录性能数据
        logger_->Log(ss.str().c_str(), sourceClass_.c_str());
    }
}