#include "Utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string GetCurrentTimeString()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &t);

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
std::string Base64Encode(const std::string& data)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    const unsigned char* bytes =
        reinterpret_cast<const unsigned char*>(data.data());
    std::size_t i = 0;
    std::size_t n = data.size();

    while (i + 2 < n)
    {
        unsigned int v = (bytes[i] << 16) |
            (bytes[i + 1] << 8) |
            (bytes[i + 2]);
        out.push_back(table[(v >> 18) & 0x3F]);
        out.push_back(table[(v >> 12) & 0x3F]);
        out.push_back(table[(v >> 6) & 0x3F]);
        out.push_back(table[v & 0x3F]);
        i += 3;
    }

    if (i < n)
    {
        unsigned int v = bytes[i] << 16;
        bool twoBytes = false;
        if (i + 1 < n)
        {
            v |= (bytes[i + 1] << 8);
            twoBytes = true;
        }

        out.push_back(table[(v >> 18) & 0x3F]);
        out.push_back(table[(v >> 12) & 0x3F]);

        if (twoBytes)
        {
            out.push_back(table[(v >> 6) & 0x3F]);
            out.push_back('=');
        }
        else
        {
            out.push_back('=');
            out.push_back('=');
        }
    }

    return out;
}

std::string EncodeHeaderUtf8B(const std::string& text)
{
    if (text.empty())
        return text;

    // 检查是否包含非 ASCII 字符（>= 0x80）
    bool needEncode = false;
    for (unsigned char ch : text)
    {
        if (ch >= 0x80)
        {
            needEncode = true;
            break;
        }
    }

    // 纯英文 / 纯 ASCII：直接返回原字符串，不做编码
    if (!needEncode)
    {
        return text;
    }

    // 出现了中文或其他非 ASCII：用 UTF-8 Base64 头部编码
    return "=?utf-8?B?" + Base64Encode(text) + "?=";
}
std::string NormalizeNewlinesToCRLF(const std::string& text)
{
    std::string out;
    out.reserve(text.size() + 16);

    const std::size_t n = text.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        char ch = text[i];
        if (ch == '\r')
        {
            // 已经是 \r\n 的情况
            if (i + 1 < n && text[i + 1] == '\n')
            {
                out.push_back('\r');
                out.push_back('\n');
                ++i; // 跳过 \n
            }
            else
            {
                // 单独的 \r，也规整成 \r\n
                out.push_back('\r');
                out.push_back('\n');
            }
        }
        else if (ch == '\n')
        {
            // 单独的 \n → \r\n
            out.push_back('\r');
            out.push_back('\n');
        }
        else
        {
            out.push_back(ch);
        }
    }

    return out;
}

