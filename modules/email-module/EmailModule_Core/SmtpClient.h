#pragma once
#include <string>
#include "Config.h"

// 通过 WinSock 使用 SMTP 协议发送邮件
class SmtpClient
{
public:
    // rawEmail 由 EmailMessageBuilder 构造
    bool SendMail(const SmtpConfig& cfg, const std::string& rawEmail, std::string& errorMsg);
};

