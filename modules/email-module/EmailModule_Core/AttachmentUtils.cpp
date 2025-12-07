#include "AttachmentUtils.h"
#include "Utils.h"

#include <fstream>
#include <sstream>
#include <algorithm>

// 从路径中提取文件名
static std::string ExtractFileName(const std::string& path)
{
    if (path.empty()) return {};

    // 同时支持 Windows 的 '\' 和 Unix 风格的 '/'
    std::size_t pos1 = path.find_last_of('\\');
    std::size_t pos2 = path.find_last_of('/');
    std::size_t pos = std::string::npos;

    if (pos1 == std::string::npos) pos = pos2;
    else if (pos2 == std::string::npos) pos = pos1;
    else pos = std::max(pos1, pos2);

    if (pos == std::string::npos)
        return path; // 没有路径分隔符，全是文件名

    if (pos + 1 >= path.size())
        return {}; // 不太正常的情况，比如以斜杠结尾

    return path.substr(pos + 1);
}

// 判断 str 是否以 suffix 结尾（C++17 里没有 std::string::ends_with，我们自己实现）
static bool EndsWith(const std::string& str, const std::string& suffix)
{
    if (suffix.size() > str.size())
        return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

// 根据扩展名猜测一个 Content-Type
static std::string GuessContentType(const std::string& fileName)
{
    // 转成小写后按常见后缀匹配
    std::string lower = fileName;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (EndsWith(lower, ".txt"))  return "text/plain";
    if (EndsWith(lower, ".html") || EndsWith(lower, ".htm")) return "text/html";
    if (EndsWith(lower, ".jpg") || EndsWith(lower, ".jpeg")) return "image/jpeg";
    if (EndsWith(lower, ".png"))  return "image/png";
    if (EndsWith(lower, ".pdf"))  return "application/pdf";

    // 默认二进制流
    return "application/octet-stream";
}

bool BuildAttachmentFromFile(
    const std::string& filePath,
    AttachmentInfo& out,
    std::string& errorMsg
)
{
    // 1. 打开文件（以二进制方式）
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open())
    {
        errorMsg = "无法打开附件文件: " + filePath;
        return false;
    }

    // 2. 读取整个文件内容到内存
    std::ostringstream oss;
    oss << ifs.rdbuf();

    if (!ifs.good() && !ifs.eof())
    {
        errorMsg = "读取附件文件时发生错误: " + filePath;
        return false;
    }

    std::string fileData = oss.str();

    // 3. Base64 编码
    std::string base64 = Base64Encode(fileData);

    // 4. 填充 AttachmentInfo
    std::string fileName = ExtractFileName(filePath);
    if (fileName.empty())
    {
        // 如果连文件名都解析不出，就用一个占位名字
        fileName = "attachment.bin";
    }

    out.fileName = fileName;
    out.contentType = GuessContentType(fileName);
    out.base64Content = base64;

    errorMsg.clear();
    return true;
}
