#ifndef AICHECKER_H
#define AICHECKER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <functional>

template<typename T> class QFutureWatcher;

class AIChecker : public QObject
{
    Q_OBJECT

public:
    static AIChecker& getInstance();
    ~AIChecker();

    void setApiKey(const QString& key);
    void setApiEndpoint(const QString& url);
    bool isConfigured();

    void analyzeContent(const QString& filePath, const QString& fileContent,
                        std::function<void(const QString&)> callback);
    void generateReport(const QString& scanResults, int totalFiles, int unsafeCount,
                        std::function<void(const QString&)> callback);
    void explainThreat(const QString& filePath, const QString& threatReason,
                       std::function<void(const QString&)> callback);

signals:
    void analysisReady(const QString& filePath, const QString& result);
    void reportReady(const QString& report);
    void explanationReady(const QString& explanation);
    void apiError(const QString& errorMsg);

private:
    AIChecker(QObject* parent = nullptr);
    AIChecker(const AIChecker&) = delete;
    AIChecker& operator=(const AIChecker&) = delete;

    void callAPI(const QString& systemPrompt, const QString& userMessage,
                 std::function<void(const QString&)> callback);
    void parseAIResponse(const QString& rawResponse,
                         std::function<void(const QString&)> callback);

    QString m_apiKey;
    QString m_apiEndpoint;

    QString m_systemContentReview;
    QString m_systemReport;
    QString m_systemThreat;

    // Track in-flight watchers so they can be cancelled on shutdown
    QList<QFutureWatcher<QString>*> m_watchers;
};

#endif
