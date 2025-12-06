// FileWriter.h
#pragma once

#include "ILogger.h" 
#include "LogEntry.h"
#include <string>
#include <fstream>
#include <mutex>

// 步骤 2.4：定义文件最大大小常量 (100KB 以便于测试)
const unsigned long MAX_LOG_FILE_SIZE_BYTES = 10 * 1024; // 10KB

class CORELOGGER_API FileWriter {
public:
    // 步骤 3.4: 构造函数接受 logPath 参数
    FileWriter(const std::string& filename, const std::string& logPath);
    ~FileWriter();

    // 写入日志条目到文件
    void Write(const LogEntry& entry);

    // 步骤 3.3：归档/清理逻辑
    void RunCleanup();

private:
    std::string filename_;
    std::string logPath_; // 步骤 3.4: 日志存储路径
    std::ofstream fileStream_;
    std::mutex writeMutex_;
    bool isFirstWrite_; // 步骤 3.3: 仅在首次写入时执行清理

    // 格式化 LogEntry 为可读字符串
    std::string FormatLogEntry(const LogEntry& entry);

    // 步骤 2.4：检查并执行日志文件滚动
    void CheckAndRoll();
    // 步骤 2.4：将当前文件重命名为带时间戳的备份文件
    void RollFile();
};