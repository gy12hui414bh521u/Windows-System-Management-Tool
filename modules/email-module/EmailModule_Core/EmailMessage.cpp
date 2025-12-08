#include "EmailMessage.h"
#include "Utils.h"

#include <sstream>

// 只在本文件内部使用的一些小工具
namespace
{
    // 生成一个简单的 MIME 边界字符串，用于 multipart/mixed
    std::string GenerateBoundary()
    {
        static int counter = 0;
        std::ostringstream oss;
        oss << "----=CppEmailBoundary_" << ++counter;
        return oss.str();
    }

    // 把 Base64 字符串按行拆分，每行最多 lineLen 个字符（默认 76）
    std::string WrapBase64Lines(const std::string& base64, std::size_t lineLen = 76)
    {
        std::string out;
        out.reserve(base64.size() + base64.size() / lineLen * 2);

        std::size_t n = base64.size();
        for (std::size_t i = 0; i < n; i += lineLen)
        {
            std::size_t len = (i + lineLen <= n) ? lineLen : (n - i);
            out.append(base64, i, len);
            out.append("\r\n");
        }
        return out;
    }
}

std::string EmailMessageBuilder::Build(const SmtpConfig& cfg, const SimpleEmail& mail)
{
    std::ostringstream oss;

    // 公共头部：From / To / Subject / Date
    oss << "From: " << cfg.fromAddress << "\r\n";
    oss << "To: " << mail.to << "\r\n";
    oss << "Subject: " << EncodeHeaderUtf8B(mail.subject) << "\r\n";
    oss << "Date: " << GetCurrentTimeString() << "\r\n";

    // 1：没有附件 → 维持原来的“纯文本邮件”结构
    if (mail.attachments.empty())
    {
        oss << "MIME-Version: 1.0\r\n";
        oss << "Content-Type: text/plain; charset=\"UTF-8\"\r\n";
        // Content-Transfer-Encoding 可以写 8bit，表示直接传输 UTF-8 文本
        oss << "Content-Transfer-Encoding: 8bit\r\n";
        oss << "\r\n";

        oss << mail.body;
        return oss.str();
    }

    // 2：有附件 → 构造 multipart/mixed
    std::string boundary = GenerateBoundary();

    oss << "MIME-Version: 1.0\r\n";
    oss << "Content-Type: multipart/mixed; boundary=\"" << boundary << "\"\r\n";
    oss << "\r\n";

    // 说明文字
    oss << "This is a multi-part message in MIME format.\r\n";
    oss << "\r\n";

    // 文本部分
    oss << "--" << boundary << "\r\n";
    oss << "Content-Type: text/plain; charset=\"UTF-8\"\r\n";
    oss << "Content-Transfer-Encoding: 8bit\r\n";
    oss << "\r\n";
    oss << mail.body << "\r\n";

    // 附件部分
    for (const auto& att : mail.attachments)
    {
        oss << "--" << boundary << "\r\n";
        oss << "Content-Type: " << att.contentType
            << "; name=\"" << att.fileName << "\"\r\n";
        oss << "Content-Transfer-Encoding: base64\r\n";
        oss << "Content-Disposition: attachment; filename=\""
            << att.fileName << "\"\r\n";
        oss << "\r\n";

        // Base64 内容按行分段，避免一行太长
        oss << WrapBase64Lines(att.base64Content);
        oss << "\r\n";
    }

    // 结束边界
    oss << "--" << boundary << "--\r\n";

    return oss.str();
}
