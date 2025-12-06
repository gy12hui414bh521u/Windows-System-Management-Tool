// ILogger.h
#pragma once

// 定义 DLL 导出/导入宏
#ifdef CORELOGGER_EXPORTS
#define CORELOGGER_API __declspec(dllexport)
#else
#define CORELOGGER_API __declspec(dllimport)
#endif

// 步骤 1.1：定义 ILogger 接口
class CORELOGGER_API ILogger {
public:
    // 步骤 1.2：实现最基础的日志记录方法（只记录字符串）
    virtual void Log(const char* message) = 0;

    // 步骤 2.1：实现带上下文的日志记录方法（用于类名）
    virtual void Log(const char* message, const char* sourceClass) = 0;

    // 步骤 3.1：新增 FATAL 级别日志记录方法 (用于异常捕获)
    virtual void Fatal(const char* message, const char* sourceClass) = 0;

    // 虚析构函数，保证继承类正确释放
    virtual ~ILogger() = default;
};

// 导出创建 Logger 实例的工厂函数


extern "C" {
    CORELOGGER_API ILogger* CreateLogger();
    CORELOGGER_API void DestroyLogger(ILogger* logger);
    CORELOGGER_API int RunLoggerConsole();      // 控制台模式入口
    CORELOGGER_API const char* GetLoggerInfo(); // 描述信息

    // 添加要求的两个导出函数
    CORELOGGER_API int run();                   // 模拟 main 函数
    CORELOGGER_API const char* description();   // 返回描述信息
}