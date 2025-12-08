// EmailModuleAuto.cpp
// Wrapper: 提供两种对外入口：
//  1) EmailModule_Invoke(const char* requestJson, char* responseBuf, int responseBufSize)
//  2) RunStdioLoop()
// 以及 description()
// 设计：优先使用 requestJson 内的数据；否则从 DLL 同目录或当前工作目录读取标准文件（email.conf, recipients.txt, mail_template.txt）；最后使用内置默认。
#include "EmailModuleApi.h"
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <cstring>
#include <sstream>
#include <windows.h>
#include <direct.h>


// simple mutex for thread-safety
static std::mutex g_invoke_mutex;

// ===================== 内置默认配置（可以按需修改） =====================
static const char* default_email_conf =
"server_ip=127.0.0.1\n"
"port=2525\n"
"from_address=test@local\n"
"use_auth=0\n"
"io_timeout_ms=5000\n"
"max_retry=1\n";

static const char* default_recipients_txt =
"alice@example.com,Alice\n"
"bob@example.com,Bob\n";

static const char* default_mail_template_txt =
"Subject: 测试邮件给 ${name}\r\n"
"\r\n"
"亲爱的 ${name},\r\n"
"\r\n"
"这是测试邮件（第 ${index} 封）。\r\n"
"\r\n"
"此致，\r\n测试团队\r\n";
// =======================================================================

// Helper: get current directory
static std::string GetCurrentDir()
{
    char buf[MAX_PATH];
    DWORD n = GetCurrentDirectoryA(MAX_PATH, buf);
    if (n == 0 || n > MAX_PATH) return ".";
    return std::string(buf);
}

// Helper: set current directory
static bool SetCurrentDir(const std::string& d)
{
    return SetCurrentDirectoryA(d.c_str()) != 0;
}

// Create a temp directory and write default files, return path in outPath
static bool CreateTempConfigDirAndWriteDefaults(std::string& outPath, std::string& err)
{
    err.clear();
    char tmpPath[MAX_PATH];
    // use GetTempPath + unique name
    DWORD l = GetTempPathA(MAX_PATH, tmpPath);
    if (l == 0 || l > MAX_PATH) { err = "GetTempPath failed"; return false; }

    // create unique subdir
    std::ostringstream name;
    name << "emailmod_" << GetCurrentProcessId() << "_" << rand() % 100000;
    std::string full = std::string(tmpPath) + name.str();
    if (!CreateDirectoryA(full.c_str(), NULL))
    {
        // if exists, allow
        DWORD ec = GetLastError();
        if (ec != ERROR_ALREADY_EXISTS) { err = "CreateDirectory failed"; return false; }
    }

    // write files
    auto writeFile = [&](const std::string& fname, const char* content)->bool {
        std::string path = full + "\\" + fname;
        FILE* f = nullptr;
        errno_t e = fopen_s(&f, path.c_str(), "wb");
        if (e || !f) return false;
        fwrite(content, 1, strlen(content), f);
        fclose(f);
        return true;
        };

    if (!writeFile("email.conf", default_email_conf)) { err = "write email.conf failed"; return false; }
    if (!writeFile("recipients.txt", default_recipients_txt)) { err = "write recipients.txt failed"; return false; }
    if (!writeFile("mail_template.txt", default_mail_template_txt)) { err = "write mail_template.txt failed"; return false; }

    outPath = full;
    return true;
}

// Cleanup optional: remove the temp dir (only try to delete files we wrote)
static void CleanupTempDir(const std::string& dir)
{
    // best-effort: delete files then remove directory
    std::string f;
    f = dir + "\\email.conf"; DeleteFileA(f.c_str());
    f = dir + "\\recipients.txt"; DeleteFileA(f.c_str());
    f = dir + "\\mail_template.txt"; DeleteFileA(f.c_str());
    RemoveDirectoryA(dir.c_str());
}

// RAII-style helper: switch cwd to temp dir for duration and restore on exit
class TempCwdGuard {
public:
    TempCwdGuard(const std::string& newCwd, bool ownDir) : own(ownDir), dir(newCwd)
    {
        old = GetCurrentDir();
        SetCurrentDir(newCwd);
    }
    ~TempCwdGuard()
    {
        SetCurrentDir(old);
        if (own) CleanupTempDir(dir);
    }
private:
    std::string old;
    std::string dir;
    bool own;
};

