#include "filescanner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMap>

// L1: 文件名安全检测
bool FileScanner::isFileNameSafe(const QString& strName)
{
    if (strName.contains("..") || strName.contains('/') || strName.contains('\\')) {
        return false;
    }
    if (strName.contains('\0')) {
        return false;
    }
    if (strName.size() > 255) {
        return false;
    }
    return true;
}

QString FileScanner::checkFileName(const QString& strName)
{
    if (strName.contains("..") || strName.contains('/') || strName.contains('\\')) {
        return QString("文件名包含非法路径字符: %1").arg(strName);
    }
    if (strName.contains('\0')) {
        return QString("文件名包含空字节: %1").arg(strName);
    }
    if (strName.size() > 255) {
        return QString("文件名过长(%1字符): %2").arg(strName.size()).arg(strName.left(50));
    }
    return QString();
}

const QStringList FileScanner::s_dangerousExtensions = {
    ".exe", ".bat", ".sh",   ".msi", ".dll", ".scr",
    ".vbs", ".ps1", ".cmd",  ".com", ".pif", ".reg",
    ".hta", ".js",  ".vbe",  ".wsf", ".wsh", ".jar"
};

// L2: 扩展名黑名单检测
bool FileScanner::isExtensionSafe(const QString& strPath)
{
    QString strLower = strPath.toLower();
    for (const QString& ext : s_dangerousExtensions) {
        if (strLower.endsWith(ext)) {
            return false;
        }
    }
    return true;
}

QString FileScanner::checkExtension(const QString& strPath)
{
    QString strLower = strPath.toLower();
    for (const QString& ext : s_dangerousExtensions) {
        if (strLower.endsWith(ext)) {
            return QString("危险文件扩展名: %1 (%2)").arg(strPath).arg(ext);
        }
    }
    return QString();
}

// L3: 文件大小检测
bool FileScanner::isFileSizeSafe(const QString& strPath, qint64 iMaxSize)
{
    QFileInfo info(strPath);
    if (info.exists() && info.isFile()) {
        return info.size() <= iMaxSize;
    }
    return true;
}

QString FileScanner::checkFileSize(const QString& strPath, qint64 iMaxSize)
{
    QFileInfo info(strPath);
    if (info.exists() && info.isFile()) {
        qint64 iSize = info.size();
        if (iSize > iMaxSize) {
            qint64 iSizeMB = iSize / (1024 * 1024);
            qint64 iMaxMB = iMaxSize / (1024 * 1024);
            return QString("文件过大: %1 (%2 MB, 上限 %3 MB)")
                .arg(strPath).arg(iSizeMB).arg(iMaxMB);
        }
    }
    return QString();
}

QList<QByteArray> FileScanner::getExpectedMagic(const QString& strExt)
{
    typedef QMap<QString, QList<QByteArray>> MagicMap;
    static MagicMap magicMap;
    if (magicMap.isEmpty()) {
        magicMap[".jpg"]  = {QByteArray::fromHex("FFD8FF")};
        magicMap[".jpeg"] = {QByteArray::fromHex("FFD8FF")};
        magicMap[".png"]  = {QByteArray::fromHex("89504E47")};
        magicMap[".gif"]  = {QByteArray::fromHex("47494638")};
        magicMap[".bmp"]  = {QByteArray::fromHex("424D")};
        magicMap[".webp"] = {QByteArray::fromHex("52494646")};
        magicMap[".pdf"] = {QByteArray::fromHex("25504446")};
        magicMap[".zip"] = {QByteArray::fromHex("504B0304"),
                            QByteArray::fromHex("504B0506"),
                            QByteArray::fromHex("504B0708")};
        magicMap[".rar"] = {QByteArray::fromHex("526172211A0700"),
                            QByteArray::fromHex("526172211A070100")};
        magicMap[".7z"]  = {QByteArray::fromHex("377ABCAF271C")};
        magicMap[".gz"]  = {QByteArray::fromHex("1F8B")};
        magicMap[".mp3"] = {QByteArray::fromHex("FFFB"),
                            QByteArray::fromHex("FFF3"),
                            QByteArray::fromHex("494433")};
        magicMap[".mp4"] = {QByteArray::fromHex("0000001866747970"),
                            QByteArray::fromHex("0000002066747970")};
        magicMap[".avi"] = {QByteArray::fromHex("52494646")};
        magicMap[".flv"] = {QByteArray::fromHex("464C5601")};
    }
    return magicMap.value(strExt.toLower());
}

