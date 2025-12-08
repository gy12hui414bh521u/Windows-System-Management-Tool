#include "Menu.h"
#include "SystemMonitor.h"  
#include <iostream>
#include <string>

#include "Config.h"
#include "Logger.h"
#include "EmailMessage.h"
#include "SmtpClient.h"
#include "BulkSender.h"
#include "Stats.h"
#include "AttachmentUtils.h"   
#include "LogSearch.h"



namespace
{
    int ReadIntChoice()
    {
        std::string line;
        std::getline(std::cin, line);
        try
        {
            return std::stoi(line);
        }
        catch (...)
        {
            return -1;
        }
    }
}

void RunMenu()
{
    SmtpConfig   cfg;
    std::string  err;
    if (!ConfigLoader::LoadSmtpConfig(cfg, err))
    {
        std::cout << "加载 SMTP 配置失败：\n" << err << "\n";
        EmailLogger::Error("加载配置失败: " + err);
        std::cout << "请先在项目目录创建 email.conf，然后重新运行本程序。\n";
        std::cout << "按回车键退出...";
        std::cin.get();
        return;
    }

    EmailLogger::Info("SMTP 配置加载成功。");

    while (true)
    {
        std::cout << "\n===== Email 管理模块 - Level 1 & 群发 =====\n";
        std::cout << "1. 发送单封测试邮件\n";
        std::cout << "2. 从 recipients.txt 群发测试邮件\n";
        std::cout << "3. 查看发送统计信息\n";
        std::cout << "4. 发送带附件的单封邮件\n";
        std::cout << "5. 按关键字搜索日志(email.log)\n";
        std::cout << "6. 启动系统资源监控\n";  
        std::cout << "0. 退出\n";
        std::cout << "请选择：";

        int choice = ReadIntChoice();
        if (choice == 0)
        {
            std::cout << "退出程序。\n";
            break;
        }
        else if (choice == 1)
        {
            SimpleEmail mail{};
            std::cout << "请输入收件人邮箱（To）: ";
            std::getline(std::cin, mail.to);

            std::cout << "请输入邮件主题: ";
            std::getline(std::cin, mail.subject);

            std::cout << "请输入邮件正文（单行，简单版）: ";
            std::getline(std::cin, mail.body);

            std::string raw = EmailMessageBuilder::Build(cfg, mail);

            SmtpClient    client;
            std::string   errorMsg;
            std::cout << "正在发送邮件，请稍候...\n";
            bool ok = client.SendMail(cfg, raw, errorMsg);

            if (ok)
            {
                std::cout << "邮件发送成功！\n";
                EmailLogger::Info("邮件发送成功，To=" + mail.to + ", Subject=" + mail.subject);
            }
            else
            {
                std::cout << "邮件发送失败：\n" << errorMsg << "\n";
                EmailLogger::Error("邮件发送失败: " + errorMsg);
            }
        }
        else if (choice == 2)
        {
            std::cout << "即将按照 recipients.txt 和 mail_template.txt 群发测试邮件...\n";

            std::string errorMsg;
            bool ok = SendBulkMails(cfg, errorMsg);

            if (ok)
            {
                std::cout << "群发完成：全部收件人发送成功。\n";
                EmailLogger::Info("群发完成：全部收件人发送成功。");
            }
            else
            {
                std::cout << "群发完成，但存在错误：\n" << errorMsg << "\n";
                EmailLogger::Error("群发结束：存在错误 - " + errorMsg);
            }
        }
        else if (choice == 3)
        {
            std::string errorMsg;
            bool ok = ShowEmailStatistics(errorMsg);
            if (!ok)
            {
                std::cout << "统计失败：\n" << errorMsg << "\n";
                EmailLogger::Error("统计失败: " + errorMsg);
            }
        }
        else if (choice == 4)
        {
            // ===== 新增：发送带附件的单封邮件 =====
            SimpleEmail mail{};
            std::cout << "请输入收件人邮箱（To）: ";
            std::getline(std::cin, mail.to);

            std::cout << "请输入邮件主题: ";
            std::getline(std::cin, mail.subject);

            std::cout << "请输入邮件正文（单行，简单版）: ";
            std::getline(std::cin, mail.body);

            std::cout << "请输入附件文件路径（例如：G:\\test\\attach.txt）: ";
            std::string filePath;
            std::getline(std::cin, filePath);

            AttachmentInfo att;
            std::string    buildErr;
            if (!BuildAttachmentFromFile(filePath, att, buildErr))
            {
                std::cout << "构造附件失败：\n" << buildErr << "\n";
                EmailLogger::Error("构造附件失败: " + buildErr);
            }
            else
            {
                // 把附件塞到邮件里
                mail.attachments.push_back(att);

                std::string raw = EmailMessageBuilder::Build(cfg, mail);

                SmtpClient  client;
                std::string sendErr;
                std::cout << "正在发送带附件的邮件，请稍候...\n";
                bool ok = client.SendMail(cfg, raw, sendErr);
                if (ok)
                {
                    std::cout << "带附件的邮件发送成功！\n";
                    EmailLogger::Info("带附件的邮件发送成功，To=" + mail.to + ", Subject=" + mail.subject +
                        ", Attachment=" + att.fileName);
                }
                else
                {
                    std::cout << "带附件的邮件发送失败：\n" << sendErr << "\n";
                    EmailLogger::Error("带附件的邮件发送失败: " + sendErr);
                }
            }
        }
        else if (choice == 5)
        {
            std::cout << "请输入要搜索的关键字：";
            std::string keyword;
            std::getline(std::cin, keyword);

            if (keyword.empty())
            {
                std::cout << "关键字为空，已取消搜索。\n";
            }
            else
            {
                std::cout << "可选：请输入起始时间（YYYY-MM-DD 或 YYYY-MM-DD HH:MM:SS，直接回车表示不限制）：";
                std::string startTime;
                std::getline(std::cin, startTime);

                std::cout << "可选：请输入结束时间（同上，直接回车表示不限制）：";
                std::string endTime;
                std::getline(std::cin, endTime);

                std::string errorMsg;
                bool ok = SearchLogByKeyword(keyword, startTime, endTime, errorMsg);
                if (!ok)
                {
                    std::cout << "日志搜索失败：\n" << errorMsg << "\n";
                    EmailLogger::Error("日志搜索失败: " + errorMsg);
                }
            }
        }
        else if (choice == 6)
        {
            std::cout << "=== 全栈系统监控中心 v3.0 ===\n";

            // 1. 设置管理员
            std::string adminEmail;
            std::cout << "请输入报警接收邮箱 (回车默认: " << cfg.fromAddress << "): ";
            std::string inputEmail;
            std::getline(std::cin, inputEmail);
            adminEmail = inputEmail.empty() ? cfg.fromAddress : inputEmail;

            // 2. 监控进程
            std::string targetProcess;
            std::cout << "请输入要守护的关键进程名 (例如 notepad.exe，回车跳过): ";
            std::getline(std::cin, targetProcess);

            // 3. 设定阈值 
            double diskLimit = 20.0; // 故意很高，触发磁盘报警
            int memLimit = 90;           // 90%
            int cpuLimit = 80;            // 触发CPU报警

            std::cout << "正在初始化监控探针 (CPU采样中，请稍候 0.5秒)...\n";

            std::string errorMsg;
            // 调用接口
            bool ok = SystemMonitor::CheckAndAlert(
                cfg,
                adminEmail,
                diskLimit,
                memLimit,
                cpuLimit,       // 传入 CPU阈值
                targetProcess,  // 传入 进程名
                errorMsg
            );

            if (!ok) std::cout << "监控报错: " << errorMsg << "\n";
            else std::cout << "监控周期结束。\n";
        }
        else
        {
            std::cout << "无效的选项，请重试。\n";
        }

    }
}
