# Qt Cloud Drive — 基于 C/S 架构的云存储系统

[![Qt](https://img.shields.io/badge/Qt-5.14.2-green)](https://www.qt.io/)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![MySQL](https://img.shields.io/badge/MySQL-5.7-orange)](https://www.mysql.com/)
[![DeepSeek](https://img.shields.io/badge/AI-DeepSeek-purple)](https://www.deepseek.com/)

独立设计的类网盘云存储系统，采用 Client-Server 架构，支持用户管理、好友社交、文件存储与分享，内置**四层文件安全扫描引擎**与 **AI 深度内容分析**。约 4000 行 C++，基于 TCP 自定义协议通信。

> **开发周期：** 2024.11 — 2024.12（基础功能），2026.07（安全检测 + AI 集成）

---

## 功能总览

| 模块 | 功能 |
|------|------|
| **用户系统** | 注册、登录、在线状态管理 |
| **好友系统** | 查找用户、添加好友、删除好友、实时聊天 |
| **文件管理** | 目录浏览、新建文件夹、重命名、删除、上传、下载 |
| **文件分享** | 分享文件给好友（服务端拷贝到好友目录，含权限校验） |
| **多线程上传** | QThread 异步分片上传，不阻塞 UI |
| **安全扫描** | 四层递进检测（文件名→扩展名→文件大小→魔数校验），覆盖上传/下载/分享/全盘扫描 |
| **AI 深度分析** | 接入 DeepSeek 大模型，检测身份证号、银行卡号、密钥泄露等敏感信息，高危文件自动处置 |
| **安全日志** | 线程安全日志模块，持久化记录所有安全拦截与 AI 处置操作 |

---

## 系统架构

系统分为客户端和服务端两部分，通过 TCP 长连接通信，使用自定义 PDU 二进制协议进行数据交换。

客户端负责用户界面和交互逻辑，包含登录注册、好友管理、文件管理和聊天四个核心模块。文件上传采用独立工作线程，将大文件按 4096 字节分片后通过信号槽机制异步发送。

服务端由 MyTcpServer 监听端口并接收连接，每个客户端连接分配一个 ClientTask（基于 QThreadPool 的 QRunnable 实现线程池管理，最大 8 线程）。MyTcpSocket 负责消息收发与粘包拆包，MsgHandler 处理具体业务逻辑，OperateDB 通过参数化查询操作 MySQL 数据库，FileScanner 提供四层文件安全扫描，AIChecker 通过 WinHTTP 异步调用 DeepSeek API 进行内容深度分析。

---

## 安全检测子系统

### 四层递进扫描（L1-L4）

| 层级 | 检测内容 | 实现方式 |
|------|---------|---------|
| L1 | 文件名合法性 | 检测路径遍历（`..`、`/`、`\`）、空字节注入、超长文件名 |
| L2 | 扩展名黑名单 | 17 种危险扩展名（.exe/.bat/.sh/.dll/.vbs 等），文件打开前拦截 |
| L3 | 文件大小限制 | 基于 `QFileInfo::size()`，阈值 10GB，上传完成后校验 |
| L4 | 魔数校验 | 读取文件头 12 字节，与 17 种格式魔数表匹配，识别 PE/ELF 伪装 |

拦截点覆盖文件上传、下载、分享及全盘扫描四个操作路径。

### AI 深度内容分析

- 接入 DeepSeek API（OpenAI 兼容接口，`deepseek-chat` 模型）
- 基于 WinHTTP + SChannel 实现 HTTPS，无需外部 OpenSSL DLL
- `QtConcurrent::run` + `QFutureWatcher` 异步架构，不阻塞线程池
- 扫描文本文件中的身份证号、银行卡号、手机号、密码、API 密钥、Token 等敏感信息
- AI 标记的高危文件自动删除并写入安全日志

### 安全日志

单例 + `QMutex` 线程安全设计，`[时间戳] [级别] 消息` 格式持久化到 `logs/security.log`，覆盖 L1-L4 拦截、上传/下载/分享拦截、AI 文件删除等全部安全操作。

---

## 通信协议（PDU）

自定义二进制协议，固定头部 + 可变长度消息体：

```cpp
struct PDU {
    uint uiTotalLen;    // 消息总长度
    uint uiMsgLen;      // 消息体长度
    uint uiType;        // 消息类型（30+ 种）
    char caData[64];    // 固定参数区
    char caMsg[];       // 可变消息体
};
```

支持 30+ 种消息类型：注册/登录、好友增删查聊、文件增删改查、上传/下载/分享、安全扫描、AI 分析 等。

---

## 核心设计

### 粘包/拆包处理

TCP 流式传输中消息可能被拆分或合并。通过 QByteArray 缓冲区 + PDU 头部长度字段，循环解析完整消息：

```cpp
while (buffer.size() >= sizeof(PDU)) {
    PDU* pdu = (PDU*)buffer.data();
    if (buffer.size() < pdu->uiTotalLen) break;  // 半包等待
    handleMsg(pdu);                                // 完整包处理
    buffer.remove(0, pdu->uiTotalLen);             // 消费
}
```

### 多线程文件上传

采用 `QThread + moveToThread` 模式，将文件读取与网络发送解耦：

- 主线程：发送上传初始化请求 → 创建 Uploader 工作线程
- Uploader 线程：按 4096 字节分片读取文件 → 逐片发送
- 完成后自动清理：`finished` 信号 → `deleteLater`

### SQL 注入防护

全部数据库操作使用参数化查询（`QSqlQuery::prepare()` + `addBindValue()`），消除字符串拼接带来的注入风险。

### 全链路 UTF-8 编码

客户端统一使用 `toUtf8()` 编码发送，服务端使用 `QString::fromUtf8()` 解码接收，彻底解决中文文件名、用户名、聊天消息的乱码问题。

### 单例模式

Client、Server、OperateDB、AIChecker、Logger 均采用单例模式，确保全局唯一的核心组件实例。

---

## 构建 & 运行

```bash
# 环境：Qt 5.14.2 + MinGW 7.3.0 64-bit + MySQL 5.7

# Server
cd Server && qmake Server.pro && mingw32-make
# 将 libmysql.dll 拷贝到 Server/debug/
./debug/Server.exe

# Client
cd Client && qmake Client.pro && mingw32-make
./release/Client.exe
```

---

## 技术栈

**语言:** C++17 · **框架:** Qt 5.14 · **数据库:** MySQL 5.7  
**通信:** TCP 自定义二进制协议（PDU）· **并发:** QThreadPool + QtConcurrent  
**安全:** 四层文件扫描 + AI 语义分析 + SQL 参数化查询 + 安全日志  
**设计模式:** 单例模式、观察者模式（信号槽）

---

## 项目结构

```
qt-cloud-drive/
├── Client/
│   ├── main.cpp              # 程序入口，加载全局样式与图标
│   ├── client.cpp/h          # TCP 连接管理、登录/注册窗口
│   ├── index.cpp/h           # 主界面（侧边栏 + QStackedWidget）
│   ├── file.cpp/h            # 文件管理（浏览、增删改、上传下载、安全检查）
│   ├── friend.cpp/h          # 好友管理（列表、删除、聊天入口）
│   ├── chat.cpp/h            # 实时聊天窗口
│   ├── onlineuser.cpp/h      # 在线用户列表（双击添加好友）
│   ├── uploader.cpp/h        # 多线程分片上传（QThread + 4096 字节分片）
│   ├── reshandler.cpp/h      # 服务端响应分发（20+ 种消息类型）
│   ├── protocol.cpp/h        # PDU 协议定义（结构体 + 消息枚举）
│   ├── style.qss             # 全局 QSS 主题样式表
│   ├── lsb.qrc               # 资源文件（配置 + 样式）
│   └── icon.qrc              # 资源文件（图标）
│
├── Server/
│   ├── main.cpp              # 程序入口（初始化数据库 + 启动 TCP 监听）
│   ├── server.cpp/h          # 配置加载、AI Key 初始化
│   ├── mytcpserver.cpp/h     # TCP 服务器（QTcpServer + 线程池 8 线程）
│   ├── mytcpsocket.cpp/h     # 单连接消息收发与粘包拆包
│   ├── clienttask.cpp/h      # 线程池 QRunnable 任务封装
│   ├── msghandler.cpp/h      # 全部消息类型业务逻辑（20+ 种）
│   ├── operatedb.cpp/h       # MySQL 数据库操作（参数化查询防注入）
│   ├── filescanner.cpp/h     # 四层文件安全扫描（L1-L4）
│   ├── aichecker.cpp/h       # AI 深度分析（DeepSeek API + 异步 HTTP）
│   ├── winhttpclient.cpp/h   # WinHTTP HTTPS 封装（SChannel TLS）
│   ├── logger.cpp/h          # 线程安全日志模块（单例 + QMutex）
│   ├── protocol.cpp/h        # 协议定义（与客户端一致）
│   ├── config.qrc            # 资源文件（嵌入 connect.config）
│   ├── connect.config        # 服务端配置（IP/端口/存储路径/API Key）
│   └── Server.pro            # Qt 项目文件
│
├── show/                     # 实验性 Qt 窗口 Demo
└── README.md
```

---

## License

MIT