// L4: 文件头魔数校验
bool FileScanner::isMagicNumberMatch(const QString& strPath)
{
    QFileInfo info(strPath);
    QString strExt = "." + info.suffix().toLower();
    if (strExt == ".") return true;

    QList<QByteArray> expectedMagics = getExpectedMagic(strExt);
    if (expectedMagics.isEmpty()) return true;

    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly)) return true;

    QByteArray header = file.read(12);
    file.close();

    if (header.isEmpty()) return true;

    for (const QByteArray& magic : expectedMagics) {
        if (header.startsWith(magic)) {
            return true;
        }
    }
    return false;
}

QString FileScanner::checkMagicNumber(const QString& strPath)
{
    QFileInfo info(strPath);
    QString strExt = "." + info.suffix().toLower();
    if (strExt == ".") return QString();

    QList<QByteArray> expectedMagics = getExpectedMagic(strExt);
    if (expectedMagics.isEmpty()) return QString();

    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QByteArray header = file.read(12);
    file.close();

    if (header.isEmpty()) return QString();

    for (const QByteArray& magic : expectedMagics) {
        if (header.startsWith(magic)) {
            return QString();
        }
    }
    if (header.startsWith("MZ")) {
        return QString("文件伪装: %1 扩展名为 %2 但实际是 Windows PE 可执行文件")
            .arg(strPath).arg(strExt);
    }
    if (header.startsWith("\x7F""ELF")) {
        return QString("文件伪装: %1 扩展名为 %2 但实际是 Linux ELF 可执行文件")
            .arg(strPath).arg(strExt);
    }
    return QString("文件伪装: %1 扩展名为 %2 但文件头不匹配")
        .arg(strPath).arg(strExt);
}

// 扫描单个文件(L1-L4)
QString FileScanner::scanFile(const QString& strFilePath)
{
    QFileInfo info(strFilePath);
    if (!info.exists() || !info.isFile()) {
        return QString();
    }

    QString nameResult = checkFileName(info.fileName());
    if (!nameResult.isEmpty()) return nameResult;

    QString extResult = checkExtension(strFilePath);
    if (!extResult.isEmpty()) return extResult;

    QString sizeResult = checkFileSize(strFilePath);
    if (!sizeResult.isEmpty()) return sizeResult;

    QString magicResult = checkMagicNumber(strFilePath);
    if (!magicResult.isEmpty()) return magicResult;

    return QString();
}

// 递归扫描目录
QList<ScanResult> FileScanner::scanDirectory(const QString& strRootPath)
{
    QList<ScanResult> results;

    QDir dir(strRootPath);
    if (!dir.exists()) return results;

    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        QString strFullPath = entry.absoluteFilePath();
        if (entry.isDir()) {
            results.append(scanDirectory(strFullPath));
        } else if (entry.isFile()) {
            QString risk = scanFile(strFullPath);
            if (!risk.isEmpty()) {
                ScanResult result;
                result.strFilePath = strFullPath;
                result.strReason = risk;

                if (risk.contains("伪装") || risk.contains("PE") || risk.contains("ELF")) {
                    result.iRiskLevel = 3;
                } else if (risk.contains("危险") || risk.contains("非法")) {
                    result.iRiskLevel = 2;
                } else {
                    result.iRiskLevel = 1;
                }
                results.append(result);
            }
        }
    }
    return results;
}
