#include "client.h"
#include "index.h"
#include "reshandler.h"
#include "string.h"
#include <QDataStream>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
ResHandler::ResHandler()
{

}

// 注册响应
void ResHandler::regist()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"regist ret"<<ret;
    if(ret){
        QMessageBox::information(&Client::getInstance(),"提示","注册成功");
    }else{
        QMessageBox::information(&Client::getInstance(),"提示","注册失败");
    }

}

// 登录响应
void ResHandler::login()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"login ret"<<ret;
    if(ret){
       Index::getInstance().show();
       Index::getInstance().getFile()->flushFile();
       Client::getInstance().hide();
    }else{
        QMessageBox::information(&Client::getInstance(),"提示","登录失败");
    }
}

// 查找用户响应
void ResHandler::findUser()
{
    int ret;
    memcpy(&ret,pdu->caData,sizeof(int));
    qDebug()<<"find user ret"<<ret;
    if(ret==0){
       QMessageBox::information(&Index::getInstance(),"提示","该用户不在线");
    }
    else if(ret==1){
       QMessageBox::information(&Index::getInstance(),"提示","该用户在线");
    }
    else if(ret==2){
       QMessageBox::information(&Index::getInstance(),"提示","该用户不存在");
    }else if(ret==-1){
        QMessageBox::information(&Index::getInstance(),"提示","查找失败");
    }
}

// 在线用户响应
void ResHandler::onlineUser()
{
    uint uiSize=pdu->uiMsgLen/32;
    char caTmp[32]={'\0'};
    QStringList userList;
    for (uint i=0;i<uiSize;i++) {
       memcpy(caTmp,pdu->caMsg+i*32,32);
       userList.append(QString::fromUtf8(caTmp));
    }
    Index::getInstance().getFriend()->m_pOnlineUser->updateLW(userList);
}

// 添加好友响应
void ResHandler::addFriend()
{
    int ret;
    memcpy(&ret,pdu->caData,sizeof(int));
    qDebug()<<"addFriend ret"<<ret;
    if(ret==0){
       QMessageBox::information(&Index::getInstance(),"提示","该用户不在线");
    }
    else if(ret==-2){
       QMessageBox::information(&Index::getInstance(),"提示","该用户已经是好友");
    }else if(ret==-1){
        QMessageBox::information(&Index::getInstance(),"提示","服务器错误：联系开发人员");
    }
}

// 好友请求转发
void ResHandler::addFriendResend()
{
    char caName[32]={'\0'};
    memcpy(caName,pdu->caData,32);
    int ret=QMessageBox::question(&Index::getInstance(),"添加好友",QString("是否同意 %1 的添加好友请求？").arg(QString::fromUtf8(caName)));
    if(ret!=QMessageBox::Yes){
        return;
    }
    PDU* respdu=mkPDU();
    memcpy(respdu->caData,pdu->caData,64);
    respdu->uiType=ENUM_TYPE_ADD_FRIEND_AGREE_REQUEST;
    Client::getInstance().sendMsg(respdu);
}

// 同意好友响应
void ResHandler::addFriendAgree()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"addFriendAgree ret"<<ret;
    if(ret){
       QMessageBox::information(&Index::getInstance(),"提示","添加好友成功");
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","添加好友失败");
    }
}

// 刷新好友响应
void ResHandler::flushFriend()
{
    QStringList friendList;
    int iSize=pdu->uiMsgLen/32;
    char caTmp[32]={'\0'};
    for(int i=0;i<iSize;i++){
       memcpy(caTmp,pdu->caMsg+i*32,32);
       friendList.append(QString::fromUtf8(caTmp));
    }
    Index::getInstance().getFriend()->update_LW(friendList);
}

// 删除好友响应
void ResHandler::delFriend()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"delFriend ret"<<ret;
    if(ret){
        Index::getInstance().getFriend()->flushFriend();
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","删除好友失败");
    }
}

