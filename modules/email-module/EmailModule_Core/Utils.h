#pragma once
#include <string>

// 返回当前本地时间字符串，例如 "2025-11-23 11:45:00"
std::string GetCurrentTimeString();

// 将字节串做 Base64 编码（用于邮件头编码）
std::string Base64Encode(const std::string& data);

// 把 UTF-8 文本按 RFC 2047 的 style 编成 "=?utf-8?B?...?="，用于 Subject 等头部字段
std::string EncodeHeaderUtf8B(const std::string& text);

// 将文本中的换行统一为 CRLF（\r\n），避免出现裸LF。
std::string NormalizeNewlinesToCRLF(const std::string& text);