#include "SmtpClient.h"
#include "Utils.h"
#include "Config.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <sstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

namespace
{
    // --- RAII 封装 WSAStartup/WSACleanup ---
    class WsaSession
    {
    public:
        explicit WsaSession(std::string& errorMsg)
            : ok_(false)
        {
            WSADATA wsaData{};
            int r = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (r != 0)
            {
                errorMsg = "WSAStartup 失败，错误码: " + std::to_string(r);
                return;
            }
            ok_ = true;
        }

        ~WsaSession()
        {
            if (ok_)
            {
                ::WSACleanup();
            }
        }

        bool IsOk() const { return ok_; }

    private:
        bool ok_;
    };

    // --- RAII 封装 SOCKET ---
    class TcpSocket
    {
    public:
        TcpSocket()
            : s_(INVALID_SOCKET)
        {
        }

        ~TcpSocket()
        {
            if (s_ != INVALID_SOCKET)
            {
                ::closesocket(s_);
                s_ = INVALID_SOCKET;
            }
        }

        bool Connect(const std::string& host, int port, std::string& errorMsg)
        {
            struct addrinfo hints {};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            std::string portStr = std::to_string(port);

            struct addrinfo* result = nullptr;
            int r = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
            if (r != 0)
            {
                errorMsg = "getaddrinfo 失败，错误码: " + std::to_string(r);
                return false;
            }

            for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next)
            {
                s_ = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if (s_ == INVALID_SOCKET)
                {
                    continue;
                }

                if (::connect(s_, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == SOCKET_ERROR)
                {
                    ::closesocket(s_);
                    s_ = INVALID_SOCKET;
                    continue;
                }

                break; // 连接成功
            }

            ::freeaddrinfo(result);

            if (s_ == INVALID_SOCKET)
            {
                errorMsg = "无法连接到 SMTP 服务器。";
                return false;
            }

            return true;
        }

        SOCKET Get() const { return s_; }

    private:
        SOCKET s_;
    };

    // --- 发送所有数据 ---
    bool SendAll(SOCKET s, const std::string& data, std::string& errorMsg)
    {
        const char* buf = data.c_str();
        int         toSend = static_cast<int>(data.size());
        int         sentSum = 0;

        while (sentSum < toSend)
        {
            int n = ::send(s, buf + sentSum, toSend - sentSum, 0);
            if (n == SOCKET_ERROR)
            {
                int ec = ::WSAGetLastError();
                errorMsg = "发送数据失败，错误码: " + std::to_string(ec);
                return false;
            }
            if (n == 0)
            {
                errorMsg = "发送数据失败：对端关闭连接。";
                return false;
            }
            sentSum += n;
        }

        return true;
    }

    // --- 读取一行响应（直到 \r\n）并解析前三位数字为 code ---
    bool ReadResponse(SOCKET s, int& codeOut, std::string& textOut, std::string& errorMsg)
    {
        textOut.clear();
        codeOut = 0;

        char buf[512];

        while (true)
        {
            int n = ::recv(s, buf, sizeof(buf) - 1, 0);
            if (n == SOCKET_ERROR)
            {
                int ec = ::WSAGetLastError();
                errorMsg = "接收数据失败，错误码: " + std::to_string(ec);
                return false;
            }
            if (n == 0)
            {
                errorMsg = "接收数据失败：服务器关闭连接。";
                return false;
            }

            buf[n] = '\0';
            textOut += buf;

            auto pos = textOut.find("\r\n");
            if (pos != std::string::npos)
            {
                std::string line = textOut.substr(0, pos);
                if (line.size() >= 3 &&
                    std::isdigit(static_cast<unsigned char>(line[0])) &&
                    std::isdigit(static_cast<unsigned char>(line[1])) &&
                    std::isdigit(static_cast<unsigned char>(line[2])))
                {
                    codeOut = (line[0] - '0') * 100 +
                        (line[1] - '0') * 10 +
                        (line[2] - '0');
                }
                else
                {
                    codeOut = 0;
                }

                return true;
            }
        }
    }

    // --- 发送命令并期望某个响应类别（2xx / 3xx 等） ---
    bool SendCommandExpect(SOCKET s,
        const std::string& cmd,
        int expectClass,
        std::string& errorMsg)
    {
        std::string toSend = cmd + "\r\n";
        if (!SendAll(s, toSend, errorMsg))
            return false;

        int         code = 0;
        std::string resp;
        if (!ReadResponse(s, code, resp, errorMsg))
            return false;

        if (code / 100 != expectClass)
        {
            errorMsg = "SMTP 错误(" + std::to_string(code) + "): " + resp;
            return false;
        }

        return true;
    }

    // --- AUTH LOGIN 认证 ---
    bool AuthenticateLogin(SOCKET s, const SmtpConfig& cfg, std::string& errorMsg)
    {
        // AUTH LOGIN
        if (!SendCommandExpect(s, "AUTH LOGIN", 3, errorMsg))
        {
            return false;
        }

        // 用户名
        std::string user64 = Base64Encode(cfg.username);
        if (!SendCommandExpect(s, user64, 3, errorMsg))
        {
            return false;
        }

        // 密码
        std::string pass64 = Base64Encode(cfg.password);
        if (!SendCommandExpect(s, pass64, 2, errorMsg))
        {
            return false;
        }

        return true;
    }