// 聊天消息响应
void ResHandler::chat()
{
   Chat* c=Index::getInstance().getFriend()->m_pChat;
   if(c->isHidden()){
       c->show();
   }
   char caChatName[32]={'\0'};
   memcpy(caChatName,pdu->caData,32);
   c->updateShow_TE(QString("%1 : %2").arg(QString::fromUtf8(caChatName)).arg(QString::fromUtf8(pdu->caMsg)));
   c->m_strChatName=caChatName;
   c->setWindowTitle(QString("与 %1 聊天中").arg(caChatName));
}

// 创建文件夹响应
void ResHandler::mkdir()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"regist ret"<<ret;
    if(ret){
       Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","创建文件夹失败");
    }
}

// 刷新文件响应
void ResHandler::flushFile()
{
    int iCount=pdu->uiMsgLen/sizeof (FileInfo);

    QList<FileInfo*>pFileInfoList;
    for(int i=0;i<iCount;i++){
        FileInfo* pFileInfo=new FileInfo;
        memcpy(pFileInfo,pdu->caMsg+i*sizeof (FileInfo),sizeof (FileInfo) );
        pFileInfoList.append(pFileInfo);
    }
    Index::getInstance().getFile()->updateFileList(pFileInfoList);
}

// 删除文件响应
void ResHandler::delFile()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"delFile ret"<<ret;
    if(ret){
       Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","删除文件失败");
    }
}

// 重命名响应
void ResHandler::renameFile()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"renameFile ret"<<ret;
    if(ret){
       Index::getInstance().getFile()->flushFile();
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","重命名文件失败");
    }
}

// 上传初始化响应
void ResHandler::uploadFileInit()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"uploadFileInit ret"<<ret;
    if(ret){
       Index::getInstance().getFile()->uploadFile();
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","上传文件失败");
    }
}

// 上传完成响应
void ResHandler::uploadFileData()
{
    QMessageBox::information(&Index::getInstance(),"提示","上传文件完成");
    Index::getInstance().getFile()->flushFile();
}

// 下载文件响应
void ResHandler::downloadFile()
{
    qint64 iFileSize=0;
    memcpy(&iFileSize,pdu->caData,sizeof(qint64));
    qDebug()<<"downloadFile iFileSize:"<<iFileSize;
    if(iFileSize==0){
        QMessageBox::information(&Index::getInstance(),"提示","下载失败：文件不存在或为空");
        return;
    }
    char caFileName[32]={'\0'};
    memcpy(caFileName,pdu->caData+sizeof(qint64),32);
    QString strFileName=QString::fromUtf8(caFileName);
    qDebug()<<"downloadFile fileName:"<<strFileName;

    QString strSavePath=QFileDialog::getSaveFileName(&Index::getInstance(),"保存文件",strFileName);
    if(strSavePath.isEmpty()){
        return;
    }
    QFile file(strSavePath);
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox::information(&Index::getInstance(),"提示","保存文件失败：无法打开文件");
        return;
    }
    file.write(pdu->caMsg,iFileSize);
    file.close();
    QMessageBox::information(&Index::getInstance(),"提示","下载文件成功");
}

// 分享文件响应
void ResHandler::shareFile()
{
    bool ret;
    memcpy(&ret,pdu->caData,sizeof(bool));
    qDebug()<<"shareFile ret"<<ret;
    if(ret){
        QMessageBox::information(&Index::getInstance(),"提示","分享文件成功");
    }else{
        QMessageBox::information(&Index::getInstance(),"提示","分享文件失败");
    }
}

// 不安全文件通知
void ResHandler::unsafeFileNotify()
{
    QString strReason = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
    qDebug() << "unsafeFileNotify:" << strReason;
    QMessageBox::warning(&Index::getInstance(), "⚠ 安全警告",
                         QString("操作被安全策略拦截：\n\n%1\n\n该文件可能包含安全风险，已阻止操作。").arg(strReason));
}

