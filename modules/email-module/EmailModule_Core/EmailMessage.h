#pragma once
#include <string>
#include <vector>   
#include "Config.h"

// 附件信息：目前存的是“已经编码好的内容”，真正读文件在别的模块做
struct AttachmentInfo
{
    std::string fileName;       // 附件文件名，例如 "report.txt"
    std::string contentType;    // MIME 类型，例如 "text/plain" 或 "application/octet-stream"
    std::string base64Content;  // 附件内容的 Base64 字符串
};

// 简单邮件数据结构
struct SimpleEmail
{
    std::string to;
    std::string subject;
    std::string body;
    std::vector<AttachmentInfo> attachments;  // 附件列表
};

// 负责构造完整的文本邮件字符串
class EmailMessageBuilder
{
public:
    static std::string Build(const SmtpConfig& cfg, const SimpleEmail& mail);
};
