#pragma once
#include <string>
#include "EmailMessage.h"

// 从给定文件路径构造一个 AttachmentInfo：
// - 读取整个文件内容
// - 做 Base64 编码
// - 根据路径推断文件名
// - 根据扩展名推断一个大致的 Content-Type（仅简单匹配）
//
// 参数：
//   filePath : 要作为附件的文件路径
//   out      : 输出的附件信息（fileName / contentType / base64Content）
//   errorMsg : 出错时写入错误描述（例如“无法打开文件...”）
bool BuildAttachmentFromFile(
    const std::string& filePath,
    AttachmentInfo& out,
    std::string& errorMsg
);

