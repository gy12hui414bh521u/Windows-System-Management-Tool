#pragma once
#include <string>
#include "Config.h"

// 从 recipients.txt 和 mail_template.txt 读取信息，按模板群发邮件。
// cfg：SMTP 配置（调用方通常从 ConfigLoader::LoadSmtpConfig 得到）
// errorMsg：失败时返回概要错误信息（同时写入日志）
//
// 返回值：
//  - true  ：全部收件人发送成功
//  - false ：至少有一封发送失败（具体信息在日志和 errorMsg 中）
bool SendBulkMails(const SmtpConfig& cfg, std::string& errorMsg);

