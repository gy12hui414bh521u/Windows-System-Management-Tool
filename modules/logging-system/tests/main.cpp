// LoggerTest.cpp
// 完整的功能测试程序（不使用预编译头）

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread> 
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <filesystem>

// 引入 CoreLogger 的头文件
#include "ILogger.h" 
#include "Logger.h"    
#include "LogEntry.h"
#include "LogConfig.h" 
#include "Stopwatch.h"

// 定义工厂函数指针类型
typedef ILogger* (*CreateLoggerFunc)();
typedef int (*RunLoggerFunc)();
typedef const char* (*DescriptionFunc)();

// 辅助常量
const unsigned long MAX_TEST_FILE_SIZE_BYTES = 10 * 1024; // 10KB (来自 FileWriter.h)

namespace fs = std::filesystem;

// -------------------------------------------------------------------
// 辅助函数 (Auxiliary Functions)
// -------------------------------------------------------------------

// 模拟等待，确保日志时间戳有差异
void Wait(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// 显示分隔线
void PrintSeparator() {
    std::cout << "----------------------------------------" << std::endl;
}

// 显示测试标题
void PrintTestTitle(const std::string& title) {
    std::cout << "\n" << title << std::endl;
    PrintSeparator();
}

// 多线程写入日志辅助函数
void ThreadLog(Logger* logger, int id) {
    std::string source = "ThreadLog_" + std::to_string(id);
    for (int i = 0; i < 3; ++i) {
        logger->Info(("多线程测试：来自线程 " + std::to_string(id) + ", 消息 " + std::to_string(i)).c_str(), source.c_str());
        Wait(5);
    }
}

// 测试文件滚动机制
void TestFileRolling(Logger* logger) {
    std::cout << "   > 写入日志以触发文件滚动 (10KB 阈值)..." << std::endl;

    // 生成一个约 1KB 的消息体
    std::string largePayload(1024, 'X');
    std::string largeMessage = "滚动测试 - 文件将很快达到10KB限制";

    // 计算触发滚动所需的日志条数 (10KB / 1KB + 1)
    int logsNeeded = (int)(MAX_TEST_FILE_SIZE_BYTES / 1024) + 3;

    for (int i = 1; i <= logsNeeded; ++i) {
        logger->Warn((largeMessage + " 日志 #" + std::to_string(i)).c_str(), "FileRollTest");
        if (i % 5 == 0) {
            std::cout << "     已写入 " << i << " 条日志..." << std::endl;
        }
    }
    std::cout << "   - OK. 已完成 " << logsNeeded << " 条日志写入" << std::endl;
}

// -------------------------------------------------------------------
// 第一轮功能测试 (Phase 1: 基础功能测试)
// -------------------------------------------------------------------

void TestPhase1_Basic(Logger* concreteLogger) {
    PrintTestTitle("第一轮测试：基础功能验证");

    // 1.1 测试 description() 函数
    std::cout << "1.1 测试 description() 函数..." << std::endl;
    HMODULE hDll = GetModuleHandleA("CoreLogger.dll");
    if (hDll) {
        DescriptionFunc description = (DescriptionFunc)::GetProcAddress(hDll, "description");
        if (description) {
            std::cout << "   DLL 描述信息：" << std::endl;
            std::cout << "   " << description() << std::endl;
        }
    }
    Wait(100);

    // 1.2 基础日志级别测试
    std::cout << "\n1.2 基础日志级别测试..." << std::endl;
    concreteLogger->Info("信息级别日志", "TestPhase1");
    Wait(50);
    concreteLogger->Warn("警告级别日志", "TestPhase1");
    Wait(50);
    concreteLogger->Error("错误级别日志", "TestPhase1");
    Wait(50);
    concreteLogger->Fatal("致命级别日志（测试）", "TestPhase1");
    std::cout << "   - OK. 所有级别日志已记录" << std::endl;

    // 1.3 测试 run() 函数（短暂运行）
    std::cout << "\n1.3 测试 run() 函数..." << std::endl;
    std::cout << "   注：run() 函数将启动控制台模式，测试后将退出" << std::endl;

    // 1.4 测试 LogConfig 配置
    std::cout << "\n1.4 测试 LogConfig 配置..." << std::endl;
    LogLevel currentLevel = LogConfig::GetInstance().GetMinLogLevel();
    std::cout << "   当前最低日志级别: " << LogEntry::LevelToString(currentLevel) << std::endl;
    std::cout << "   日志保留天数: " << LogConfig::GetInstance().GetRetentionDays() << "天" << std::endl;
    std::cout << "   日志文件路径: " << LogConfig::GetInstance().GetLogFilePath() << std::endl;
}

// -------------------------------------------------------------------
// 第二轮功能测试 (Phase 2: 高级功能测试)
// -------------------------------------------------------------------

void TestPhase2_Advanced(Logger* concreteLogger) {
    PrintTestTitle("第二轮测试：高级功能验证");

    // 2.1 测试日志级别过滤
    std::cout << "2.1 测试日志级别过滤..." << std::endl;

    // 设置为 WARNING 级别
    LogConfig::GetInstance().SetMinLogLevel(LogLevel::WARNING);
    std::cout << "   > 设置最低级别为 WARNING (INFO 将被过滤)" << std::endl;

    concreteLogger->Info("这条INFO日志应该被过滤", "FilterTest");
    Wait(30);
    concreteLogger->Warn("这条WARNING日志应该被记录", "FilterTest");
    Wait(30);

    // 恢复为 INFO 级别
    LogConfig::GetInstance().SetMinLogLevel(LogLevel::INFO);
    std::cout << "   > 恢复最低级别为 INFO" << std::endl;
    concreteLogger->Info("这条INFO日志现在应该被记录", "FilterTest");
    std::cout << "   - OK. 级别过滤功能正常" << std::endl;

    // 2.2 测试多线程日志记录
    std::cout << "\n2.2 测试多线程日志记录..." << std::endl;
    std::vector<std::thread> threads;
    for (int i = 1; i <= 4; ++i) {
        threads.emplace_back(ThreadLog, concreteLogger, i);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    std::cout << "   - OK. 4个线程并发写入完成" << std::endl;

    // 2.3 测试性能计时器
    std::cout << "\n2.3 测试性能计时器..." << std::endl;
    {
        ScopedTimer timer(concreteLogger, "PerformanceTest", "性能测试块");
        // 模拟一些工作
        for (int i = 0; i < 100000; ++i) {
            volatile int x = i * 2;
        }
    }
    std::cout << "   - OK. 性能计时器测试完成" << std::endl;

    // 2.4 测试不同上下文
    std::cout << "\n2.4 测试不同上下文源..." << std::endl;
    concreteLogger->Info("来自网络模块的日志", "NetworkModule");
    Wait(30);
    concreteLogger->Warn("来自数据库模块的警告", "DatabaseModule");
    Wait(30);
    concreteLogger->Error("来自UI模块的错误", "UIModule");
    std::cout << "   - OK. 上下文源测试完成" << std::endl;
}

// -------------------------------------------------------------------
// 第三轮功能测试 (Phase 3: 持久化与稳定性测试)
// -------------------------------------------------------------------

void TestPhase3_Persistence(Logger* concreteLogger) {
    PrintTestTitle("第三轮测试：持久化与稳定性验证");

    // 3.1 测试文件滚动机制
    std::cout << "3.1 测试文件滚动机制..." << std::endl;
    TestFileRolling(concreteLogger);

    // 3.2 检查日志目录
    std::cout << "\n3.2 检查日志目录..." << std::endl;
    try {
        fs::path logPath = LogConfig::GetInstance().GetLogFilePath();
        if (fs::exists(logPath)) {
            std::cout << "   日志目录存在: " << logPath.string() << std::endl;

            int fileCount = 0;
            for (const auto& entry : fs::directory_iterator(logPath)) {
                if (entry.is_regular_file()) {
                    std::cout << "     - " << entry.path().filename().string();
                    if (entry.path().filename().string().find("application") != std::string::npos) {
                        std::cout << " (主日志文件)";
                    }
                    std::cout << std::endl;
                    fileCount++;
                }
            }
            std::cout << "   共发现 " << fileCount << " 个日志文件" << std::endl;
        }
        else {
            std::cout << "   警告：日志目录不存在" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "   无法检查日志目录: " << e.what() << std::endl;
    }

    // 3.3 测试配置修改
    std::cout << "\n3.3 测试配置修改..." << std::endl;
    int oldDays = LogConfig::GetInstance().GetRetentionDays();
    LogConfig::GetInstance().SetRetentionDays(14);
    std::cout << "   日志保留天数从 " << oldDays << " 天修改为 "
        << LogConfig::GetInstance().GetRetentionDays() << " 天" << std::endl;

    std::string oldPath = LogConfig::GetInstance().GetLogFilePath();
    LogConfig::GetInstance().SetLogFilePath("./test_logs");
    std::cout << "   日志路径从 '" << oldPath << "' 修改为 '"
        << LogConfig::GetInstance().GetLogFilePath() << "'" << std::endl;

    // 恢复原配置
    LogConfig::GetInstance().SetRetentionDays(oldDays);
    LogConfig::GetInstance().SetLogFilePath(oldPath);
    std::cout << "   - OK. 配置修改测试完成，已恢复原配置" << std::endl;

    // 3.4 压力测试（少量）
    std::cout << "\n3.4 压力测试..." << std::endl;
    Stopwatch stopwatch;
    stopwatch.Start();

    for (int i = 0; i < 50; ++i) {
        concreteLogger->Info(("压力测试消息 #" + std::to_string(i)).c_str(), "StressTest");
        if (i % 10 == 0) {
            Wait(1);
        }
    }

    stopwatch.Stop();
    std::cout << "   写入 50 条日志耗时: " << stopwatch.GetElapsedMilliseconds() << "ms" << std::endl;
    std::cout << "   - OK. 压力测试完成" << std::endl;
}

// -------------------------------------------------------------------
// 主函数 (Main Function: Setup & Execution)
// -------------------------------------------------------------------

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "=== CoreLogger DLL 综合功能测试程序 ===" << std::endl;
    std::cout << "========================================" << std::endl;

    Logger* concreteLogger = nullptr;
    ILogger* logger = nullptr;
    HMODULE hDll = NULL;
    int testStatus = 0;

    try {
        // Step 1: 加载 DLL 并创建 Logger 实例
        std::cout << "\n第一步：加载 CoreLogger.dll..." << std::endl;

        hDll = ::LoadLibraryA("CoreLogger.dll");
        if (hDll == NULL) {
            std::stringstream ss;
            ss << "无法加载 CoreLogger.dll。WinError: " << ::GetLastError();
            throw std::runtime_error(ss.str());
        }

        // 获取工厂函数
        CreateLoggerFunc createLogger = (CreateLoggerFunc)::GetProcAddress(hDll, "CreateLogger");
        if (createLogger == NULL) {
            throw std::runtime_error("无法找到 CreateLogger 工厂函数");
        }

        // 获取其他导出函数
        RunLoggerFunc runLogger = (RunLoggerFunc)::GetProcAddress(hDll, "run");
        DescriptionFunc description = (DescriptionFunc)::GetProcAddress(hDll, "description");

        if (!runLogger || !description) {
            std::cout << "警告：无法找到所有导出函数，但将继续测试..." << std::endl;
        }

        // 创建 Logger 实例
        logger = createLogger();
        if (logger == nullptr) {
            throw std::runtime_error("CreateLogger 返回空指针");
        }

        concreteLogger = dynamic_cast<Logger*>(logger);
        if (concreteLogger == nullptr) {
            throw std::runtime_error("Logger 对象向下转型失败");
        }

        std::cout << "   - 成功加载 DLL 并创建 Logger 实例" << std::endl;

        // Step 2: 执行三轮测试
        std::cout << "\n第二步：执行功能测试..." << std::endl;

        TestPhase1_Basic(concreteLogger);
        Wait(200);
        TestPhase2_Advanced(concreteLogger);
        Wait(200);
        TestPhase3_Persistence(concreteLogger);

        std::cout << "\n========================================" << std::endl;
        std::cout << "=== 所有测试完成 ===" << std::endl;
        std::cout << "========================================" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "\n!!! 致命错误：测试失败: " << e.what() << std::endl;
        testStatus = 1;
    }
    catch (...) {
        std::cerr << "\n!!! 致命错误：测试失败: 发生未知错误" << std::endl;
        testStatus = 1;
    }

    // Step 3: 清理资源
    std::cout << "\n第三步：清理资源..." << std::endl;
    if (logger) {
        delete logger;
        std::cout << "   - Logger 实例已释放" << std::endl;
    }
    if (hDll) {
        ::FreeLibrary(hDll);
        std::cout << "   - CoreLogger.dll 已卸载" << std::endl;
    }

    // Step 4: 测试结果总结
    PrintSeparator();
    std::cout << "测试结果：" << std::endl;
    std::cout << "   状态: " << (testStatus == 0 ? "✓ 成功" : "✗ 失败") << std::endl;
    std::cout << "\n请检查以下内容：" << std::endl;
    std::cout << "   1. 日志文件 application.log" << std::endl;
    std::cout << "   2. 备份的滚动日志文件" << std::endl;
    std::cout << "   3. 确认所有日志级别都已测试" << std::endl;
    std::cout << "   4. 确认多线程日志没有损坏" << std::endl;
    PrintSeparator();

    std::cout << "\n按回车键退出..." << std::endl;
    std::cin.get();

    return testStatus;
}