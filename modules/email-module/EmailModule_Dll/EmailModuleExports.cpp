#include "EmailModuleApi.h"

#include <string>
#include <cstring>

// 说明：
// 这些是 DLL 的 C 接口导出函数，将来组长的主程序只需要包含对应声明，就能调用。
// 我们内部使用 EmailModule 命名空间下的 C++ 接口来完成实际功能。

// 辅助函数：把 std::string 的错误信息拷贝到调用方提供的缓冲区
static void CopyErrorToBuffer(const std::string& err, char* buffer, int bufferSize)
{
    if (!buffer || bufferSize <= 0)
        return;

    // 使用安全版本的拷贝，自动截断
#if defined(_MSC_VER)
    strncpy_s(buffer, static_cast<size_t>(bufferSize), err.c_str(), _TRUNCATE);
#else
    std::strncpy(buffer, err.c_str(), static_cast<size_t>(bufferSize - 1));
    buffer[bufferSize - 1] = '\0';
#endif
}

// 1) 导出：发送一封简单文本邮件（不带附件）
//
// 参数：
//   to/subject/body : UTF-8 字符串（C 字符串）
//   errorBuf        : 调用方提供的缓冲区，用来接收错误信息（可为 nullptr）
//   errorBufSize    : 缓冲区大小（字节数）
//
// 返回值：
//   true  = 发送成功
//   false = 发送失败（错误信息写入 errorBuf）
extern "C" __declspec(dllexport)
bool Email_SendSimple(const char* to,
    const char* subject,
    const char* body,
    char* errorBuf,
    int errorBufSize)
{
    std::string err;

    std::string toStr = to ? to : "";
    std::string subjectStr = subject ? subject : "";
    std::string bodyStr = body ? body : "";

    bool ok = EmailModule::SendSimpleMail(toStr, subjectStr, bodyStr, err);
    if (!ok)
    {
        CopyErrorToBuffer(err, errorBuf, errorBufSize);
    }
    return ok;
}

// 2) 导出：按照 recipients.txt + mail_template.txt 群发
extern "C" __declspec(dllexport)
bool Email_SendBulk(char* errorBuf, int errorBufSize)
{
    std::string err;
    bool ok = EmailModule::SendBulkMails(err);
    if (!ok)
    {
        CopyErrorToBuffer(err, errorBuf, errorBufSize);
    }
    return ok;
}

// 3) 导出：显示统计（直接在控制台打印）
//    返回 false 表示统计过程中出错（例如 email.log 不存在）
extern "C" __declspec(dllexport)
bool Email_ShowStats(char* errorBuf, int errorBufSize)
{
    std::string err;
    bool ok = EmailModule::ShowStatistics(err);
    if (!ok)
    {
        CopyErrorToBuffer(err, errorBuf, errorBufSize);
    }
    return ok;
}

// 4) 导出：按关键字搜索日志，可选时间范围
extern "C" __declspec(dllexport)
bool Email_SearchLog(const char* keyword,
    const char* startTime,
    const char* endTime,
    char* errorBuf,
    int errorBufSize)
{
    std::string err;

    std::string kwStr = keyword ? keyword : "";
    std::string stStr = startTime ? startTime : "";
    std::string edStr = endTime ? endTime : "";

    bool ok = EmailModule::SearchLog(kwStr, stStr, edStr, err);
    if (!ok)
    {
        CopyErrorToBuffer(err, errorBuf, errorBufSize);
    }
    return ok;
}
