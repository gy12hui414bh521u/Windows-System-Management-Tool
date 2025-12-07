#include "pch.h" 
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

// ==========================================
//  工具类：Base64 加密/解密
// ==========================================
class Base64 {
    static const string base64_chars;
public:
    static string Encode(const string& in) {
        string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(base64_chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    static string Decode(const string& in) {
        string out;
        vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;
        int val = 0, valb = -8;
        for (unsigned char c : in) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }
};
const string Base64::base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// ==========================================
//  核心类：文件系统管理器
// ==========================================
class MyFileSystem {
private:
    string rootPath;
    long long diskQuota;
    long long currentUsed;
    const string LOG_FILE = "system_log.txt";

    string getCurrentTime() {
        auto t = time(nullptr);
        struct tm tm;
        localtime_s(&tm, &t);
        ostringstream oss;
        oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void logAction(string action, string details) {
        ofstream log(fs::path(rootPath) / LOG_FILE, ios::app);
        if (log.is_open()) {
            log << "[" << getCurrentTime() << "] [" << action << "] " << details << endl;
            log.close();
        }
    }

    long long calculateSize(fs::path p) {
        long long size = 0;
        if (!fs::exists(p)) return 0;
        for (const auto& entry : fs::recursive_directory_iterator(p)) {
            if (entry.is_regular_file() && entry.path().filename() != LOG_FILE) {
                size += entry.file_size();
            }
        }
        return size;
    }

    void printTreeRecursive(fs::path p, string indent) {
        if (!fs::exists(p)) return;
        for (const auto& entry : fs::directory_iterator(p)) {
            if (entry.path().filename() == LOG_FILE) continue;
            if (entry.is_directory()) {
                cout << indent << "|-- [" << entry.path().filename().string() << "]" << endl;
                printTreeRecursive(entry.path(), indent + "    ");
            }
            else {
                cout << indent << "|-- " << entry.path().filename().string()
                    << " (" << entry.file_size() << " B)" << endl;
            }
        }
    }

public:
    MyFileSystem(string path, long long quota) {
        rootPath = path;
        diskQuota = quota;
        if (!fs::exists(rootPath)) fs::create_directories(rootPath);
        currentUsed = calculateSize(rootPath);
        logAction("SYSTEM", "FileSystem Initialized");
    }

    string CreateFile(string name, string content) {
        long long newSize = content.length();
        if (currentUsed + newSize > diskQuota) {
            string err = "Quota exceeded! Limit: " + to_string(diskQuota) + ", Remaining: " + to_string(diskQuota - currentUsed);
            logAction("ERROR", err);
            return "Error: " + err;
        }
        fs::path p = fs::path(rootPath) / name;
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        ofstream outfile(p);
        outfile << content;
        outfile.close();
        currentUsed += newSize;
        logAction("CREATE", name);
        return "Success: Created " + name;
    }

    string DeleteFile(string name) {
        fs::path p = fs::path(rootPath) / name;
        if (fs::exists(p) && fs::is_regular_file(p)) {
            long long size = fs::file_size(p);
            fs::remove(p);
            currentUsed -= size;
            logAction("DELETE", name);
            return "Success: Deleted " + name;
        }
        return "Error: File not found.";
    }

    string CopyFile(string srcName, string destName) {
        fs::path src = fs::path(rootPath) / srcName;
        fs::path dest = fs::path(rootPath) / destName;
        if (!fs::exists(src)) return "Error: Source not found.";
        if (fs::exists(dest)) return "Error: Dest exists.";
        long long fileSize = fs::file_size(src);
        if (currentUsed + fileSize > diskQuota) return "Error: Quota exceeded.";
        try {
            fs::copy_file(src, dest);
            currentUsed += fileSize;
            logAction("COPY", srcName + " -> " + destName);
            return "Success: Copied.";
        }
        catch (exception& e) { return string("Error: ") + e.what(); }
    }

    string MoveFile(string srcName, string destName) {
        fs::path src = fs::path(rootPath) / srcName;
        fs::path dest = fs::path(rootPath) / destName;
        if (!fs::exists(src)) return "Error: Source not found.";
        try {
            fs::rename(src, dest);
            logAction("MOVE", srcName + " -> " + destName);
            return "Success: Moved.";
        }
        catch (exception& e) { return string("Error: ") + e.what(); }
    }

    void Grep(string keyword, string fileName) {
        fs::path p = fs::path(rootPath) / fileName;
        if (!fs::exists(p)) { cout << "Error: File not found." << endl; return; }
        ifstream file(p);
        string line;
        int lineNum = 1;
        bool found = false;
        cout << "--- Grep [" << keyword << "] in " << fileName << " ---" << endl;
        while (getline(file, line)) {
            if (line.find(keyword) != string::npos) {
                cout << lineNum << ": " << line << endl;
                found = true;
            }
            lineNum++;
        }
        if (!found) cout << "(No matches)" << endl;
        cout << "-----------------------------------" << endl;
    }

    string EncryptFile(string name) {
        fs::path p = fs::path(rootPath) / name;
        if (!fs::exists(p)) return "Error: File not found.";
        ifstream in(p, ios::binary);
        string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        in.close();
        if (content.rfind("[ENCRYPTED]", 0) == 0) return "Error: Already encrypted.";
        string encoded = "[ENCRYPTED]" + Base64::Encode(content);
        ofstream out(p, ios::trunc);
        out << encoded;
        out.close();
        logAction("ENCRYPT", name);
        return "Success: Encrypted.";
    }

    string DecryptFile(string name) {
        fs::path p = fs::path(rootPath) / name;
        if (!fs::exists(p)) return "Error: File not found.";
        ifstream in(p);
        string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        in.close();
        if (content.rfind("[ENCRYPTED]", 0) != 0) return "Error: Not encrypted.";
        string decoded = Base64::Decode(content.substr(11));
        ofstream out(p, ios::trunc | ios::binary);
        out << decoded;
        out.close();
        logAction("DECRYPT", name);
        return "Success: Decrypted.";
    }

    void PrintTree() {
        cout << "Root: [" << fs::path(rootPath).filename().string() << "]" << endl;
        printTreeRecursive(rootPath, "");
    }

    string GetStatus() {
        double percent = diskQuota > 0 ? (double)currentUsed / diskQuota * 100.0 : 0;
        stringstream ss;
        ss << fixed << setprecision(1) << percent;
        return "Used: " + to_string(currentUsed) + " / " + to_string(diskQuota) + " (" + ss.str() + "%)";
    }
};

// ==========================================
//  DLL 导出接口 
// ==========================================
extern "C" {
    __declspec(dllexport) const char* description() {
        // 更新描述
        return "FileSystem_Run(stdin/stdout)";
    }

    // 这里原来的 int main() 已经被改成 int run() 了
    __declspec(dllexport) int run() {
        MyFileSystem fs("C:\\Temp\\MyVirtualDiskCpp", 5000);

        cout << "=== Windows FileSystem Automation ===" << endl;
        cout << "Ready. Commands: create, del, cp, mv, grep, encrypt, decrypt, tree, status, exit" << endl;

        string cmd;
        while (cin >> cmd) {
            if (cmd == "exit") break;
            if (cmd == "tree") fs.PrintTree();
            else if (cmd == "status") cout << fs.GetStatus() << endl;
            else if (cmd == "create") { string n, c; cin >> n >> c; cout << fs.CreateFile(n, c) << endl; }
            else if (cmd == "del") { string n; cin >> n; cout << fs.DeleteFile(n) << endl; }
            else if (cmd == "cp") { string s, d; cin >> s >> d; cout << fs.CopyFile(s, d) << endl; }
            else if (cmd == "mv") { string s, d; cin >> s >> d; cout << fs.MoveFile(s, d) << endl; }
            else if (cmd == "grep") { string k, n; cin >> k >> n; fs.Grep(k, n); }
            else if (cmd == "encrypt") { string n; cin >> n; cout << fs.EncryptFile(n) << endl; }
            else if (cmd == "decrypt") { string n; cin >> n; cout << fs.DecryptFile(n) << endl; }
            else { cout << "Unknown command." << endl; cin.clear(); string d; getline(cin, d); }
        }
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}