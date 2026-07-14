#ifndef WINHTTPCLIENT_H
#define WINHTTPCLIENT_H

#include <QString>
#include <QByteArray>
#include <QMap>

// 同步 HTTPS POST，使用 WinHTTP（Windows SChannel TLS，不依赖 OpenSSL）
// 参数:
//   url     — 完整的 HTTPS URL (e.g. https://api.anthropic.com/v1/messages)
//   headers — HTTP 请求头 (Key -> Value)
//   body    — POST 请求体
// 返回值:
//   成功时返回响应体 (UTF-8 字符串)
//   失败时返回 "ERROR:" 开头的错误信息
QString winHttpPost(const QString& url,
                    const QMap<QString, QString>& headers,
                    const QByteArray& body);

#endif // WINHTTPCLIENT_H
