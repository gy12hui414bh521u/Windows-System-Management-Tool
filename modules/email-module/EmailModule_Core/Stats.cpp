#include "Stats.h"

#include <fstream>
#include <iostream>
#include <string>
#include <map>

// 从一行日志中提取时间字符串
// 返回中括号里的那一段时间；提取失败则返回空串。
static std::string ExtractTimeFromLogLine(const std::string& line)
{
    auto firstL = line.find('[');
    if (firstL == std::string::npos) return {};

    auto firstR = line.find(']', firstL + 1);
    if (firstR == std::string::npos) return {};

    return line.substr(firstL + 1, firstR - firstL - 1);
}

// 按收件人统计用的数据结构
struct RecipientStat
{
    long long success = 0;
    long long fail = 0;
    std::string lastTime; // 最后一次发送时间
};

// 从一条日志行中尽量提取收件人邮箱
static std::string ExtractRecipientFromLogLine(const std::string& line)
{
    // 格式一：... To=xxx, Subject=...
    {
        const std::string key = "To=";
        auto pos = line.find(key);
        if (pos != std::string::npos)
        {
            pos += key.size();
            // 以逗号或空格作为结束
            auto end = line.find_first_of(", ", pos);
            std::string email =
                (end == std::string::npos)
                ? line.substr(pos)
                : line.substr(pos, end - pos);

            if (!email.empty())
                return email;
        }
    }

    // 格式二：群发：成功发送给 xxx（第 N 封）
    {
        const std::string key = "群发：成功发送给 ";
        auto pos = line.find(key);
        if (pos != std::string::npos)
        {
            pos += key.size();
            auto end = line.find_first_of(" （", pos); 
            std::string email =
                (end == std::string::npos)
                ? line.substr(pos)
                : line.substr(pos, end - pos);

            if (!email.empty())
                return email;
        }
    }

    // 格式三：群发：发送给 xxx 失败，错误: ...
    {
        const std::string key = "群发：发送给 ";
        auto pos = line.find(key);
        if (pos != std::string::npos)
        {
            pos += key.size();
            auto end = line.find_first_of(" ，", pos); 
            std::string email =
                (end == std::string::npos)
                ? line.substr(pos)
                : line.substr(pos, end - pos);

            if (!email.empty())
                return email;
        }
    }

    return {};
}

bool ShowEmailStatistics(std::string& errorMsg)
{
    std::ifstream ifs("email.log");
    if (!ifs.is_open())
    {
        errorMsg = "无法打开 email.log，请先发送几封邮件生成日志。";
        return false;
    }

    std::string line;
    long long totalSuccess = 0;
    long long totalFail = 0;
    long long singleSuccess = 0;
    long long singleFail = 0;
    long long bulkSuccess = 0;
    long long bulkFail = 0;
    std::string lastSendTime;

    // 按收件人统计
    std::map<std::string, RecipientStat> recipientStats;

    while (std::getline(ifs, line))
    {
        if (line.empty())
            continue;

        bool isSuccess = false;
        bool isFail = false;

        // 单封发送相关
        if (line.find("邮件发送成功") != std::string::npos)
        {
            isSuccess = true;
            ++singleSuccess;
        }
        else if (line.find("邮件发送失败") != std::string::npos)
        {
            isFail = true;
            ++singleFail;
        }
        // 群发相关
        else if (line.find("群发：成功发送给") != std::string::npos)
        {
            isSuccess = true;
            ++bulkSuccess;
        }
        else if (line.find("群发：发送给") != std::string::npos
            && line.find("失败") != std::string::npos)
        {
            isFail = true;
            ++bulkFail;
        }

        if (isSuccess || isFail)
        {
            // 更新时间
            std::string timeStr = ExtractTimeFromLogLine(line);
            if (!timeStr.empty())
            {
                // 日志是追加写入的，所以最后覆盖的是最近一次发送时间
                lastSendTime = timeStr;
            }

            if (isSuccess) ++totalSuccess;
            if (isFail)    ++totalFail;

            // 尝试解析收件人邮箱做按收件人统计
            std::string email = ExtractRecipientFromLogLine(line);
            if (!email.empty())
            {
                RecipientStat& st = recipientStats[email];
                if (isSuccess) ++st.success;
                if (isFail)    ++st.fail;
                if (!timeStr.empty())
                {
                    st.lastTime = timeStr;
                }
            }
        }
    }

    if (totalSuccess + totalFail == 0)
    {
        std::cout << "\n[统计] 日志中还没有发送相关记录，请先发送几封邮件再来看统计。\n";
        errorMsg.clear();
        return true;
    }

    // ===== 原有总览统计 =====
    std::cout << "\n===== 邮件发送统计 =====\n";
    std::cout << "总发送尝试次数 : " << (totalSuccess + totalFail) << "\n";
    std::cout << "  成功次数     : " << totalSuccess << "\n";
    std::cout << "  失败次数     : " << totalFail << "\n\n";

    std::cout << "  其中：\n";
    std::cout << "    单封成功   : " << singleSuccess << "\n";
    std::cout << "    单封失败   : " << singleFail << "\n";
    std::cout << "    群发成功   : " << bulkSuccess << "\n";
    std::cout << "    群发失败   : " << bulkFail << "\n\n";

    if (!lastSendTime.empty())
    {
        std::cout << "最后一次发送时间 : " << lastSendTime << "\n";
    }
    else
    {
        std::cout << "最后一次发送时间 : （无法从日志提取时间）\n";
    }

    std::cout << "========================\n";

    // ===== 按收件人统计 =====
    if (!recipientStats.empty())
    {
        std::cout << "\n===== 按收件人统计 =====\n";
        for (const auto& pair : recipientStats)
        {
            const std::string& email = pair.first;
            const RecipientStat& st = pair.second;

            std::cout << email
                << "  成功: " << st.success
                << "  失败: " << st.fail;

            if (!st.lastTime.empty())
            {
                std::cout << "  最后发送: " << st.lastTime;
            }
            std::cout << "\n";
        }
        std::cout << "========================\n";
    }

    errorMsg.clear();
    return true;
}
