#include "Pop3Client.h"
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

// 链接网络库
#pragma comment(lib, "ws2_32.lib")

// 使用匿名空间，防止和 SmtpClient 里的类名冲突
namespace {

    // 1. 网络初始化器
    class WsaInit {
    public:
        WsaInit() {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        }
        ~WsaInit() { WSACleanup(); }
    };

    // 2. Socket 包装类
    class Pop3Socket {
        SOCKET sock;
    public:
        Pop3Socket() : sock(INVALID_SOCKET) {}
        ~Pop3Socket() { Close(); }

        void Close() {
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
        }

        bool Connect(const std::string& host, int port, std::string& errMsg) {
            Close();
            struct addrinfo hints = { 0 }, * result = nullptr;
            hints.ai_family = AF_UNSPEC; // 支持 IPv4 和 IPv6
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            std::string portStr = std::to_string(port);
            if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
                errMsg = "DNS解析失败: " + host; return false;
            }

            // 尝试每一个解析出来的地址
            for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
                sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if (sock == INVALID_SOCKET) continue;

                if (connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR) {
                    break; // 连接成功
                }
                Close();
            }

            freeaddrinfo(result);

            if (sock == INVALID_SOCKET) {
                errMsg = "无法连接到服务器"; return false;
            }
            return true;
        }

        bool SendAll(const std::string& data, std::string& errMsg) {
            if (send(sock, data.c_str(), (int)data.length(), 0) == SOCKET_ERROR) {
                errMsg = "发送数据失败"; return false;
            }
            return true;
        }

        // 简单接收
        int Recv(char* buf, int maxLen) {
            return recv(sock, buf, maxLen, 0);
        }
    };
}

// 辅助函数：发送命令并检查
static bool SendAndCheck(Pop3Socket& socket, const std::string& cmd, std::string& errorMsg) {
    std::string err;
    if (!socket.SendAll(cmd + "\r\n", err)) {
        errorMsg = "发送命令失败: " + cmd; return false;
    }

    char buf[1024] = { 0 };
    int n = socket.Recv(buf, 1023);
    if (n <= 0) {
        errorMsg = "服务器无响应"; return false;
    }

    std::string resp(buf);
    if (resp.find("+OK") != 0) {
        // 如果服务器返回 -ERR，虽然也是响应，但代表操作失败
        errorMsg = "服务器报错: " + resp; return false;
    }
    return true;
}

// 核心功能实现
bool Pop3Client::CheckEmailCount(const std::string& host, int port, std::string& outInfo, std::string& errorMsg) {
    WsaInit init; // 确保网络库已初始化
    Pop3Socket socket;

    // 1. 连接
    if (!socket.Connect(host, port, errorMsg)) {
        return false;
    }

    // 2. 接收欢迎语
    char buf[1024] = { 0 };
    socket.Recv(buf, 1023);

    // 3. 登录
    if (!SendAndCheck(socket, "USER test", errorMsg)) return false;
    if (!SendAndCheck(socket, "PASS test", errorMsg)) return false;

    // 4. 查询统计 (STAT)
    std::string err;
    socket.SendAll("STAT\r\n", err);

    memset(buf, 0, sizeof(buf));
    socket.Recv(buf, 1023);
    std::string statResp(buf);

    if (statResp.find("+OK") != 0) {
        errorMsg = "查询失败: " + statResp; return false;
    }

    // 5. 退出
    socket.SendAll("QUIT\r\n", err);

    // 6. 整理结果
    outInfo = "连接成功！\n[服务器响应] " + statResp + "(格式: 邮件数量 总字节数)\n请前往 smtp4dev 网页查看详情。";

    return true;
}