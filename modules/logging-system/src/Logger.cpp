// Logger.cpp
#include "pch.h"
#include "Logger.h"
#include "StackTrace.h" // 步骤 3.1: 引入堆栈追踪头文件
#include <iostream>
#include <Windows.h> 

// 固定文件名为 "application.log" 
const std::string DEFAULT_LOG_FILENAME = "application.log";

// 步骤 1.1, 10, 3.4：实现 Logger 构造函数
Logger::Logger()
// 步骤 3.4: 使用 LogConfig 中的路径配置
    : fileWriter_(std::make_unique<FileWriter>(DEFAULT_LOG_FILENAME, LogConfig::GetInstance().GetLogFilePath()))
{
    // 构造时初始化 FileWriter
}

// 析构函数 (无操作，依赖 unique_ptr 释放 fileWriter_)
Logger::~Logger() {
}


// 辅助方法：获取当前线程 ID (Windows)
unsigned long Logger::GetThreadId() const {
    return static_cast<unsigned long>(::GetCurrentThreadId());
}


// 步骤 1.2, 1.5：实现 ILogger 接口方法（默认 INFO 级别，无上下文）
void Logger::Log(const char* message) {
    Log(LogLevel::INFO, message, "ILogger::Log");
}

// 步骤 2.1：实现带上下文的日志记录
void Logger::Log(const char* message, const char* sourceClass) {
    Log(LogLevel::INFO, message, sourceClass);
}

// 步骤 3.1：实现 Fatal 级别日志记录
void Logger::Fatal(const char* message, const char* sourceClass) {
    Log(LogLevel::FATAL, message, sourceClass);
}


// 核心日志记录方法（包含级别和上下文）
void Logger::Log(LogLevel level, const char* message, const char* sourceClass) {
    // 步骤 2.2：检查最低日志级别进行过滤
    // NONE 级别的数值最高，任何实际日志级别都低于 NONE，从而达到禁用所有日志的目的
    if (level < LogConfig::GetInstance().GetMinLogLevel()) {
        return; // 级别低于设置的最低级别，忽略
    }

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.message = message;
    entry.threadId = GetThreadId();        // 步骤 2.3：记录线程 ID
    entry.sourceClass = sourceClass ? sourceClass : "Unknown"; // 步骤 2.3：记录类名

    if (fileWriter_) {
        fileWriter_->Write(entry);
    }
}

// 步骤 2.1：实现 Info/Warn/Error 辅助方法
void Logger::Info(const char* message, const char* sourceClass) {
    Log(LogLevel::INFO, message, sourceClass);
}

void Logger::Warn(const char* message, const char* sourceClass) {
    Log(LogLevel::WARNING, message, sourceClass);
}

void Logger::Error(const char* message, const char* sourceClass) {
    Log(LogLevel::ERROR_LEVEL, message, sourceClass);
}

// 步骤 3.1：注册异常处理器
void Logger::RegisterExceptionHandler() {
    StackTrace::RegisterUnhandledExceptionHandler(this);
    std::cout << "Unhandled Exception Handler registered." << std::endl;
}


// 步骤 1.1, 10：实现 CreateLogger 导出函数
extern "C" CORELOGGER_API ILogger* CreateLogger() {
    // 步骤 3.1: 在创建 Logger 实例时注册异常处理器
    Logger* logger = new Logger();
    logger->RegisterExceptionHandler();
    return logger;
}

extern "C" CORELOGGER_API int RunLoggerConsole() {
    std::cout << "=== Logger Console Mode ===" << std::endl;

    // 使用CreateLogger创建实例
    Logger* logger = dynamic_cast<Logger*>(CreateLogger());
    if (!logger) {
        std::cerr << "Failed to create logger!" << std::endl;
        return 1;
    }

    std::string message;
    while (true) {
        std::cout << "输入日志内容 (输入quit退出): ";  // 输出到控制台
        std::getline(std::cin, message);    // 从控制台监听输入

        if (message == "quit" || message == "exit") break;

        // 使用现有的Logger系统记录
        logger->Info(message.c_str(), "ConsoleInput");
        std::cout << "已记录到日志文件。" << std::endl;
    }

    delete logger;
    std::cout << "Logger Console 关闭。" << std::endl;
    return 0;
}

extern "C" CORELOGGER_API const char* GetLoggerInfo() {
    return "Core Logger DLL v1.0 - Professional logging system with console interface";
}