// split helper for RunStdioLoop
static std::vector<std::string> splitPipe(const std::string& s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        size_t j = s.find('|', i);
        if (j == std::string::npos) j = s.size();
        out.push_back(s.substr(i, j - i));
        i = j + 1;
    }
    return out;
}

// minimal JSON write result (no dependency)
//static void WriteJsonResult(bool ok, const std::string& msg, char* outBuf, int outSize) {
//    if (!outBuf || outSize <= 0) return;
//    std::string j = std::string("{\"ok\":") + (ok ? "true" : "false") + ",\"msg\":\"";
//    for (char c : msg) {
//        if (c == '\\' || c == '"') { j.push_back('\\'); j.push_back(c); }
//        else if (c == '\n') { j += "\\n"; }
//        else j.push_back(c);
//    }
//    j += "\"}";
//#if defined(_MSC_VER)
//    strncpy_s(outBuf, static_cast<size_t>(outSize), j.c_str(), _TRUNCATE);
//#else
//    std::strncpy(outBuf, j.c_str(), static_cast<size_t>(outSize - 1));
//    outBuf[outSize - 1] = '\0';
//#endif
//}

//// very small JSON extraction helper (searches "key":"value" - not full parser)
//static std::string findJsonValueSimple(const std::string& json, const std::string& key) {
//    std::string pat = "\"" + key + "\"";
//    auto p = json.find(pat);
//    if (p == std::string::npos) return "";
//    auto colon = json.find(':', p + pat.size());
//    if (colon == std::string::npos) return "";
//    auto start = json.find_first_of('\"', colon);
//    if (start == std::string::npos) return "";
//    auto end = json.find_first_of('\"', start + 1);
//    if (end == std::string::npos) return "";
//    return json.substr(start + 1, end - start - 1);
//}
//
//// Exported: JSON single entry point
//extern "C" __declspec(dllexport) int __stdcall EmailModule_Invoke(
//    const char* requestJson, char* responseBuf, int responseBufSize)
//{
//    std::lock_guard<std::mutex> lk(g_invoke_mutex);
//
//    if (!responseBuf || responseBufSize <= 0) return 100;
//
//    if (!requestJson) {
//        WriteJsonResult(false, "requestJson null", responseBuf, responseBufSize);
//        return 1;
//    }
//
//    std::string req(requestJson);
//    std::string cmd = findJsonValueSimple(req, "cmd");
//    if (cmd.empty()) {
//        WriteJsonResult(false, "no cmd", responseBuf, responseBufSize);
//        return 2;
//    }
//
//    if (cmd == "send_simple") {
//        std::string to = findJsonValueSimple(req, "to");
//        std::string subject = findJsonValueSimple(req, "subject");
//        std::string body = findJsonValueSimple(req, "body");
//        char err[512] = { 0 };
//        bool ok = Email_SendSimple(to.c_str(), subject.c_str(), body.c_str(), err, sizeof(err));
//        if (ok) { WriteJsonResult(true, "sent", responseBuf, responseBufSize); return 0; }
//        else { WriteJsonResult(false, std::string("send failed: ") + err, responseBuf, responseBufSize); return 3; }
//    }
//    else if (cmd == "send_bulk") {
//        char err[512] = { 0 };
//        bool ok = Email_SendBulk(err, sizeof(err));
//        if (ok) { WriteJsonResult(true, "bulk_sent", responseBuf, responseBufSize); return 0; }
//        else { WriteJsonResult(false, std::string("bulk failed: ") + err, responseBuf, responseBufSize); return 4; }
//    }
//    // inside EmailModule_Invoke when handling "send_bulk"
//    else if (cmd == "send_bulk") {
//        // check if JSON provided recipients/template/config; if not, try filesystem; if still not, use embedded defaults by creating temp dir
//        bool needTemp = false;
//        // (for brevity assume we checked and found nothing - here we always use temp if no explicit fields)
//        std::string tmpDir, tmperr;
//        if (!CreateTempConfigDirAndWriteDefaults(tmpDir, tmperr)) {
//            WriteJsonResult(false, std::string("cannot create default config: ") + tmperr, responseBuf, responseBufSize);
//            return 10;
//        }
//        TempCwdGuard guard(tmpDir, true); // will restore and cleanup on exit of scope
//
//        char err[1024] = { 0 };
//        bool ok = Email_SendBulk(err, sizeof(err));
//        if (ok) { WriteJsonResult(true, "bulk_sent", responseBuf, responseBufSize); return 0; }
//        else { WriteJsonResult(false, std::string("bulk failed: ") + err, responseBuf, responseBufSize); return 4; }
//    }
//
//    else if (cmd == "search_log") {
//        std::string kw = findJsonValueSimple(req, "keyword");
//        std::string st = findJsonValueSimple(req, "start");
//        std::string ed = findJsonValueSimple(req, "end");
//        char err[1024] = { 0 };
//        bool ok = Email_SearchLog(kw.c_str(), st.c_str(), ed.c_str(), err, sizeof(err));
//        if (ok) { WriteJsonResult(true, "search_done", responseBuf, responseBufSize); return 0; }
//        else { WriteJsonResult(false, std::string("search failed: ") + err, responseBuf, responseBufSize); return 6; }
//    }
//    else {
//        WriteJsonResult(false, "unknown cmd", responseBuf, responseBufSize);
//        return 7;
//    }
//}

