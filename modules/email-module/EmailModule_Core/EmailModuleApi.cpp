#include "EmailModuleApi.h"
#include "Pop3Client.h" 
#include "Config.h"
#include "EmailMessage.h"
#include "SmtpClient.h"
#include "Logger.h"
#include "BulkSender.h"
#include "Stats.h"
#include "LogSearch.h"

#include <string>

namespace
{
    // 统一加载配置
    bool LoadSmtpConfigOrError(SmtpConfig& cfg, std::string& errorMsg)
    {
        if (!ConfigLoader::LoadSmtpConfig(cfg, errorMsg))
        {
            EmailLogger::Error("加载 SMTP 配置失败: " + errorMsg);
            return false;
        }
        return true;
    }
}

namespace EmailModule
{
    bool SendSimpleMail(const std::string& to,
        const std::string& subject,
        const std::string& body,
        std::string& errorMsg)
    {
        SmtpConfig cfg;
        if (!LoadSmtpConfigOrError(cfg, errorMsg))
        {
            return false;
        }

        SimpleEmail mail{};
        mail.to = to;
        mail.subject = subject;
        mail.body = body;

        std::string raw = EmailMessageBuilder::Build(cfg, mail);

        SmtpClient client;
        std::string sendErr;
        bool ok = client.SendMail(cfg, raw, sendErr);

        if (ok)
        {
            EmailLogger::Info("简单邮件发送成功，To=" + to + ", Subject=" + subject);
            errorMsg.clear();
            return true;
        }
        else
        {
            errorMsg = sendErr;
            EmailLogger::Error("简单邮件发送失败: " + sendErr);
            return false;
        }
    }

    bool SendBulkMails(std::string& errorMsg)
    {
        SmtpConfig cfg;
        if (!LoadSmtpConfigOrError(cfg, errorMsg))
        {
            return false;
        }

        bool ok = ::SendBulkMails(cfg, errorMsg);
        if (ok)
        {
            EmailLogger::Info("群发完成：全部收件人发送成功（通过 API）。");
        }
        else
        {
            EmailLogger::Error("群发结束：存在错误（通过 API） - " + errorMsg);
        }

        return ok;
    }

    bool ShowStatistics(std::string& errorMsg)
    {
        bool ok = ::ShowEmailStatistics(errorMsg);
        if (!ok)
        {
            EmailLogger::Error("统计失败（通过 API 调用）: " + errorMsg);
        }
        return ok;
    }

    bool SearchLog(const std::string& keyword,
        const std::string& startTime,
        const std::string& endTime,
        std::string& errorMsg)
    {
        bool ok = ::SearchLogByKeyword(keyword, startTime, endTime, errorMsg);
        if (!ok)
        {
            EmailLogger::Error("日志搜索失败（通过 API 调用）: " + errorMsg);
        }
        return ok;
    }

    bool CheckInbox(std::string& outputInfo, std::string& errorMsg) {
        // 1. 先加载配置，为了拿到服务器 IP
        SmtpConfig cfg;
        if (!ConfigLoader::LoadSmtpConfig(cfg, errorMsg)) {
            return false;
        }

        // 2. 启动 POP3 客户端
        Pop3Client pop3;
        // smtp4dev 的 POP3 端口默认是 110
        return pop3.CheckEmailCount(cfg.serverIp, 110, outputInfo, errorMsg);
    }
}