// 安全检查响应
void ResHandler::securityScan()
{
    QByteArray ba(pdu->caMsg, pdu->uiMsgLen);
    QDataStream stream(ba);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 uiCount = 0;
    stream >> uiCount;

    qDebug() << "securityScan 文件数量:" << uiCount;

    if (uiCount == 0) {
        QMessageBox::information(&Index::getInstance(), "安全检查",
                                 "未发现安全风险，规则扫描(L1-L4)全部通过。");
    } else {
        QStringList riskLines;
        QStringList riskPaths;
        for (quint32 i = 0; i < uiCount; i++) {
            quint32 uiPathLen = 0;
            stream >> uiPathLen;
            QByteArray baPath(uiPathLen, '\0');
            stream.readRawData(baPath.data(), uiPathLen);

            qint32 iRiskLevel = 0;
            stream >> iRiskLevel;

            quint32 uiReasonLen = 0;
            stream >> uiReasonLen;
            QByteArray baReason(uiReasonLen, '\0');
            stream.readRawData(baReason.data(), uiReasonLen);

            QString strPath = QString::fromUtf8(baPath);
            QString strReason = QString::fromUtf8(baReason);

            QString strLevel;
            if (iRiskLevel == 3) strLevel = "高危";
            else if (iRiskLevel == 2) strLevel = "中危";
            else strLevel = "低危";

            riskLines.append(QString("%1 | %2 | %3").arg(strLevel, strPath, strReason));
            riskPaths.append(strPath);
        }

        QString strReport = QString("发现 %1 个不安全文件:\n\n").arg(uiCount) + riskLines.join("\n");
        int ret = QMessageBox::warning(&Index::getInstance(), "安全检查结果",
                                        strReport + "\n\n是否删除这些不安全文件？",
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (ret == QMessageBox::Yes) {
            QString strUserPath = Index::getInstance().getFile()->m_strUserPath;
            for (const QString& strRelPath : riskPaths) {
                QString strFullPath = strUserPath + "/" + strRelPath;
                QByteArray baFullPath = strFullPath.toUtf8();
                PDU* delPdu = mkPDU(baFullPath.size() + 1);
                delPdu->uiType = ENUM_TYPE_UNSAFE_FILE_DEL_REQUEST;
                memcpy(delPdu->caMsg, baFullPath.data(), baFullPath.size());
                Client::getInstance().sendMsg(delPdu);
            }
            QMessageBox::information(&Index::getInstance(), "提示",
                                     QString("已删除 %1 个不安全文件，正在刷新...").arg(uiCount));
            Index::getInstance().getFile()->flushFile();
        }
    }

    int aiRet = QMessageBox::question(&Index::getInstance(), "AI 智能分析",
                                       "是否使用 AI 对所有文件进行深度扫描？\n\n"
                                       "AI 可以检测规则扫描无法发现的内容:\n"
                                       "- 身份证号、银行卡号、手机号\n"
                                       "- 密码、API 密钥、Token 令牌\n"
                                       "- 敏感信息泄露\n\n"
                                       "（需在服务端配置 API Key）",
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (aiRet == QMessageBox::Yes) {
        QString strUserPath = Index::getInstance().getFile()->m_strUserPath;
        QByteArray baPath = strUserPath.toUtf8();
        PDU* aiPdu = mkPDU(baPath.size() + 1);
        aiPdu->uiType = ENUM_TYPE_AI_ANALYZE_REQUEST;
        memcpy(aiPdu->caMsg, baPath.data(), baPath.size());
        Client::getInstance().sendMsg(aiPdu);
    }
}

// AI分析响应
void ResHandler::aiAnalyze()
{
    QString strResult = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
    qDebug() << "aiAnalyze result:" << strResult.left(200);

    bool hasDeletion = strResult.contains("已自动删除") || strResult.contains("已删除");

    if (hasDeletion) {
        QMessageBox::warning(&Index::getInstance(), "AI 深度扫描 — 发现高危文件",
                             strResult + "\n\n⚠ 以上高危文件已被自动删除。");
    } else {
        QMessageBox::information(&Index::getInstance(), "AI 深度扫描报告", strResult);
    }

    Index::getInstance().getFile()->flushFile();
}

// AI报告响应
void ResHandler::aiReport()
{
    QString strResult = QString::fromUtf8(pdu->caMsg, pdu->uiMsgLen);
    qDebug() << "aiReport result:" << strResult.left(200);
    QMessageBox::information(&Index::getInstance(), "AI 安全报告", strResult);
}