// =========== 以下是新增的函数 ===========

extern "C" CORELOGGER_API int run() {
    std::cout << "=== CoreLogger 日志系统 ===" << std::endl;

    while (true) {
        std::cout << "\n请选择模式：" << std::endl;
        std::cout << "1. 控制台日志模式（输入日志到文件）" << std::endl;
        std::cout << "2. 快速测试模式（记录几条测试日志）" << std::endl;
        std::cout << "3. 查看系统信息" << std::endl;
        std::cout << "4. 帮助信息" << std::endl;
        std::cout << "5. 数字计算测试（整合功能）" << std::endl;
        std::cout << "0. 退出程序" << std::endl;
        std::cout << "输入选择 (0-5): ";

        std::string input;
        std::getline(std::cin, input);

        // 处理直接命令
        if (input == "help" || input == "h") {
            std::cout << "\n帮助信息：" << std::endl;
            std::cout << "- 输入数字 0-5 选择菜单项" << std::endl;
            std::cout << "- 输入 help 查看此帮助" << std::endl;
            std::cout << "- 输入 quit 退出程序" << std::endl;
            continue;
        }

        if (input == "quit" || input == "exit" || input == "0") {
            std::cout << "再见！" << std::endl;
            break;
        }

        int choice;
        try {
            choice = std::stoi(input);
        }
        catch (...) {
            std::cout << "错误：请输入数字 0-5" << std::endl;
            continue;
        }

        switch (choice) {
        case 1:
            std::cout << "进入控制台日志模式..." << std::endl;
            return RunLoggerConsole();

        case 2: {
            Logger* logger = dynamic_cast<Logger*>(CreateLogger());
            if (!logger) {
                std::cerr << "创建 Logger 失败!" << std::endl;
                break;
            }
            logger->Info("这是一条测试信息", "Test");
            logger->Warn("这是一个警告", "Test");
            logger->Error("这是一个错误", "Test");
            std::cout << "已记录3条测试日志到 application.log" << std::endl;
            delete logger;
            break;
        }

        case 3:
            std::cout << description() << std::endl;
            std::cout << "日志路径: " << LogConfig::GetInstance().GetLogFilePath() << std::endl;
            std::cout << "保留天数: " << LogConfig::GetInstance().GetRetentionDays() << "天" << std::endl;
            break;

        case 4:
            std::cout << "\n====== 帮助信息 ======" << std::endl;
            std::cout << "这是一个专业的日志系统DLL，包含以下功能：" << std::endl;
            std::cout << "- 多级别日志记录" << std::endl;
            std::cout << "- 线程安全" << std::endl;
            std::cout << "- 文件滚动" << std::endl;
            std::cout << "- 异常捕获" << std::endl;
            std::cout << "- 性能计时" << std::endl;
            std::cout << "=====================" << std::endl;
            break;

        case 5: {
            std::cout << "\n====== 数字计算测试 ======" << std::endl;
            std::cout << "这是整合测试功能：输入数字，输出 (数字+1)" << std::endl;
            std::cout << "输入数字（输入 'back' 返回）: ";

            std::string numInput;
            std::getline(std::cin, numInput);

            if (numInput == "back" || numInput == "b") {
                std::cout << "返回主菜单" << std::endl;
                break;
            }

            try {
                int num = std::stoi(numInput);
                std::cout << "计算结果: " << num << " + 1 = " << (num + 1) << std::endl;
                return 42;  // 整合同学要求的返回值
            }
            catch (...) {
                std::cout << "错误：请输入有效的数字" << std::endl;
            }
            break;
        }

        default:
            std::cout << "无效选择，请输入 0-5" << std::endl;
            break;
        }
    }

    return 0;
}

extern "C" CORELOGGER_API const char* description() {
    static const char* desc =
        "Core Logger DLL v1.0\n"
        "- 多级别日志（INFO/WARN/ERROR/FATAL）\n"
        "- 线程安全日志记录\n"
        "- 性能计时工具（Stopwatch/ScopedTimer）\n"
        "- 日志文件滚动（超过10KB自动归档）\n"
        "- 基础日志配置\n"
        "- 控制台交互模式\n\n"
        "- 异常捕获机制（待充分测试）\n"
        "- 日志清理策略（基础框架）";

    return desc;
}

extern "C" CORELOGGER_API void DestroyLogger(ILogger* logger) {
    if (logger) {
        delete logger;
    }
}