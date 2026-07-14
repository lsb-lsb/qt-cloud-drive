#include "aichecker.h"
#include "winhttpclient.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

AIChecker::AIChecker(QObject* parent)
    : QObject(parent)
{
    m_apiEndpoint = "https://api.deepseek.com/v1/chat/completions";

    m_systemContentReview =
        "You are a security content reviewer. You will receive file content from one or more "
        "files. Each file's content is separated by a \"--- FILE: <filepath> ---\" marker.\n"
        "Analyze the content for sensitive information leaks. Specifically check for:\n"
        "1. Personal ID numbers (Chinese ID card format: 18 digits matching birth date pattern)\n"
        "2. Bank card numbers (16-19 digit sequences)\n"
        "3. Phone numbers (Chinese mobile: 1[3-9]xxxxxxxxx)\n"
        "4. Passwords, API keys, access tokens (key=value patterns)\n"
        "5. Email addresses and physical addresses\n"
        "Respond in Chinese. Format your response as a list, one finding per line:\n"
        "  [严重程度] 文件: <filepath> — <发现的问题> — 建议: <处理建议>\n"
        "Severity levels: 高/中/低.\n"
        "IMPORTANT: Always mention the exact file path from the \"--- FILE:\" marker for each finding.\n"
        "If nothing found, just say '未发现敏感信息'.";

    m_systemReport =
        "You are a cloud storage security analyst. Given scan results from a file safety "
        "scanner, generate a concise security report in Chinese. Include:\n"
        "1. Overall risk assessment (safe / caution / dangerous)\n"
        "2. Summary of issues found (by risk level)\n"
        "3. Recommended actions for each high-risk finding\n"
        "4. Best practice suggestions for cloud storage security\n"
        "Keep the report under 500 words. Use clear bullet points.";

    m_systemThreat =
        "You are a malware analyst. Explain the given file threat detection result "
        "in plain Chinese. Explain:\n"
        "1. What this type of threat means (e.g., 'this is a PE executable disguised as a JPEG image')\n"
        "2. What harm it could cause\n"
        "3. What the user should do about it\n"
        "Keep explanation under 200 words, practical and actionable.";
}

AIChecker::~AIChecker()
{
    for (auto* w : m_watchers) {
        w->cancel();
        w->deleteLater();
    }
    m_watchers.clear();
}

AIChecker& AIChecker::getInstance()
{
    static AIChecker instance;
    return instance;
}

void AIChecker::setApiKey(const QString& key)
{
    m_apiKey = key;
    qDebug() << "AI Checker: API key configured";
}

void AIChecker::setApiEndpoint(const QString& url)
{
    if (!url.isEmpty()) {
        m_apiEndpoint = url;
    }
    qDebug() << "AI Checker: endpoint = " << m_apiEndpoint;
}

bool AIChecker::isConfigured()
{
    return !m_apiKey.isEmpty();
}

void AIChecker::parseAIResponse(const QString& rawResponse,
                                 std::function<void(const QString&)> callback)
{
    if (!callback) return;

    if (rawResponse.startsWith("ERROR:")) {
        QString err = "API request failed: " + rawResponse.mid(7);
        qDebug() << err;
        callback(err);
        return;
    }

    QByteArray responseData = rawResponse.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    // DeepSeek/OpenAI error format
    if (obj.contains("error")) {
        QJsonObject errObj = obj["error"].toObject();
        QString err = "API error: " + errObj["message"].toString();
        qDebug() << err;
        callback(err);
        return;
    }

    // DeepSeek/OpenAI response: choices[0].message.content
    if (obj.contains("choices")) {
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            QJsonObject message = choice["message"].toObject();
            QString text = message["content"].toString();
            callback(text);
            return;
        }
    }

    callback("Unexpected API response format");
}

// 调用DeepSeek API
void AIChecker::callAPI(const QString& systemPrompt, const QString& userMessage,
                        std::function<void(const QString&)> callback)
{
    if (m_apiKey.isEmpty()) {
        if (callback) callback("Error: API key not configured");
        return;
    }

    QJsonObject requestBody;
    requestBody["model"] = "deepseek-chat";
    requestBody["max_tokens"] = 1024;

    QJsonArray messagesArr;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = systemPrompt;
    messagesArr.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messagesArr.append(userMsg);

    requestBody["messages"] = messagesArr;

    QJsonDocument doc(requestBody);
    QByteArray data = doc.toJson();

    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + m_apiKey;

    QString endpoint = m_apiEndpoint;

    qDebug() << "AI API request ->" << m_apiEndpoint;

    // Create watcher on heap, cleaned up when finished lambda fires
    auto* watcher = new QFutureWatcher<QString>(this);
    m_watchers.append(watcher);

    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, callback]() {
        QString result = watcher->result();
        m_watchers.removeAll(watcher);
        parseAIResponse(result, callback);
        watcher->deleteLater();
    });

    // Run WinHTTP call on thread pool (no OpenSSL dependency)
    QFuture<QString> future = QtConcurrent::run(
        winHttpPost, endpoint, headers, data);
    watcher->setFuture(future);
}

// AI内容安全分析
void AIChecker::analyzeContent(const QString& filePath, const QString& fileContent,
                               std::function<void(const QString&)> callback)
{
    QString prompt = QString("File path: %1\n\nContent:\n%2").arg(filePath, fileContent);

    if (prompt.length() > 8000) {
        prompt = prompt.left(8000) + "\n\n[Content truncated - showing first 8000 characters]";
    }

    callAPI(m_systemContentReview, prompt, callback);
}

// AI生成安全报告
void AIChecker::generateReport(const QString& scanResults, int totalFiles, int unsafeCount,
                               std::function<void(const QString&)> callback)
{
    QString prompt = QString("Scan summary: %1 total files, %2 unsafe files found.\n\n"
                             "Detailed findings:\n%3")
                         .arg(totalFiles).arg(unsafeCount).arg(scanResults);

    callAPI(m_systemReport, prompt, callback);
}

// AI解释安全威胁
void AIChecker::explainThreat(const QString& filePath, const QString& threatReason,
                              std::function<void(const QString&)> callback)
{
    QString prompt = QString("File: %1\nDetection result: %2\nPlease explain this threat.")
                         .arg(filePath, threatReason);

    callAPI(m_systemThreat, prompt, callback);
}
