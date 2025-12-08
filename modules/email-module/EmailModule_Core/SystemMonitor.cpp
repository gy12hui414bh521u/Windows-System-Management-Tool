#include "SystemMonitor.h"
#include "SmtpClient.h"
#include "EmailMessage.h"
#include "Logger.h"
#include "Utils.h"

// 基础定义
#define NOMINMAX 
#include <windows.h> 

// 只需要标准库，不再需要 tlhelp32
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib> // 用于 system() 函数

// === 1. 基础硬件监控 ===

double SystemMonitor::GetDiskFreeSpaceGB(const std::string& drive)
{
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFree;
    if (GetDiskFreeSpaceExA(drive.c_str(), &freeBytesAvailable, &totalBytes, &totalFree))
    {
        return static_cast<double>(freeBytesAvailable.QuadPart) / (1024.0 * 1024.0 * 1024.0);
    }
    return -1.0;
}

int SystemMonitor::GetMemoryUsagePercent()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo))
    {
        return static_cast<int>(memInfo.dwMemoryLoad);
    }
    return 0;
}

// === 2. CPU 计算 (只依赖基础 API) ===

static unsigned long long FileTimeToInt64(const FILETIME& ft)
{
    return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

int SystemMonitor::GetCpuUsagePercent()
{
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return 0;

    unsigned long long idle1 = FileTimeToInt64(idleTime);
    unsigned long long kernel1 = FileTimeToInt64(kernelTime);
    unsigned long long user1 = FileTimeToInt64(userTime);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return 0;

    unsigned long long idle2 = FileTimeToInt64(idleTime);
    unsigned long long kernel2 = FileTimeToInt64(kernelTime);
    unsigned long long user2 = FileTimeToInt64(userTime);

    unsigned long long idleDelta = idle2 - idle1;
    unsigned long long kernelDelta = kernel2 - kernel1;
    unsigned long long userDelta = user2 - user1;
    unsigned long long totalSystem = kernelDelta + userDelta;

    if (totalSystem == 0) return 0;
    return (int)((totalSystem - idleDelta) * 100 / totalSystem);
}

// === 3. 进程监控 (命令黑客版 - 完美绕过 SDK 问题) ===

bool SystemMonitor::IsProcessRunning(const std::string& processName)
{
    // 使用 Windows 自带的 tasklist 命令来查找进程
    // 这不需要任何特殊的头文件，只要你的电脑是 Windows 就能跑！

    // 构造命令: tasklist | find /I "notepad.exe" > nul
    std::string cmd = "tasklist | find /I \"" + processName + "\" > nul";

    // system() 执行命令
    // 如果找到了，返回 0
    // 如果没找到，返回 1
    int result = system(cmd.c_str());

    // 注意：system 的返回值在不同环境可能不同，但在 Windows cmd 下，
    // 成功找到通常返回 0
    return (result == 0);
}

// === 4. 报警逻辑 ===

bool SystemMonitor::SendAlertEmail(const SmtpConfig& cfg, const std::string& to, const std::string& subject, const std::string& content, std::string& errorMsg)
{
    SimpleEmail mail;
    mail.to = to;
    mail.subject = subject;
    mail.body = content;

    std::string raw = EmailMessageBuilder::Build(cfg, mail);
    SmtpClient client;

    if (!client.SendMail(cfg, raw, errorMsg))
    {
        EmailLogger::Error("报警邮件发送失败: " + errorMsg);
        return false;
    }

    EmailLogger::Info("已触发报警，邮件发送至: " + to);
    return true;
}

bool SystemMonitor::CheckAndAlert(
    const SmtpConfig& cfg,
    const std::string& adminEmail,
    double thresholdDiskGB,
    int thresholdMemPercent,
    int cpuThresholdPercent,
    const std::string& processName,
    std::string& errorMsg)
{
    std::string report;
    bool needAlert = false;
    int abnormalCount = 0;

    report += "=== 服务器资源监控报告 ===\n\n";

    // 1. CPU
    int cpuUsage = GetCpuUsagePercent();
    if (cpuUsage > cpuThresholdPercent)
    {
        needAlert = true; abnormalCount++;
        report += "⚠️ [CPU 过载] 当前: " + std::to_string(cpuUsage) + "%\n";
    }
    else
    {
        report += "✅ [CPU] 正常: " + std::to_string(cpuUsage) + "%\n";
    }

    // 2. 内存
    int memUsage = GetMemoryUsagePercent();
    if (memUsage > thresholdMemPercent)
    {
        needAlert = true; abnormalCount++;
        report += "⚠️ [内存 不足] 当前: " + std::to_string(memUsage) + "%\n";
    }
    else
    {
        report += "✅ [内存] 正常: " + std::to_string(memUsage) + "%\n";
    }

    // 3. 磁盘
    char buffer[256];
    if (GetLogicalDriveStringsA(sizeof(buffer), buffer) > 0)
    {
        char* drive = buffer;
        while (*drive)
        {
            std::string driveStr = drive;
            double freeGB = GetDiskFreeSpaceGB(driveStr);
            if (freeGB >= 0 && freeGB < thresholdDiskGB)
            {
                needAlert = true; abnormalCount++;
                report += "⚠️ [磁盘] " + driveStr + " 空间不足: " + std::to_string(freeGB) + " GB\n";
            }
            drive += strlen(drive) + 1;
        }
    }

    // 4. 进程检查 (命令行版)
    if (!processName.empty())
    {
        if (!IsProcessRunning(processName))
        {
            needAlert = true; abnormalCount++;
            report += "🛑 [严重错误] 关键服务未运行: " + processName + "\n";
        }
        else
        {
            report += "✅ [进程] 服务运行中: " + processName + "\n";
        }
    }

    std::cout << "\n--------------------------------\n";
    std::cout << report;
    std::cout << "--------------------------------\n";

    if (needAlert)
    {
        std::cout << ">>> 🚨 监控到异常，正在发送报警邮件...\n";
        std::string subject = "【监控警报】系统异常 - " + GetCurrentTimeString();
        return SendAlertEmail(cfg, adminEmail, subject, report, errorMsg);
    }
    else
    {
        std::cout << ">>> 系统各项指标正常。\n";
        return true;
    }
}