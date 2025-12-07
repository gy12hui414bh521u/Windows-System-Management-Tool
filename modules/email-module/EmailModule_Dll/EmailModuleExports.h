#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // 发送一封简单文本邮件（不带附件）
    // 参数：
    //   to/subject/body : UTF-8 C 字符串
    //   errorBuf        : 调用方提供的错误缓冲区（可为 nullptr）
    //   errorBufSize    : 缓冲区长度
    // 返回：true 成功，false 失败（失败时 errorBuf 被写入错误信息，若 errorBuf 为 nullptr 则不返回错误文本）
    bool Email_SendSimple(const char* to,
        const char* subject,
        const char* body,
        char* errorBuf,
        int errorBufSize);

    // 按项目约定读取 recipients.txt 与 mail_template.txt 并群发
    bool Email_SendBulk(char* errorBuf, int errorBufSize);

    // 在控制台打印/输出统计（成功/失败数量等）
    // 返回 false 表示统计过程中出错（如没有日志）
    bool Email_ShowStats(char* errorBuf, int errorBufSize);

    // 按关键字 + 可选时间范围搜索 email.log 并在控制台打印结果
    // startTime / endTime 可以为空字符串
    bool Email_SearchLog(const char* keyword,
        const char* startTime,
        const char* endTime,
        char* errorBuf,
        int errorBufSize);

#ifdef __cplusplus
}
#endif

