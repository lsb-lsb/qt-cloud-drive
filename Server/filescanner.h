#ifndef FILESCANNER_H
#define FILESCANNER_H

#include <QString>
#include <QStringList>
#include <QList>

struct ScanResult {
    QString strFilePath;
    int iRiskLevel;
    QString strReason;
};

class FileScanner
{
public:
    FileScanner() = delete;

    static QString scanFile(const QString& strFilePath);
    static QList<ScanResult> scanDirectory(const QString& strRootPath);

    static bool isFileNameSafe(const QString& strName);
    static QString checkFileName(const QString& strName);

    static bool isExtensionSafe(const QString& strPath);
    static QString checkExtension(const QString& strPath);

    static bool isFileSizeSafe(const QString& strPath, qint64 iMaxSize = 10LL * 1024 * 1024 * 1024);
    static QString checkFileSize(const QString& strPath, qint64 iMaxSize = 10LL * 1024 * 1024 * 1024);

    static bool isMagicNumberMatch(const QString& strPath);
    static QString checkMagicNumber(const QString& strPath);

private:
    static QList<QByteArray> getExpectedMagic(const QString& strExt);
    static const QStringList s_dangerousExtensions;
};

#endif
