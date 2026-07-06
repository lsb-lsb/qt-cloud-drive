# Qt Cloud Drive — 基于 C/S 架构的云存储系统

[![Qt](https://img.shields.io/badge/Qt-5.14.2-green)](https://www.qt.io/)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![MySQL](https://img.shields.io/badge/MySQL-5.7-orange)](https://www.mysql.com/)

独立设计的类网盘云存储系统，采用 Client-Server 架构，支持用户管理、好友社交、文件存储与分享。~3000 行 C++，基于 TCP 自定义协议通信。

> **开发周期：** 2024.11 — 2024.12

---

## 功能总览

| 模块 | 功能 |
|------|------|
| **用户系统** | 注册、登录、在线状态管理 |
| **好友系统** | 查找用户、添加好友、删除好友、实时聊天 |
| **文件管理** | 目录浏览、新建文件夹、重命名、删除、上传、下载 |
| **文件分享** | 一键分享文件给好友（服务端拷贝到好友目录） |
| **多线程上传** | QThread 异步分片上传，不阻塞 UI |

---

## 系统架构

```
┌─────────────────────┐        ┌─────────────────────┐
│     Client (Qt)      │  TCP   │     Server (Qt)      │
│                      │◄──────►│                      │
│  ┌───────────────┐   │  PDU   │  ┌───────────────┐   │
│  │  Login/Regist  │   │ 协议   │  │  MyTcpServer   │   │
│  │  Friend Page   │   │       │  │  ClientTask     │   │
│  │  File Page     │   │       │  │  MsgHandler     │   │
│  │  Chat Window   │   │       │  │  OperateDB      │───┼─── MySQL
│  │  Upload Thread │   │       │  │  File System    │   │
│  └───────────────┘   │        │  └───────────────┘   │
└─────────────────────┘        └─────────────────────┘
```

---

## 通信协议（PDU）

自定义二进制协议，固定头部 + 可变长度消息体：

```cpp
struct PDU {
    uint uiTotalLen;    // 消息总长度
    uint uiMsgLen;      // 消息体长度
    uint uiType;        // 消息类型（20+ 种）
    char caData[64];    // 固定参数区
    char caMsg[];       // 可变消息体
};
```

支持 20+ 种消息类型：注册/登录、好友增删查聊、文件增删改查、上传/下载/分享 等。

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

### 单例模式管理核心组件

Client、Server、OperateDB 均采用单例模式，确保全局唯一的 TCP 连接、数据库连接和服务端实例，避免状态不一致。

---

## 构建 & 运行

```bash
# 环境：Qt 5.14.2 + MinGW 7.3.0 64-bit + MySQL 5.7

# Server
cd Server && qmake Server.pro && mingw32-make
./release/Server.exe

# Client
cd Client && qmake Client.pro && mingw32-make
./release/Client.exe
```

---

## 技术栈

**语言:** C++17 · **框架:** Qt 5.14 · **数据库:** MySQL 5.7  
**通信:** TCP 自定义二进制协议（PDU）· **并发:** QThread + moveToThread  
**设计模式:** 单例模式、观察者模式（信号槽）

---

## 项目结构

```
qtprojects/
├── Client/                 # 客户端
│   ├── client.cpp/h        # 主窗口 + TCP 连接管理
│   ├── index.cpp/h         # 首页（好友/文件页切换）
│   ├── file.cpp/h          # 文件管理页
│   ├── friend.cpp/h        # 好友管理页
│   ├── chat.cpp/h          # 聊天窗口
│   ├── onlineuser.cpp/h    # 在线用户列表
│   ├── uploader.cpp/h      # 多线程文件上传器
│   ├── reshandler.cpp/h    # 服务端响应处理器
│   └── protocol.cpp/h      # 通信协议定义
├── Server/                 # 服务端
│   ├── server.cpp/h        # 服务端入口
│   ├── mytcpserver.cpp/h   # TCP 服务器（线程池）
│   ├── mytcpsocket.cpp/h   # 单连接消息处理
│   ├── msghandler.cpp/h    # 消息业务逻辑
│   ├── operatedb.cpp/h     # MySQL 数据库操作
│   └── protocol.cpp/h      # 通信协议定义
└── show/                   # 实验性窗口 demo
```

---

## License

MIT
