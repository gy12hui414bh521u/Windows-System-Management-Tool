EmailModule 集成说明
=====================

交付文件：
- EmailModule_Dll.dll
- EmailModule_Dll.lib
- EmailModuleExports.h
- email.conf.example
- recipients.txt (示例)
- mail_template.txt (示例)
- 简单演示代码（可选）

集成步骤（组长侧）：
1. 将 EmailModule_Dll.dll 复制到主程序 EXE 同目录（或系统 PATH 中可见位置）。
2. 如果采用静态链接方式：
   - 在主程序工程中把 EmailModule_Dll.lib 添加到 链接器 -> 附加依赖项，
   - 把 EmailModuleExports.h 放到 include 路径，然后在代码中直接调用导出函数。
3. 如果采用动态加载方式（LoadLibrary/GetProcAddress），则只需要 DLL 文件和头文件中函数签名作为参考。
4. 编辑 email.conf（工作目录下）配置 SMTP/IMAP，如果演示使用本地 smtp4dev，请使用示例配置（127.0.0.1 / 2525）。
5. 推荐 errorBuf 大小 >= 512 字节，用于接收详细错误信息。

运行/演示建议：
- 本地演示：使用 smtp4dev（稳定、不上网）。配置 email.conf 指向 smtp4dev；主程序调用 Email_SendSimple 或 Email_SendBulk → 在 smtp4dev 界面即时看到邮件被捕获。
- 若要对接真实邮箱：需确保 SMTP/IMAP 支持 TLS 并配置认证（App Password / OAuth2），此项超出本演示范围。

注意事项：
- DLL（x64/x86）位数必须与主程序一致。
- errorBuf 可传 nullptr 表示不接收错误文本。
- 所有字符串均以 UTF-8 编码传入/传出。
