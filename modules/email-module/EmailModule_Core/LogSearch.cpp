#include "LogSearch.h"

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

// 简单的大小写不敏感匹配（只对英文字母大小写转换，对中文原样）
static bool ContainsCaseInsensitive(const std::string& text, const std::string& keyword)
{
    if (keyword.empty())
        return false;

    std::string t = text;
    std::string k = keyword;

    auto toLower = [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
        };

    std::transform(t.begin(), t.end(), t.begin(), toLower);
    std::transform(k.begin(), k.end(), k.begin(), toLower);

    return t.find(k) != std::string::npos;
}

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

// 规范化时间字符串到 "YYYY-MM-DD HH:MM:SS" 用于比较
static std::string NormalizeTimeKey(const std::string& input, bool isStart)
{
    if (input.empty())
        return {};

    if (input.size() >= 19)
    {
        return input.substr(0, 19);
    }
    else if (input.size() == 10)
    {
        if (isStart)
            return input + " 00:00:00";
        else
            return input + " 23:59:59";
    }

    // 其他长度视为无效，不做时间过滤
    return {};
}

bool SearchLogByKeyword(const std::string& keyword,
    const std::string& startTimeStr,
    const std::string& endTimeStr,
    std::string& errorMsg)
{
    std::ifstream ifs("email.log");
    if (!ifs.is_open())
    {
        errorMsg = "无法打开 email.log，请先发送几封邮件生成日志。";
        return false;
    }

    // 规范化时间范围
    std::string normStart = NormalizeTimeKey(startTimeStr, true);
    std::string normEnd = NormalizeTimeKey(endTimeStr, false);

    std::string line;
    long long   matchCount = 0;
    long long   lineNumber = 0;

    std::cout << "\n===== 日志搜索结果 =====\n";
    std::cout << "关键字: \"" << keyword << "\"\n";
    if (!normStart.empty())
        std::cout << "起始时间: " << normStart << "\n";
    if (!normEnd.empty())
        std::cout << "结束时间: " << normEnd << "\n";
    if (normStart.empty() && normEnd.empty())
        std::cout << "时间范围: 不限制\n";
    std::cout << "----------------------------------------\n";

    while (std::getline(ifs, line))
    {
        ++lineNumber;

        // 时间范围过滤
        std::string timeStr = ExtractTimeFromLogLine(line);
        if (!timeStr.empty())
        {
            if (!normStart.empty() && timeStr < normStart)
                continue;
            if (!normEnd.empty() && timeStr > normEnd)
                continue;
        }
        // 如果提取不到时间，就当作不过滤时间

        // 关键字过滤
        if (!ContainsCaseInsensitive(line, keyword))
            continue;

        ++matchCount;
        std::cout << "[" << lineNumber << "] " << line << "\n";
    }

    if (matchCount == 0)
    {
        std::cout << "未找到匹配的日志行。\n";
    }
    else
    {
        std::cout << "----------------------------------------\n";
        std::cout << "共找到匹配行数: " << matchCount << "\n";
    }

    std::cout << "========================================\n";

    errorMsg.clear();
    return true;
}
