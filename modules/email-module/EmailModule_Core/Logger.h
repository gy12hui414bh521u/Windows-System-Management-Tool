#pragma once
#include <string>

// 简单日志模块：把信息追加写入 email.log
class EmailLogger
{
public:
    static void Info(const std::string& msg);
    static void Error(const std::string& msg);

private:
    static void Log(const std::string& level, const std::string& msg);
};

