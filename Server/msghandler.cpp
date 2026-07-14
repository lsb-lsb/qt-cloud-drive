#include "msghandler.h"
#include "aichecker.h"
#include "filescanner.h"
#include "mytcpserver.h"
#include "operatedb.h"
#include "server.h"
#include "stdlib.h"
#include "string.h"
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
MsgHandler::MsgHandler()
{

}

// 注册
PDU *MsgHandler::regist()
{
    char caName[32]={'\0'};
    memcpy(caName,pdu->caData,32);
    char caPwd[32]={'\0'};
    memcpy(caPwd,pdu->caData+32,32);
    qDebug()<<"regist caName"<<caName<<"caPwd"<<caPwd;
    bool ret=OperateDB::getInstance().handleRegist(caName,caPwd);
    qDebug()<<"regist ret"<<ret;
    if(ret){
         QDir dir;
         bool res=dir.mkdir(QString("%1/%2").arg(Server::getInstance().m_strRootPath).arg(caName));
         qDebug()<<"创建用户文件夹 res"<<res;
    }
    PDU* respdu=mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiType=ENUM_TYPE_REGIST_RESPOND;
    return respdu;
}

// 登录
PDU *MsgHandler::login(QString &strLoginName)
{
    char caName[32]={'\0'};
    memcpy(caName,pdu->caData,32);
    char caPwd[32]={'\0'};
    memcpy(caPwd,pdu->caData+32,32);
    qDebug()<<"login caName"<<caName<<"caPwd"<<caPwd;
    bool ret=OperateDB::getInstance().handleLogin(caName,caPwd);
    qDebug()<<"login ret"<<ret;
    if(ret){
    strLoginName=caName;
           }
    PDU* respdu=mkPDU();
    memcpy(respdu->caData,&ret,sizeof(bool));
    respdu->uiType=ENUM_TYPE_LOGIN_RESPOND;
    return respdu;
}

// 查找用户
PDU *MsgHandler::findUser()
{
    char caName[32]={'\0'};
    memcpy(caName,pdu->caData,32);
    qDebug()<<"find user caName"<<caName;
    int ret=OperateDB::getInstance().handleFindUser(caName);
    qDebug()<<"find user ret"<<ret;
    PDU* respdu=mkPDU();
    memcpy(respdu->caData,&ret,sizeof(int));
    respdu->uiType=ENUM_TYPE_FIND_USER_RESPOND;
    return respdu;
}

// 在线用户
PDU *MsgHandler::onlineUser()
{
    QStringList res=OperateDB::getInstance().handleOnlineUser();
    PDU* respdu=mkPDU(res.size()*32);
    respdu->uiType=ENUM_TYPE_ONLINE_USER_RESPOND;
    for(int i=0;i<res.size();i++){
        memcpy(respdu->caMsg+i*32,res[i].toUtf8().constData(),32);
    }
    return respdu;
}

// 添加好友
PDU *MsgHandler::addFriend()
{
    char caCurName[32]={'\0'};
    char caTarName[32]={'\0'};
    memcpy(caCurName,pdu->caData,32);
    memcpy(caTarName,pdu->caData+32,32);
    int ret=OperateDB::getInstance().handleAddFriend(caCurName,caTarName);
    qDebug()<<"addFriend ret"<<ret;
    if(ret==1){
        pdu->uiType=ENUM_TYPE_ADD_FRIEND_RESEND;
        MyTcpServer::getInstance().resend(caTarName,pdu);
        return NULL;
    }
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_ADD_FRIEND_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(int));
    return respdu;
}

// 同意好友
PDU *MsgHandler::addFriendAgree()
{

    char caCurName[32]={'\0'};
    char caTarName[32]={'\0'};
    memcpy(caCurName,pdu->caData,32);
    memcpy(caTarName,pdu->caData+32,32);
    bool ret=OperateDB::getInstance().handleAddFriendAgree(caCurName,caTarName);
    qDebug()<<"addFriendAgree ret"<<ret;
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_ADD_FRIEND_AGREE_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    MyTcpServer::getInstance().resend(caCurName,respdu);
    return respdu;
}