// Exported: stdio loop (line-based).
extern "C" {

__declspec(dllexport) const char* description()
{
        return "Email Module";
}

__declspec(dllexport) int run()
{   
    // [新增] 必须加上这 3 行！否则你的 DLL 找不到配置文件！
        // (这是你原版 RunStdioLoop 里没有的，但必须补上)
    std::string err, tmpPath;
    if (!CreateTempConfigDirAndWriteDefaults(tmpPath, err)) {
        std::cout << "ERR|InitFailed|" << err << std::endl; return -1;
    }
    TempCwdGuard guard(tmpPath, true);

    std::lock_guard<std::mutex> lk(g_invoke_mutex);

    std::string line;
    std::cout << "EMAIL_MODULE_STDIO_READY\n" << std::flush;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        auto parts = splitPipe(line);
        std::string cmd = parts.size() ? parts[0] : "";
        if (cmd == "QUIT" || cmd == "EXIT") {
            std::cout << "OK|BYE\n" << std::flush;
            break;
        }
        else if (cmd == "SEND_SIMPLE") {
            if (parts.size() < 4) {
                std::cout << "ERR|Need 3 args\n" << std::flush;
                continue;
            }
            const char* to = parts[1].c_str();
            const char* subject = parts[2].c_str();
            const char* body = parts[3].c_str();

            std::string errStr;
            bool ok = EmailModule::SendSimpleMail(to, subject, body, errStr);
            if (ok) std::cout << "OK|SENT\n";
            else std::cout << "ERR|" << err << "\n";
            std::cout << std::flush;
        }
        else if (cmd == "BULK_SEND") {
            std::string errStr;
            bool ok = EmailModule::SendBulkMails(errStr);
            if (ok) std::cout << "OK|BULK_DONE\n";
            else std::cout << "ERR|" << err << "\n";
            std::cout << std::flush;
        }
        else if (cmd == "SHOW_STATS") {
            std::string errStr;
            bool ok = EmailModule::ShowStatistics(errStr);
            if (ok) std::cout << "OK|STATS_SHOWN\n";
            else std::cout << "ERR|" << err << "\n";
            std::cout << std::flush;
        }
        else if (cmd == "SEARCH_LOG") {
            const char* kw = parts.size() > 1 ? parts[1].c_str() : "";
            const char* st = parts.size() > 2 ? parts[2].c_str() : "";
            const char* ed = parts.size() > 3 ? parts[3].c_str() : "";
            std::string errStr;
            bool ok = EmailModule::SearchLog(kw, st, ed, errStr);
            if (ok) std::cout << "OK|SEARCH_DONE\n";
            else std::cout << "ERR|" << err << "\n";
            std::cout << std::flush;
        }
        // --- 新增：检查邮件 ---
        else if (cmd == "CHECK_MAIL") {
            std::string info;
            std::string errStr;
            // 调用我们刚才写的 API
            if (EmailModule::CheckInbox(info, errStr)) {
                // 打印成功信息，info 里包含了 "+OK xxxx"
                std::cout << "OK|" << info << std::endl;
            }
            else {
                std::cout << "ERR|" << errStr << std::endl;
            }
        }
        else {
            std::cout << "ERR|UnknownCmd\n" << std::flush;
        }
    }

    return 0;
}
}
