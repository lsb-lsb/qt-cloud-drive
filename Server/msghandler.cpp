#include "msghandler.h"
#include "mytcpserver.h"
#include "operatedb.h"
#include "server.h"
#include "stdlib.h"
#include "string.h"
#include <QDebug>
#include <QDir>
MsgHandler::MsgHandler()
{

}

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

PDU *MsgHandler::onlineUser()
{
    QStringList res=OperateDB::getInstance().handleOnlineUser();
    PDU* respdu=mkPDU(res.size()*32);
    respdu->uiType=ENUM_TYPE_ONLINE_USER_RESPOND;
    for(int i=0;i<res.size();i++){
        memcpy(respdu->caMsg+i*32,res[i].toStdString().c_str(),32);
    }
    return respdu;
}

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

PDU *MsgHandler::flushFriend()
{
    QStringList res=OperateDB::getInstance().handleFlushFriend(pdu->caData);
    PDU* respdu=mkPDU(res.size()*32);
    respdu->uiType= ENUM_TYPE_FLUSH_FRIEND_RESPOND;
    for(int i=0;i<res.size();i++){
        memcpy(respdu->caMsg+i*32,res[i].toStdString().c_str(),32);
    }
    return respdu;
}

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

PDU *MsgHandler::chat()
{
    char tarName[32]={'\0'};
    memcpy(tarName,pdu->caData+32,32);
    pdu->uiType=ENUM_TYPE_CHAT_RESEND;
    MyTcpServer::getInstance().resend(tarName,pdu);
    return NULL;
}

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
        memcpy(pFileInfo->caName,fileInfoList[i].fileName().toStdString().c_str(),32);
        qDebug()<<"pFileInfo->caName"<<pFileInfo->caName;
    }
    return respdu;
}

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

PDU *MsgHandler::uploadFileInit()
{
    char caFileName[32]={'\0'};
    memcpy(caFileName,pdu->caData,32);
    m_iUploadFileSize=0;
    memcpy(&m_iUploadFileSize,pdu->caData+32,sizeof(qint64));
    QString strPath=QString("%1/%2").arg(pdu->caMsg).arg(caFileName);
    m_fUploadFile.setFileName(strPath);
    m_iUploadFileReceived=0;
    bool ret= m_fUploadFile.open(QIODevice::WriteOnly);
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_UPLOAD_FILE_INIT_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}

PDU *MsgHandler::uploadFileData()
{
    m_fUploadFile.write(pdu->caMsg,pdu->uiMsgLen);
    m_iUploadFileReceived+=pdu->uiMsgLen;
    if(m_iUploadFileReceived<m_iUploadFileSize){
        return NULL;
    }
    m_fUploadFile.close();
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_UPLOAD_FILE_DATA_RESPOND;
    return respdu;
}

PDU *MsgHandler::downloadFile()
{
    QString strFilePath=pdu->caMsg;
    qDebug()<<"downloadFile strFilePath"<<strFilePath;
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
    // 提取文件名
    int index=strFilePath.lastIndexOf('/');
    QString strFileName=strFilePath.right(strFilePath.size()-index-1);
    PDU* respdu=mkPDU(iFileSize);
    respdu->uiType=ENUM_TYPE_DOWNLOAD_FILE_RESPOND;
    memcpy(respdu->caData,&iFileSize,sizeof(qint64));
    memcpy(respdu->caData+sizeof(qint64),strFileName.toStdString().c_str(),32);
    file.read(respdu->caMsg,iFileSize);
    file.close();
    qDebug()<<"downloadFile 读取完成，大小:"<<iFileSize<<"文件名:"<<strFileName;
    return respdu;
}

PDU *MsgHandler::shareFile()
{
    char caTarName[32]={'\0'};
    char caFileName[32]={'\0'};
    memcpy(caTarName,pdu->caData,32);
    memcpy(caFileName,pdu->caData+32,32);
    qDebug()<<"shareFile tarName:"<<caTarName<<"fileName:"<<caFileName;
    QString strSrcPath=pdu->caMsg;
    QString strTarDir=QString("%1/%2").arg(Server::getInstance().m_strRootPath).arg(caTarName);
    QString strTarPath=QString("%1/%2").arg(strTarDir).arg(caFileName);
    qDebug()<<"shareFile src:"<<strSrcPath<<"tar:"<<strTarPath;
    bool ret=QFile::copy(strSrcPath,strTarPath);
    qDebug()<<"shareFile ret:"<<ret;
    PDU* respdu=mkPDU();
    respdu->uiType=ENUM_TYPE_SHARE_FILE_RESPOND;
    memcpy(respdu->caData,&ret,sizeof(bool));
    return respdu;
}


