#pragma once
#include <string>

// SMTP 配置信息
struct SmtpConfig
{
    std::string serverIp;     // 例如 127.0.0.1
    int         port = 25;    // 默认 25
    std::string fromAddress;  // 发件人邮箱

    // SMTP 认证
    // 如果 useAuth == true，则优先尝试使用 username/password 做 AUTH。
    std::string username;     // SMTP 登录用户名
    std::string password;     // SMTP 登录密码或授权码
    bool        useAuth = false; // 是否启用认证

    // 确保网络健壮性
    int         ioTimeoutMs = 5000; // 读写超时时间（毫秒），<=0 表示不特别设置
    int         maxRetry = 1;    // 最大发送重试次数（1 表示不重试）
};

class ConfigLoader
{
public:
    // 从程序运行目录的 email.conf 读取配置
    static bool LoadSmtpConfig(SmtpConfig& cfg, std::string& errorMsg);

private:
    static void Trim(std::string& s);
};
