// FileWriter.cpp
#include "pch.h"
#include "FileWriter.h"
#include "LogConfig.h" // 引入 LogConfig 获取保留天数
#include <sstream> 
#include <ctime>   
#include <iostream> 
#include <Windows.h> 
#include <filesystem> 

namespace fs = std::filesystem;

// 固定文件名为 "application.log"
const std::string DEFAULT_LOG_FILENAME = "application.log";

// 步骤 1.3, 3.4：实现 FileWriter 构造函数 (使用 logPath)
FileWriter::FileWriter(const std::string& filename, const std::string& logPath)
    : filename_(filename), logPath_(logPath), isFirstWrite_(true) {

    // 步骤 3.4：确保日志目录存在
    try {
        if (!fs::exists(logPath_)) {
            fs::create_directories(logPath_);
        }

        fs::path fullPath = fs::path(logPath_) / filename_;
        fileStream_.open(fullPath.string(), std::ios::out | std::ios::app);
        if (!fileStream_.is_open()) {
            std::cerr << "Error: Could not open log file: " << fullPath.string() << std::endl;
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating log directory or opening file: " << e.what() << std::endl;
    }
}

// 步骤 1.3：实现 ~FileWriter 析构函数
FileWriter::~FileWriter() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

// 步骤 3.3：实现清理/归档逻辑
void FileWriter::RunCleanup() {
    // 归档/清理逻辑仅在第一次写入时执行
    if (!isFirstWrite_) {
        return;
    }
    isFirstWrite_ = false;

    int retentionDays = LogConfig::GetInstance().GetRetentionDays();
    if (retentionDays <= 0) {
        return; // 不执行清理
    }

    try {
        std::cout << "Starting log cleanup. Retention days: " << retentionDays << "..." << std::endl;
        auto now = std::chrono::system_clock::now();
        // 计算 N 天前的截止时间点
        auto cutoffTime = now - std::chrono::hours(24 * retentionDays);

        // 遍历日志目录
        for (const auto& entry : fs::directory_iterator(logPath_)) {
            if (entry.is_regular_file()) {
                // 仅处理备份日志文件 (带时间戳)
                if (entry.path().filename().string().find(DEFAULT_LOG_FILENAME.substr(0, DEFAULT_LOG_FILENAME.find_last_of('.'))) == 0 &&
                    entry.path().filename().string() != DEFAULT_LOG_FILENAME) {

                    // 获取文件最后修改时间
                    auto ftime = fs::last_write_time(entry.path());

                    // 将 std::filesystem::file_time_type 转换为 std::chrono::system_clock::time_point
                    // 注意：这在 C++17 中实现复杂，这里使用一个通用的转换逻辑（可能因平台而异）
                    // 假设 fs::file_time_type::clock::now() 与 std::chrono::system_clock::now() 接近
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                    );

                    if (sctp < cutoffTime) {
                        // 文件早于截止时间，删除
                        fs::remove(entry.path());
                        std::cout << "Cleaned up old log file: " << entry.path().filename().string() << std::endl;
                    }
                }
            }
        }
        std::cout << "Log cleanup finished." << std::endl;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error during log cleanup: " << e.what() << std::endl;
    }
}


// 步骤 2.4：实现 CheckAndRoll()
void FileWriter::CheckAndRoll() {
    // ... (保持不变) ...
    if (!fileStream_.is_open()) {
        return;
    }

    // 1. 确保所有待写入数据已同步到文件
    fileStream_.flush();

    // 2. 备份当前写入位置
    std::streampos current_pos = fileStream_.tellp();

    // 3. 将写入指针移动到文件末尾
    fileStream_.seekp(0, std::ios::end);

    // 4. 获取文件大小 (当前位置即为文件大小)
    __int64 currentSize = static_cast<__int64>(fileStream_.tellp());

    // 5. 恢复写入位置
    fileStream_.seekp(current_pos);

    // 6. 检查是否达到最大值
    if (currentSize >= MAX_LOG_FILE_SIZE_BYTES) {
        RollFile();
    }
}


// 步骤 2.4：实现文件滚动逻辑
void FileWriter::RollFile() {
    // 1. 关闭当前文件流，确保文件句柄释放
    if (fileStream_.is_open()) {
        fileStream_.close();
    }

    // 强制短暂等待，确保 Windows 释放文件句柄
    ::Sleep(50);

    // 2. 生成带时间戳的新文件名 (application.YYYYMMDD_HHMMSS.log)
    auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};

    if (localtime_s(&bt, &now_time) != 0) {
        std::cerr << "Error generating timestamp for log roll." << std::endl;
        fs::path fullPath = fs::path(logPath_) / filename_;
        fileStream_.open(fullPath.string(), std::ios::out | std::ios::app);
        return;
    }

    char timeBuffer[50];
    std::strftime(timeBuffer, sizeof(timeBuffer), ".%Y%m%d_%H%M%S.log", &bt);

    size_t dot_pos = filename_.find_last_of('.');
    std::string newFilenameBase = (dot_pos != std::string::npos) ? filename_.substr(0, dot_pos) : filename_;
    std::string newFilename = newFilenameBase + timeBuffer;

    fs::path oldFullPath = fs::path(logPath_) / filename_;
    fs::path newFullPath = fs::path(logPath_) / newFilename;

    // 3. 重命名旧文件 (使用 MoveFileA)
    if (fs::exists(oldFullPath)) {
        if (::MoveFileA(oldFullPath.string().c_str(), newFullPath.string().c_str())) {
            // Success
            std::cout << "Log file rolled: " << oldFullPath.string() << " -> " << newFullPath.string() << std::endl;
        }
        else {
            // Failure: 打印 Windows 错误码
            DWORD error = ::GetLastError();
            std::cerr << "--- ROLL FAILED --- Error renaming file: " << oldFullPath.string()
                << ", WinError: " << error << ". File is LOCKED by OS." << std::endl;
        }
    }

    // 4. 重新打开文件流，创建新的 application.log
    fileStream_.open(oldFullPath.string(), std::ios::out | std::ios::app);
    if (!fileStream_.is_open()) {
        std::cerr << "Error: Could not reopen log file after roll: " << oldFullPath.string() << std::endl;
    }
}

// 步骤 1.3, 1.5, 2.3：实现 FormatLogEntry (包含上下文信息)
std::string FileWriter::FormatLogEntry(const LogEntry& entry) {

    const auto now_time = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm bt{};

    if (localtime_s(&bt, &now_time) != 0) {
        return "TIMESTAMP_ERROR [" + std::string(LogEntry::LevelToString(entry.level)) + "] " + entry.message + "\n";
    }

    char timeBuffer[30];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &bt);

    std::stringstream ss;
    ss << timeBuffer
        << " [" << LogEntry::LevelToString(entry.level) << "] "
        << " [TID:" << entry.threadId << "] "
        << " [" << entry.sourceClass << "] "
        << entry.message << "\n";

    return ss.str();
}

// 步骤 1.3, 2.4, 3.3：实现 Write 
void FileWriter::Write(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(writeMutex_);

    // 步骤 3.3：在第一次写入前执行清理 (同步/启动时检查)
    RunCleanup();

    // 步骤 2.4：在写入之前检查文件大小
    CheckAndRoll();

    if (fileStream_.is_open()) {
        fileStream_ << FormatLogEntry(entry);
        fileStream_.flush(); // 强制刷新，确保 FATAL 级别的日志能够立刻写入磁盘
    }
}