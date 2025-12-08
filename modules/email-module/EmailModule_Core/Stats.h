#pragma once
#include <string>

// 从 email.log 中统计发送情况，并打印到控制台。
// errorMsg：出错时返回简要说明（例如无法打开日志文件）。
//
// 返回值：
//   true  = 统计成功（日志存在并成功解析）
//   false = 出错（比如日志不存在），调用方可以根据 errorMsg 给用户提示。
bool ShowEmailStatistics(std::string& errorMsg);

