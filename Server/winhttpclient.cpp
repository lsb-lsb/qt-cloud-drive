#include "winhttpclient.h"

#include <windows.h>
#include <winhttp.h>
#include <QUrl>
#include <QDebug>
#include <vector>

QString winHttpPost(const QString& url,
                    const QMap<QString, QString>& headers,
                    const QByteArray& body)
{
    QUrl qurl(url);
    if (!qurl.isValid()) {
        return QString("ERROR: Invalid URL: %1").arg(url);
    }

    QString host = qurl.host();
    int port = qurl.port(qurl.scheme() == "https" ? 443 : 80);
    QString path = qurl.path();
    if (path.isEmpty()) path = "/";
    if (qurl.hasQuery()) path += "?" + qurl.query();

    std::wstring wHost = host.toStdWString();
    std::wstring wPath = path.toStdWString();

    HINTERNET hSession = WinHttpOpen(
        L"QtCloudDrive/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) {
        DWORD err = GetLastError();
        return QString("ERROR: WinHttpOpen failed (code: %1)").arg(err);
    }

    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), port, 0);
    if (!hConnect) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hSession);
        return QString("ERROR: WinHttpConnect to %1:%2 failed (code: %3)")
            .arg(host).arg(port).arg(err);
    }

    DWORD flags = (qurl.scheme() == "https") ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        wPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);
    if (!hRequest) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return QString("ERROR: WinHttpOpenRequest failed (code: %1)").arg(err);
    }

    WinHttpSetTimeouts(hRequest, 30000, 60000, 60000, 60000);

    std::wstring wHeaders;
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        wHeaders += it.key().toStdWString() + L": " + it.value().toStdWString() + L"\r\n";
    }

    BOOL result = WinHttpSendRequest(
        hRequest,
        wHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wHeaders.c_str(),
        wHeaders.empty() ? 0 : -1L,
        body.isEmpty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.constData(),
        body.size(),
        body.size(),
        0);
    if (!result) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return QString("ERROR: WinHttpSendRequest failed (code: %1)").arg(err);
    }

    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return QString("ERROR: WinHttpReceiveResponse failed (code: %1)").arg(err);
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &statusCode,
                        &statusCodeSize,
                        WINHTTP_NO_HEADER_INDEX);

    std::string responseStr;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable + 1);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            responseStr.append(buffer.data(), bytesRead);
        } else {
            break;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (statusCode >= 400) {
        return QString("ERROR: HTTP %1: %2")
            .arg(statusCode)
            .arg(QString::fromUtf8(responseStr.c_str(), responseStr.size()));
    }

    return QString::fromUtf8(responseStr.c_str(), responseStr.size());
}
