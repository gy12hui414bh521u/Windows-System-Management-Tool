#pragma once
#include <string>

namespace EmailModule
{
    // 发送一封最简单的文本邮件（不带附件）。
    // 参数：
    //   to      : 收件人邮箱
    //   subject : 主题（UTF-8）
    //   body    : 正文（UTF-8，简单文本）
    //   errorMsg: 出错时返回错误信息
    //
    // 返回值：
    //   true  = 发送成功（已经被 SMTP 服务器接受）
    //   false = 发送失败（errorMsg 中有原因）
    bool SendSimpleMail(const std::string& to,
        const std::string& subject,
        const std::string& body,
        std::string& errorMsg);

    // 按照 recipients.txt + mail_template.txt 群发邮件。
    // 内部会自动读取 email.conf 获取 SMTP 配置。
    bool SendBulkMails(std::string& errorMsg);

    // 显示发送统计信息（在控制台打印），并通过 errorMsg 返回错误。
    bool ShowStatistics(std::string& errorMsg);

    // 按关键字搜索日志（email.log），可选时间范围过滤。
    // 所有输出打到控制台，errorMsg 只在出错（例如日志不存在）时使用。
    bool SearchLog(const std::string& keyword,
        const std::string& startTime,
        const std::string& endTime,
        std::string& errorMsg);
    // 检查收件箱 (POP3)
    // bool CheckInbox(std::string& outputInfo, std::string& errorMsg);
}