    // --- 设置 socket 收发超时 ---
    bool SetSocketTimeouts(SOCKET s, int timeoutMs, std::string& errorMsg)
    {
        if (timeoutMs <= 0)
            return true;

        int tv = timeoutMs;

        if (::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&tv),
            static_cast<int>(sizeof(tv))) == SOCKET_ERROR)
        {
            errorMsg = "设置 socket 读超时时间失败。";
            return false;
        }

        if (::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&tv),
            static_cast<int>(sizeof(tv))) == SOCKET_ERROR)
        {
            errorMsg = "设置 socket 写超时时间失败。";
            return false;
        }

        return true;
    }

    // --- 从原始邮件中解析 To 地址 ---
    // 假定有一行形如：To: demo@test.com
    static std::string ExtractToAddress(const std::string& raw)
    {
        const std::string key = "\nTo:";
        auto pos = raw.find("To:");
        if (pos == std::string::npos)
            return {};

        pos += 3; // 跳过 "To:"
        // 跳过空格
        while (pos < raw.size() &&
            (raw[pos] == ' ' || raw[pos] == '\t'))
        {
            ++pos;
        }

        auto end = raw.find('\n', pos);
        std::string line =
            (end == std::string::npos)
            ? raw.substr(pos)
            : raw.substr(pos, end - pos);

        // 如果有 <...>，取中间这一段
        auto lt = line.find('<');
        auto gt = line.find('>');
        std::string addr;
        if (lt != std::string::npos &&
            gt != std::string::npos &&
            gt > lt + 1)
        {
            addr = line.substr(lt + 1, gt - lt - 1);
        }
        else
        {
            addr = line;
        }

        // 简单 trim
        while (!addr.empty() &&
            (addr.front() == ' ' ||
                addr.front() == '\t'))
        {
            addr.erase(addr.begin());
        }
        while (!addr.empty() &&
            (addr.back() == ' ' ||
                addr.back() == '\t' ||
                addr.back() == '\r'))
        {
            addr.pop_back();
        }

        return addr;
    }

    // --- 只发送一次的核心逻辑；重试由外层控制 ---
    bool SendMailOnce(const SmtpConfig& cfg,
        const std::string& rawMessage,
        std::string& errorMsg)
    {
        WsaSession wsa(errorMsg);
        if (!wsa.IsOk())
            return false;

        TcpSocket socket;
        if (!socket.Connect(cfg.serverIp, cfg.port, errorMsg))
            return false;

        // 设置读写超时
        if (!SetSocketTimeouts(socket.Get(), cfg.ioTimeoutMs, errorMsg))
            return false;

        int         code = 0;
        std::string resp;

        // 服务器欢迎语
        if (!ReadResponse(socket.Get(), code, resp, errorMsg))
            return false;
        if (code / 100 != 2)
        {
            errorMsg = "SMTP 服务器返回错误: " + resp;
            return false;
        }

        // EHLO
        if (!SendCommandExpect(socket.Get(), "EHLO localhost", 2, errorMsg))
            return false;

        // 如果启用认证，则先 AUTH LOGIN
        if (cfg.useAuth)
        {
            if (!AuthenticateLogin(socket.Get(), cfg, errorMsg))
            {
                return false;
            }
        }

        // MAIL FROM
        {
            std::ostringstream cmd;
            cmd << "MAIL FROM:<" << cfg.fromAddress << ">";
            if (!SendCommandExpect(socket.Get(), cmd.str(), 2, errorMsg))
                return false;
        }

        // RCPT TO
        std::string toAddr = ExtractToAddress(rawMessage);
        if (toAddr.empty())
        {
            errorMsg = "无法从邮件内容中解析收件人地址（To）。";
            return false;
        }
        {
            std::ostringstream cmd;
            cmd << "RCPT TO:<" << toAddr << ">";
            if (!SendCommandExpect(socket.Get(), cmd.str(), 2, errorMsg))
                return false;
        }

        // DATA
        if (!SendCommandExpect(socket.Get(), "DATA", 3, errorMsg))
            return false;

        // 发送正文，末尾确保有 "\r\n.\r\n"
        std::string dataToSend = rawMessage;
        if (dataToSend.size() < 5 ||
            dataToSend.substr(dataToSend.size() - 5) != "\r\n.\r\n")
        {
            dataToSend += "\r\n.\r\n";
        }

        if (!SendAll(socket.Get(), dataToSend, errorMsg))
            return false;

        if (!ReadResponse(socket.Get(), code, resp, errorMsg))
            return false;
        if (code / 100 != 2)
        {
            errorMsg = "发送邮件失败: " + resp;
            return false;
        }

        // QUIT（失败不算致命错误）
        std::string quitErr;
        SendCommandExpect(socket.Get(), "QUIT", 2, quitErr);

        return true;
    }
} // namespace

// --- 对外接口：带重试次数 ---
bool SmtpClient::SendMail(const SmtpConfig& cfg,
    const std::string& rawMessage,
    std::string& errorMsg)
{
    int maxAttempt = (cfg.maxRetry < 1) ? 1 : cfg.maxRetry;

    std::string lastError;
    for (int attempt = 1; attempt <= maxAttempt; ++attempt)
    {
        if (SendMailOnce(cfg, rawMessage, errorMsg))
        {
            return true;
        }

        lastError = errorMsg;
    }

    if (maxAttempt > 1)
    {
        errorMsg = "尝试发送邮件 " + std::to_string(maxAttempt) +
            " 次仍失败。最后一次错误: " + lastError;
    }
    else
    {
        errorMsg = lastError;
    }

    return false;
}