// 刷新好友列表
PDU *MsgHandler::flushFriend()
{
    QStringList res=OperateDB::getInstance().handleFlushFriend(pdu->caData);
    PDU* respdu=mkPDU(res.size()*32);
    respdu->uiType= ENUM_TYPE_FLUSH_FRIEND_RESPOND;
    for(int i=0;i<res.size();i++){
        QByteArray baName = res[i].toUtf8();
        memcpy(respdu->caMsg+i*32, baName.constData(), qMin(baName.size(), 32));
    }
    return respdu;
}

// 删除好友
PDU *MsgHandler::delFriend()
{
    char curName[32]={'\0'};
    char tarName[32]={'\0'};
    memcpy(curName,pdu->caData,32);
    memcpy(tarName,pdu->caData+32,32);
    bool ret=OperateDB::getInstance().handleDelFriend(curName,tarName);
    qDebug()<<"delFriend ret:"<<ret;
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_DEL_FRIEND_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 转发聊天消息
PDU *MsgHandler::chat()
{
    char tarName[32]={'\0'};
    memcpy(tarName,pdu->caData+32,32);
    pdu->uiType=ENUM_TYPE_CHAT_RESEND;
    MyTcpServer::getInstance().resend(tarName,pdu);
    return NULL;
}

// 创建文件夹
PDU *MsgHandler::mkdir()
{
    QString strPath=QString("%1/%2").arg(pdu->caMsg).arg(pdu->caData);
    qDebug()<<"mkdir strPath"<<strPath;
    QDir dir;
    bool ret=dir.mkdir(strPath);
    qDebug()<<"mkdir ret:"<<ret;
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_MKDIR_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 刷新文件列表
PDU *MsgHandler::flushFile()
{
    QDir dir(pdu->caMsg);
    QFileInfoList fileInfoList=dir.entryInfoList();

    int iCount=fileInfoList.size()-2;
    if(iCount<0) iCount=0;
    PDU* respdu=mkPDU(iCount*sizeof(FileInfo));
    respdu->uiType=ENUM_TYPE_FLUSH_FILE_RESPOND;
    for(int i=0,j=0;i<fileInfoList.size();i++){
        if(fileInfoList[i].fileName()=="."||fileInfoList[i].fileName()==".."){
            continue;
        }
        FileInfo* pFileInfo= (FileInfo*)(respdu->caMsg)+j++;
        if(fileInfoList[i].isDir()){
            pFileInfo->iFileType=0;
        }else{
            pFileInfo->iFileType=1;
        }
        QByteArray baName = fileInfoList[i].fileName().toUtf8();
        int iNameLen = qMin(baName.size(), 255);
        memcpy(pFileInfo->caName, baName.data(), iNameLen);
        pFileInfo->caName[iNameLen] = '\0';
        qDebug()<<"pFileInfo->caName"<<pFileInfo->caName;
    }
    return respdu;
}

// 删除文件
PDU *MsgHandler::delFile()
{
    QFileInfo fileInfo(pdu->caMsg);
    int ret;
    if(fileInfo.isDir()){
        QDir dir(pdu->caMsg);
        ret=dir.removeRecursively();
    }else{
        QFile file;
        ret=file.remove(pdu->caMsg);
    }
    qDebug()<<"delFile ret"<<ret;
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_DEL_FILE_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 重命名文件
PDU *MsgHandler::renameFile()
{
    char caOldName[32]={'\0'};
    char caNewName[32]={'\0'};
    memcpy(caOldName,pdu->caData,32);
    memcpy(caNewName,pdu->caData+32,32);
    QString strOldPath=QString("%1/%2").arg(pdu->caMsg).arg(caOldName);
    QString strNewPath=QString("%1/%2").arg(pdu->caMsg).arg(caNewName);
    QDir dir;
    bool ret= dir.rename(strOldPath,strNewPath);
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_RENAME_FILE_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 上传初始化
PDU *MsgHandler::uploadFileInit()
{
    char caFileName[32]={'\0'};
    memcpy(caFileName,pdu->caData,32);
    m_iUploadFileSize=0;
    memcpy(&m_iUploadFileSize,pdu->caData+32,sizeof(qint64));
    QString strPath=QString("%1/%2").arg(pdu->caMsg).arg(caFileName);
    m_fUploadFile.setFileName(strPath);
    m_iUploadFileReceived=0;

    QString nameRisk = FileScanner::checkFileName(caFileName);
    if (!nameRisk.isEmpty()) {
        qDebug() << "Upload blocked (L1):" << nameRisk;
        return makeUnsafeNotify(nameRisk);
    }

    QString extRisk = FileScanner::checkExtension(strPath);
    if (!extRisk.isEmpty()) {
        qDebug() << "Upload blocked (L2):" << extRisk;
        return makeUnsafeNotify(extRisk);
    }

    m_strUploadFileName = caFileName;
    m_strUploadDirPath = pdu->caMsg;

    // Ensure parent directory exists
    QDir dir;
    dir.mkpath(QFileInfo(strPath).absolutePath());

    bool ret= m_fUploadFile.open(QIODevice::WriteOnly);
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_UPLOAD_FILE_INIT_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 上传数据块
PDU *MsgHandler::uploadFileData()
{
    m_fUploadFile.write(pdu->caMsg,pdu->uiMsgLen);
    m_iUploadFileReceived+=pdu->uiMsgLen;
    if(m_iUploadFileReceived<m_iUploadFileSize){
        return NULL;
    }
    m_fUploadFile.close();

    QString strFilePath = m_fUploadFile.fileName();
    qDebug() << "Upload complete, security check:" << strFilePath;

    QString sizeRisk = FileScanner::checkFileSize(strFilePath);
    if (!sizeRisk.isEmpty()) {
        qDebug() << "Upload blocked (L3):" << sizeRisk;
        QFile::remove(strFilePath);
        return makeUnsafeNotify(sizeRisk);
    }

    QString magicRisk = FileScanner::checkMagicNumber(strFilePath);
    if (!magicRisk.isEmpty()) {
        qDebug() << "Upload blocked (L4):" << magicRisk;
        QFile::remove(strFilePath);
        return makeUnsafeNotify(magicRisk);
    }

    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_UPLOAD_FILE_DATA_RESPOND;
    return respdu;
}

// 下载文件
PDU *MsgHandler::downloadFile()
{
    QString strFilePath=pdu->caMsg;
    qDebug()<<"downloadFile strFilePath"<<strFilePath;

    QString extRisk = FileScanner::checkExtension(strFilePath);
    if (!extRisk.isEmpty()) {
        qDebug() << "Download blocked (L2):" << extRisk;
        return makeUnsafeNotify(extRisk);
    }

    QString sizeRisk = FileScanner::checkFileSize(strFilePath);
    if (!sizeRisk.isEmpty()) {
        qDebug() << "Download blocked (L3):" << sizeRisk;
        return makeUnsafeNotify(sizeRisk);
    }

    QString magicRisk = FileScanner::checkMagicNumber(strFilePath);
    if (!magicRisk.isEmpty()) {
        qDebug() << "Download blocked (L4):" << magicRisk;
        return makeUnsafeNotify(magicRisk);
    }

    QFile file(strFilePath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"downloadFile 打开文件失败";
        PDU* respdu=mkPDU();
        respdu->uiType=ENUM_TYPE_DOWNLOAD_FILE_RESPOND;
        qint64 iFileSize=0;
        memcpy(respdu->caData,&iFileSize,sizeof(qint64));
        return respdu;
    }
    qint64 iFileSize=file.size();
    int index=strFilePath.lastIndexOf('/');
    QString strFileName=strFilePath.right(strFilePath.size()-index-1);
    PDU* respdu=mkPDU(iFileSize);
    respdu->uiType=ENUM_TYPE_DOWNLOAD_FILE_RESPOND;
    memcpy(respdu->caData,&iFileSize,sizeof(qint64));
    QByteArray baFileName = strFileName.toUtf8();
    memcpy(respdu->caData+sizeof(qint64), baFileName.constData(), qMin(baFileName.size(), 32));
    file.read(respdu->caMsg,iFileSize);
    file.close();
    qDebug()<<"downloadFile 读取完成，大小:"<<iFileSize<<"文件名:"<<strFileName;
    return respdu;
}

// 分享文件
PDU *MsgHandler::shareFile()
{
    char caTarName[32]={'\0'};
    char caFileName[32]={'\0'};
    memcpy(caTarName,pdu->caData,32);
    memcpy(caFileName,pdu->caData+32,32);
    qDebug()<<"shareFile tarName:"<<caTarName<<"fileName:"<<caFileName;
    QString strSrcPath=pdu->caMsg;

    QString extRisk = FileScanner::checkExtension(strSrcPath);
    if (!extRisk.isEmpty()) {
        qDebug() << "Share blocked (L2):" << extRisk;
        return makeUnsafeNotify(extRisk);
    }

    QString sizeRisk = FileScanner::checkFileSize(strSrcPath);
    if (!sizeRisk.isEmpty()) {
        qDebug() << "Share blocked (L3):" << sizeRisk;
        return makeUnsafeNotify(sizeRisk);
    }

    QString magicRisk = FileScanner::checkMagicNumber(strSrcPath);
    if (!magicRisk.isEmpty()) {
        qDebug() << "Share blocked (L4):" << magicRisk;
        return makeUnsafeNotify(magicRisk);
    }

    int userStatus = OperateDB::getInstance().handleFindUser(caTarName);
    if (userStatus == -1 || userStatus == 2) {
        qDebug() << "shareFile: target user not found:" << caTarName;
        PDU* respdu = mkPDU();
        respdu->uiType = ENUM_TYPE_SHARE_FILE_RESPOND;
        bool ret = false;
        memcpy(respdu->caData, &ret, sizeof(bool));
        return respdu;
    }

    QString strTarDir=QString("%1/%2").arg(Server::getInstance().m_strRootPath).arg(caTarName);
    QString strTarPath=QString("%1/%2").arg(strTarDir).arg(caFileName);
    qDebug()<<"shareFile src:"<<strSrcPath<<"tar:"<<strTarPath;

    QDir shareDir;
    shareDir.mkpath(strTarDir);

    bool ret=QFile::copy(strSrcPath,strTarPath);
    qDebug()<<"shareFile ret:"<<ret;
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_SHARE_FILE_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

// 构建不安全文件通知
PDU* MsgHandler::makeUnsafeNotify(const QString& strReason)
{
    QByteArray baReason = strReason.toUtf8();
    PDU* respdu = mkPDU(baReason.size());
    respdu->uiType = ENUM_TYPE_UNSAFE_FILE_NOTIFY;
    memcpy(respdu->caMsg, baReason.data(), baReason.size());
    return respdu;
}

// 安全检查(L1-L4规则扫描)
PDU* MsgHandler::securityScan()
{
    QString strUserPath = pdu->caMsg;
    qDebug() << "securityScan path:" << strUserPath;

    QList<ScanResult> results = FileScanner::scanDirectory(strUserPath);
    qDebug() << "securityScan found:" << results.size();

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << (quint32)results.size();
    for (const ScanResult& r : results) {
        QString strRelPath = r.strFilePath;
        if (strRelPath.startsWith(strUserPath)) {
            strRelPath = strRelPath.mid(strUserPath.length());
            if (strRelPath.startsWith('/')) strRelPath = strRelPath.mid(1);
        }

        QByteArray baPath = strRelPath.toUtf8();
        QByteArray baReason = r.strReason.toUtf8();

        stream << (quint32)baPath.size();
        stream.writeRawData(baPath.data(), baPath.size());
        stream << (qint32)r.iRiskLevel;
        stream << (quint32)baReason.size();
        stream.writeRawData(baReason.data(), baReason.size());
    }

    PDU* respdu = mkPDU(ba.size());
    respdu->uiType = ENUM_TYPE_SECURITY_SCAN_RESPOND;
    memcpy(respdu->caMsg, ba.data(), ba.size());
    return respdu;
}

// 删除不安全文件
PDU* MsgHandler::unsafeFileDel()
{
    QString strFilePath = pdu->caMsg;
    qDebug() << "unsafeFileDel:" << strFilePath;

    bool ret = false;
    QFileInfo info(strFilePath);
    if (info.exists()) {
        if (info.isDir()) {
            QDir dir(strFilePath);
            ret = dir.removeRecursively();
        } else {
            QFile file(strFilePath);
            ret = file.remove(strFilePath);
        }
    }

    qDebug() << "unsafeFileDel ret:" << ret;
    PDU* respdu = mkPDU();
    respdu->uiType = ENUM_TYPE_UNSAFE_FILE_DEL_REQUEST;
    memcpy(respdu->caData, &ret, sizeof(bool));
    return respdu;
}

// AI深度扫描
PDU* MsgHandler::aiAnalyze()
{
    QString strDirPath = pdu->caMsg;
    qDebug() << "aiAnalyze scanning directory:" << strDirPath;

    if (!AIChecker::getInstance().isConfigured()) {
        QByteArray ba = QString("AI 未配置: 请在服务端 connect.config 第4行添加 API Key").toUtf8();
        PDU* respdu = mkPDU(ba.size() + 1);
        respdu->uiType = ENUM_TYPE_AI_ANALYZE_RESPOND;
        memcpy(respdu->caMsg, ba.data(), ba.size());
        return respdu;
    }

    QDir dir(strDirPath);
    if (!dir.exists()) {
        QByteArray ba = QString("Directory not found: %1").arg(strDirPath).toUtf8();
        PDU* respdu = mkPDU(ba.size() + 1);
        respdu->uiType = ENUM_TYPE_AI_ANALYZE_RESPOND;
        memcpy(respdu->caMsg, ba.data(), ba.size());
        return respdu;
    }

    QStringList textExts = {".txt", ".csv", ".json", ".xml", ".md", ".cpp", ".h",
                            ".py", ".js", ".html", ".css", ".yaml", ".yml", ".log",
                            ".ini", ".cfg", ".conf", ".sh", ".bat", ".ps1", ".sql"};

    struct TextFileInfo {
        QString path;       // relative path for AI display
        QString absPath;    // absolute path for deletion
        QString content;
    };
    QList<TextFileInfo> textFiles;
    int totalSize = 0;
    int fileCount = 0;

    std::function<void(const QString&)> collectFiles = [&](const QString& currentDir) {
        if (fileCount >= 10 || totalSize > 6000) return;

        QDir d(currentDir);
        QFileInfoList entries = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& entry : entries) {
            if (fileCount >= 10 || totalSize > 6000) break;

            if (entry.isDir()) {
                collectFiles(entry.absoluteFilePath());
                continue;
            }

            QString ext = entry.suffix().toLower();
            if (!ext.isEmpty()) ext = "." + ext;
            if (!textExts.contains(ext)) continue;

            QFile file(entry.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QString content = QString::fromUtf8(file.read(4096));
            file.close();

            if (content.isEmpty()) continue;

            TextFileInfo info;
            info.path = d.relativeFilePath(entry.filePath());
            info.absPath = entry.absoluteFilePath();
            info.content = content;
            textFiles.append(info);
            totalSize += content.size();
            fileCount++;
        }
    };

    collectFiles(strDirPath);

    if (textFiles.isEmpty()) {
        QByteArray ba = QString("AI 扫描完成: %1 中未发现文本文件。\n二进制文件和媒体文件不在 AI 分析范围内。").arg(strDirPath).toUtf8();
        PDU* respdu = mkPDU(ba.size() + 1);
        respdu->uiType = ENUM_TYPE_AI_ANALYZE_RESPOND;
        memcpy(respdu->caMsg, ba.data(), ba.size());
        return respdu;
    }

    QString combinedContent;
    for (const TextFileInfo& info : textFiles) {
        combinedContent += QString("--- FILE: %1 ---\n%2\n\n").arg(info.path, info.content);
    }

    QString aiResult;
    QEventLoop loop;
    AIChecker::getInstance().analyzeContent(strDirPath, combinedContent,
        [&](const QString& result) {
            aiResult = result;
            loop.quit();
        });
    loop.exec();

    QStringList deletedFiles;
    if (!aiResult.contains("未发现敏感信息")) {
        for (const TextFileInfo& info : textFiles) {
            QString fileName = info.path.section('/', -1);
            bool matched = aiResult.contains("FILE: " + info.path) ||
                           aiResult.contains("文件: " + info.path) ||
                           aiResult.contains("文件 " + info.path) ||
                           aiResult.contains(": " + fileName) ||
                           aiResult.contains("：" + fileName);
            if (matched) {
                QFile::remove(info.absPath);
                deletedFiles.append(info.path);
                qDebug() << "AI flagged file deleted:" << info.absPath;
            }
        }
    }

    QString summary = QString("AI 深度扫描报告\n扫描目录: %1\n扫描文本文件: %2 个\n\n").arg(strDirPath).arg(fileCount) + aiResult;

    if (!deletedFiles.isEmpty()) {
        summary += "\n\n===== 已自动删除的高危文件 =====";
        for (const QString& f : deletedFiles) {
            summary += QString("\n🚫 %1").arg(f);
        }
        summary += QString("\n\n共删除 %1 个高危文件。").arg(deletedFiles.size());
    }

    QByteArray ba = summary.toUtf8();
    PDU* respdu = mkPDU(ba.size() + 1);
    respdu->uiType = ENUM_TYPE_AI_ANALYZE_RESPOND;
    memcpy(respdu->caMsg, ba.data(), ba.size());
    return respdu;
}

// AI安全报告
PDU* MsgHandler::aiReport()
{
    QString strSummary = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
    qDebug() << "aiReport summary length:" << strSummary.size();

    if (!AIChecker::getInstance().isConfigured()) {
        QByteArray ba = QString("AI 未配置: 请在服务端 connect.config 第4行添加 API Key").toUtf8();
        PDU* respdu = mkPDU(ba.size());
        respdu->uiType = ENUM_TYPE_AI_REPORT_RESPOND;
        memcpy(respdu->caMsg, ba.data(), ba.size());
        return respdu;
    }

    int totalFiles = 0, unsafeCount = 0;
    memcpy(&totalFiles, pdu->caData, sizeof(int));
    memcpy(&unsafeCount, pdu->caData + sizeof(int), sizeof(int));

    QString aiReport;
    QEventLoop loop;
    AIChecker::getInstance().generateReport(strSummary, totalFiles, unsafeCount,
        [&](const QString& result) {
            aiReport = result;
            loop.quit();
        });
    loop.exec();

    QByteArray ba = aiReport.toUtf8();
    PDU* respdu = mkPDU(ba.size());
    respdu->uiType = ENUM_TYPE_AI_REPORT_RESPOND;
    memcpy(respdu->caMsg, ba.data(), ba.size());
    return respdu;
}
