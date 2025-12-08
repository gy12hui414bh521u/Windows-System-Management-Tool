#include "BulkSender.h"

#include "TemplateEngine.h"
#include "EmailMessage.h"
#include "SmtpClient.h"
#include "Logger.h"
#include "Utils.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cctype>

// 只在本文件内部使用的工具和数据结构
namespace
{
    struct Recipient
    {
        std::string email;
        std::string name;
    };

    bool IsWhitespace(char ch)
    {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    }

    void Trim(std::string& s)
    {
        auto it = s.begin();
        while (it != s.end() && IsWhitespace(*it)) ++it;
        s.erase(s.begin(), it);

        if (s.empty()) return;
        auto rit = s.end() - 1;
        while (rit >= s.begin() && IsWhitespace(*rit))
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

    // 解析一行 "email,name" → Recipient
    bool ParseRecipientLine(const std::string& line, Recipient& out)
    {
        // 忽略空行和注释行
        if (line.empty() || line[0] == '#')
            return false;

        auto pos = line.find(',');
        if (pos == std::string::npos)
        {
            return false;
        }

        std::string email = line.substr(0, pos);
        std::string name = line.substr(pos + 1);

        Trim(email);
        Trim(name);

        if (email.empty())
        {
            return false;
        }

        out.email = email;
        out.name = name;
        return true;
    }

    // 从 recipients.txt 读取收件人列表
    bool LoadRecipients(std::vector<Recipient>& recipients, std::string& errorMsg)
    {
        std::ifstream ifs("recipients.txt");
        if (!ifs.is_open())
        {
            errorMsg = "无法打开 recipients.txt，请确认文件在程序运行目录下。";
            return false;
        }

        std::string line;
        int lineNumber = 0;
        while (std::getline(ifs, line))
        {
            ++lineNumber;
            Recipient r;
            if (ParseRecipientLine(line, r))
            {
                recipients.push_back(r);
            }
            else
            {
                // 无效行可以选择忽略，也可以记日志
                // 这里我们简单忽略空行/注释行，对真正格式错误的行记一下日志
                if (!line.empty() && line[0] != '#')
                {
                    EmailLogger::Error("解析 recipients.txt 第 " + std::to_string(lineNumber) +
                        " 行失败，内容为: " + line);
                }
            }
        }

        if (recipients.empty())
        {
            errorMsg = "recipients.txt 中没有有效的收件人。";
            return false;
        }

        return true;
    }

    // 从 mail_template.txt 读取整个模板文本
    bool LoadMailTemplate(std::string& tpl, std::string& errorMsg)
    {
        std::ifstream ifs("mail_template.txt");
        if (!ifs.is_open())
        {
            errorMsg = "无法打开 mail_template.txt，请确认文件在程序运行目录下。";
            return false;
        }

        std::ostringstream oss;
        oss << ifs.rdbuf();
        tpl = oss.str();

        if (tpl.empty())
        {
            errorMsg = "mail_template.txt 内容为空。";
            return false;
        }

        return true;
    }
} // namespace

bool SendBulkMails(const SmtpConfig& cfg, std::string& errorMsg)
{
    std::vector<Recipient> recipients;
    std::string            err;

    // 1. 读收件人列表
    if (!LoadRecipients(recipients, err))
    {
        errorMsg = err;
        EmailLogger::Error("群发：加载收件人列表失败: " + err);
        return false;
    }

    // 2. 读模板内容
    std::string templateText;
    if (!LoadMailTemplate(templateText, err))
    {
        errorMsg = err;
        EmailLogger::Error("群发：加载模板文件失败: " + err);
        return false;
    }

    EmailLogger::Info("群发：开始发送邮件，总收件人数 = " + std::to_string(recipients.size()));

    int successCount = 0;
    int failCount = 0;
    int index = 0;

    SmtpClient client;

    for (const auto& r : recipients)
    {
        ++index;

        // 3. 构造模板变量
        std::map<std::string, std::string> vars;
        vars["name"] = r.name;                       // 收件人名字（可能为空）
        vars["index"] = std::to_string(index);        // 第几封
        vars["time"] = GetCurrentTimeString();       // 当前时间

        // 4. 生成正文
        std::string body = RenderTemplate(templateText, vars);

        // 5. 构造邮件
        SimpleEmail mail;
        mail.to = r.email;
        mail.subject = "群发测试邮件 #" + std::to_string(index);
        mail.body = body;

        std::string raw = EmailMessageBuilder::Build(cfg, mail);

        // 6. 发送
        std::string sendError;
        bool ok = client.SendMail(cfg, raw, sendError);
        if (ok)
        {
            ++successCount;
            EmailLogger::Info("群发：成功发送给 " + r.email +
                "（第 " + std::to_string(index) + " 封）");
        }
        else
        {
            ++failCount;
            EmailLogger::Error("群发：发送给 " + r.email +
                " 失败，错误: " + sendError);
        }
    }

    // 7. 总结
    std::ostringstream summary;
    summary << "群发完成：成功 " << successCount << " 封，失败 " << failCount
        << " 封，总计 " << recipients.size() << " 封。";
    EmailLogger::Info(summary.str());

    if (failCount > 0)
    {
        errorMsg = summary.str();
        return false;
    }

    errorMsg.clear();
    return true;
}
