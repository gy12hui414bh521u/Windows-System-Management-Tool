#pragma once
#include <string>
#include "Config.h" 

class SystemMonitor
{
public:
    // v3.0 全能监控
    // - diskThresholdGB: 磁盘报警阈值
    // - memThresholdPercent: 内存报警阈值
    // - cpuThresholdPercent: CPU报警阈值 (新增!)
    // - processName: 要守护的关键进程名 (比如 "notepad.exe", 为空则不检查) (新增!)
    static bool CheckAndAlert(
        const SmtpConfig& cfg,
        const std::string& adminEmail,
        double thresholdDiskGB,
        int thresholdMemPercent,
        int cpuThresholdPercent,     // 新增
        const std::string& processName, // 新增
        std::string& errorMsg
    );

private:
    static double GetDiskFreeSpaceGB(const std::string& drive);
    static int GetMemoryUsagePercent();

    // 获取 CPU 使用率 (需要采样，会阻塞一小会儿)
    static int GetCpuUsagePercent();

    // 检查进程是否存在
    static bool IsProcessRunning(const std::string& processName);

    static bool SendAlertEmail(
        const SmtpConfig& cfg,
        const std::string& to,
        const std::string& subject,
        const std::string& content,
        std::string& errorMsg
    );
};