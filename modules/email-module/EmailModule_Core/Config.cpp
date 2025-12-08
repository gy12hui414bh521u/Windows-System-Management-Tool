#include "Config.h"

#include <fstream>
#include <string>
#include <cctype>

bool ConfigLoader::LoadSmtpConfig(SmtpConfig& cfg, std::string& errorMsg)
{
    std::ifstream ifs("email.conf");
    if (!ifs.is_open())
    {
        errorMsg = "无法打开配置文件 email.conf，请确认文件存在于程序运行目录。";
        return false;
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        Trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        Trim(key);
        Trim(value);

        if (key == "server_ip")
        {
            cfg.serverIp = value;
        }
        else if (key == "port")
        {
            try
            {
                cfg.port = std::stoi(value);
            }
            catch (...)
            {
                errorMsg = "配置文件中 port 值无效。";
                return false;
            }
        }
        else if (key == "from_address")
        {
            cfg.fromAddress = value;
        }
        else if (key == "username")
        {
            cfg.username = value;
        }
        else if (key == "password")
        {
            cfg.password = value;
        }
        else if (key == "use_auth")
        {
            // 允许几种写法：1/true/yes → 启用；0/false/no → 关闭；其他默认当关闭
            std::string vLower = value;
            for (char& ch : vLower)
            {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }

            if (vLower == "1" || vLower == "true" || vLower == "yes")
            {
                cfg.useAuth = true;
            }
            else
            {
                cfg.useAuth = false;
            }
        }
        else if (key == "io_timeout_ms")
        {
            try
            {
                cfg.ioTimeoutMs = std::stoi(value);
            }
            catch (...)
            {
                errorMsg = "配置文件中 io_timeout_ms 值无效。";
                return false;
            }

            if (cfg.ioTimeoutMs < 0)
            {
                cfg.ioTimeoutMs = 0; // 负数当作不设置
            }
        }
        else if (key == "max_retry")
        {
            try
            {
                cfg.maxRetry = std::stoi(value);
            }
            catch (...)
            {
                errorMsg = "配置文件中 max_retry 值无效。";
                return false;
            }

            if (cfg.maxRetry < 1)
            {
                cfg.maxRetry = 1; // 至少尝试一次
            }
        }

    }

    if (cfg.serverIp.empty())
    {
        errorMsg = "配置文件缺少 server_ip。";
        return false;
    }
    if (cfg.fromAddress.empty())
    {
        errorMsg = "配置文件缺少 from_address。";
        return false;
    }
    // 如果配置要求启用认证，则需要提供用户名和密码
    if (cfg.useAuth)
    {
        if (cfg.username.empty() || cfg.password.empty())
        {
            errorMsg = "use_auth 已开启，但 username 或 password 为空。";
            return false;
        }
    }

    return true;
}

void ConfigLoader::Trim(std::string& s)
{
    auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };

    auto it = s.begin();
    while (it != s.end() && isSpace(*it)) ++it;
    s.erase(s.begin(), it);

    if (s.empty()) return;
    auto rit = s.end() - 1;
    while (rit >= s.begin() && isSpace(*rit))
    {
        if (rit == s.begin())
        {
            s.clear();
            return;
        }
        --rit;
    }
    s.erase(rit + 1, s.end());
}
