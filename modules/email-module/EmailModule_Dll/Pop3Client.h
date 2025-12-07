#pragma once
#include <string>


class Pop3Client {
public:
    // 连接服务器，询问有多少封邮件，返回一个简报字符串
    bool CheckEmailCount(const std::string& host, int port, std::string& outInfo, std::string& errorMsg);
};
