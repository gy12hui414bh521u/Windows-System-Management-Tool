# EmailModule_Core – C++ 邮件管理模块

## 1. 项目简介

本模块是软件工程课程期末大作业的一部分，负责实现**邮件管理子系统**。  
组长搭建整体框架后，其他同学以“模块”为单位进行开发，本模块采用 **C++17 + WinSock + 控制台界面** 实现：

- 从配置文件读取 SMTP 参数；
- 发送单封测试邮件；
- 从收件人列表文件中**批量群发邮件**（支持模板变量）；
- 将发送过程写入日志，并提供**统计功能**；
- 使用本地测试 SMTP 服务器 `smtp4dev` 进行联调。

---

## 2. 已完成功能（按我们之前的层级）

### Level 1：核心必做

1. **配置管理（M1）**
   - 配置文件：`email.conf`
   - 内容示例：
     ```ini
     server_ip=127.0.0.1
     port=2525
     from_address=test@example.com
     ```
   - 启动时由 `ConfigLoader::LoadSmtpConfig` 负责读取和校验。

2. **日志系统（M2）**
   - 日志文件：`email.log`
   - 类：`EmailLogger`（`Info` / `Error`）
   - 记录内容：时间、级别、简要说明（例如发送成功 / 失败原因、群发统计等）。

3. **控制台菜单（M3）**
   - 入口：`RunMenu()`（`Menu.cpp`），`main` 只负责调用它。
   - 菜单项：
     - `1` 发送单封测试邮件
     - `2` 从 `recipients.txt` 群发测试邮件
     - `3` 查看发送统计信息
     - `0` 退出

4. **邮件构造（M4）**
   - 结构体：`SimpleEmail { to, subject, body }`
   - 类：`EmailMessageBuilder::Build`  
     负责生成带头部的文本邮件（From/To/Subject/Date/MIME-Version/Content-Type 等），
     使用 UTF-8 编码，支持中文主题（Subject 使用 `=?utf-8?B?...?=` 形式）。

5. **SMTP 客户端（M6）**
   - 类：`SmtpClient::SendMail`  
   - 使用 WinSock（RAII 封装 `WsaSession`、`TcpSocket`），实现基本 SMTP 流程：
     `HELO` → `MAIL FROM` → `RCPT TO` → `DATA` → `QUIT`  
   - 不依赖第三方库，只与本地 `smtp4dev` 对接。
   - 发送前对换行进行规范化，尽量满足 RFC 5321 的 CRLF 要求。

---

### Level 2：扩展功能

6. **模板引擎（M8）**
   - 文件：`TemplateEngine.h / .cpp`
   - 函数：`RenderTemplate(tpl, vars)`  
   - 支持 `${name}`、`${index}`、`${time}` 形式的占位符替换，不存在的 key 保留原样。

7. **群发 / 批量发送（M10）**
   - 收件人列表文件：`recipients.txt`
     ```text
     # 每行：邮箱,名字
     demo1@test.com,小明
     demo2@test.com,小红
     demo3@test.com,小刚
     ```
   - 模板文件：`mail_template.txt`
     ```text
     你好，${name}！
     这是第 ${index} 封来自 C++ 邮件模块的测试邮件。
     当前时间：${time}
     ```
   - 模块：`BulkSender::SendBulkMails`  
     读取 `recipients.txt` 和 `mail_template.txt`，循环调用 `SmtpClient`，日志中记录每一封邮件的成功/失败。

8. **统计功能（M_Stats）**
   - 模块：`ShowEmailStatistics`（`Stats.cpp`）
   - 从 `email.log` 中解析：
     - 总发送次数
     - 成功次数 / 失败次数
     - 单封成功/失败次数
     - 群发成功/失败次数
     - 最后一次发送时间
   - 以表格形式输出到控制台。

---

## 3. 模块划分与依赖关系

- `Utils`  
  - 时间获取、Base64 编码、UTF-8 头部编码、换行规范化等基础工具。
- `Logger`  
  - 依赖 `Utils`，负责写 `email.log`。
- `Config`  
  - 只依赖 STL，负责读取 `email.conf`。
- `EmailMessage`  
  - 依赖 `Utils` 和 `Config`，负责构造完整邮件字符串。
- `SmtpClient`  
  - 依赖 `Config`、`Logger`、`Utils`，封装 WinSock SMTP 发送逻辑。
- `TemplateEngine`  
  - 只依赖 STL，提供简单占位符替换。
- `BulkSender`  
  - 依赖 `TemplateEngine`、`EmailMessage`、`SmtpClient`、`Logger`、`Utils`。
- `Stats`  
  - 只依赖 STL，分析 `email.log` 并打印统计。
- `Menu`  
  - 依赖上述所有模块，负责交互逻辑。
- `EmailModule_Core.cpp`  
  - 只包含 `main`，调用 `InitConsoleUtf8()` 和 `RunMenu()`。

---

## 4. 运行环境与使用方法

### 4.1 编译环境

- IDE：Visual Studio（C++ 桌面开发工作负载）
- 语言级别：C++17
- 字符集：**/utf-8**（源文件与可执行字符集均为 UTF-8）
- 依赖库：WinSock2（VS 默认自带）

### 4.2 配置本地 SMTP 测试服务器

- 工具：`smtp4dev` Desktop 版
- 监听端口：建议设置为 `2525`
- 在 `email.conf` 中配置：
  ```ini
  server_ip=127.0.0.1
  port=2525
  from_address=test@example.com
  ```
### 4.3 运行步骤

- 启动 smtp4dev，确认正在监听端口（例如 “SMTP server listening on port 2525”）。

- 确认项目目录下存在：
```
email.conf

recipients.txt

mail_template.txt
```
- 在 VS 中生成并运行（Ctrl + F5）。

- 在菜单中选择：

 - 1：发送单封测试邮件（控制台输入收件人、主题、正文）。

 - 2：按 recipients.txt + mail_template.txt 群发。

 - 3：根据 email.log 查看统计信息。

- 打开发送工具窗口查看邮件内容。

### 5. 软件工程与实现细节说明（可作为报告的一部分）

- 内存安全：

  - 全程使用 STL 容器和栈对象；不使用 new/delete。

  - WinSock 资源通过 RAII 封装类（WsaSession、TcpSocket）管理。

- 编码与国际化：

  - 项目统一使用 UTF-8 编码；

  - 控制台启动时调用 SetConsoleOutputCP(CP_UTF8) / SetConsoleCP(CP_UTF8)；

  - 邮件 Subject 采用 RFC 2047 的 UTF-8 Base64 编码，支持中英文混排。

- 模块化与可扩展性：

  - 每个功能单元独立成模块（配置、日志、SMTP、模板、群发、统计）；

  - 菜单与业务逻辑解耦，后续可以替换为图形界面或集成到其他系统。

- 可测试性：

  - 使用 smtp4dev 作为本地测试服务器，不依赖真实邮件服务；

  - 关键行为都写入 email.log，方便回放和统计。

- 错误处理：

  - 所有可能出错的路径（文件 IO、网络连接等）均返回 bool + errorMsg；

  - 同时写入日志，方便排查问题。

### 6. 后续可选扩展（如果时间允许）

- 支持带附件的 MIME 邮件；

- 支持 SMTP 认证（AUTH LOGIN）；

- 支持接收邮件（POP3/IMAP 简单客户端）；

- 更复杂的过滤器（按关键字统计、筛选日志等）；

- 将当前模块封装成 DLL，提供统一导出函数给组长集成。