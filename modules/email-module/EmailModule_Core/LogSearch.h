#pragma once
#include <string>

// 在 email.log 中搜索包含指定关键字的日志行，并可选按时间范围过滤。
// keyword      : 搜索关键字（子串匹配，英文不区分大小写）。
// startTimeStr : 起始时间字符串（可选），格式建议：
//                - "YYYY-MM-DD"
//                - 或 "YYYY-MM-DD HH:MM:SS"
// endTimeStr   : 结束时间字符串（可选），格式同上。
// errorMsg     : 出错时返回错误信息（如无法打开日志文件）。
//
// 返回值：
//   true  = 搜索过程正常完成（即使没有匹配行）
//   false = 出错（如日志文件不存在）
bool SearchLogByKeyword(const std::string& keyword,
    const std::string& startTimeStr,
    const std::string& endTimeStr,
    std::string& errorMsg);

