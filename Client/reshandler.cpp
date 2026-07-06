#include "client.h"
#include "index.h"
#include "reshandler.h"
#include "string.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
ResHandler::ResHandler()
{

}

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

void ResHandler::onlineUser()
{
    uint uiSize=pdu->uiMsgLen/32;
    char caTmp[32]={'\0'};
    QStringList userList;
    for (uint i=0;i<uiSize;i++) {
       memcpy(caTmp,pdu->caMsg+i*32,32);
       userList.append(caTmp);
    }
    Index::getInstance().getFriend()->m_pOnlineUser->updateLW(userList);
}

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

void ResHandler::addFriendResend()
{
    char caName[32]={'\0'};
    memcpy(caName,pdu->caData,32);
    int ret=QMessageBox::question(&Index::getInstance(),"添加好友",QString("是否同意 %1 的添加好友请求？").arg(caName));
    if(ret!=QMessageBox::Yes){
        return;
    }
    PDU* respdu=mkPDU();
    memcpy(respdu->caData,pdu->caData,64);
    respdu->uiType=ENUM_TYPE_ADD_FRIEND_AGREE_REQUEST;
    Client::getInstance().sendMsg(respdu);
}

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

void ResHandler::flushFriend()
{
    QStringList friendList;
    int iSize=pdu->uiMsgLen/32;
    char caTmp[32]={'\0'};
    for(int i=0;i<iSize;i++){
       memcpy(caTmp,pdu->caMsg+i*32,32);
       friendList.append(caTmp);
    }
    Index::getInstance().getFriend()->update_LW(friendList);
}

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

void ResHandler::chat()
{
   Chat* c=Index::getInstance().getFriend()->m_pChat;
   if(c->isHidden()){
       c->show();
   }
   char caChatName[32]={'\0'};
   memcpy(caChatName,pdu->caData,32);
   c->updateShow_TE(QString("%1 : %2").arg(caChatName).arg(pdu->caMsg));
   c->m_strChatName=caChatName;
   c->setWindowTitle(QString("与 %1 聊天中").arg(caChatName));
}

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

void ResHandler::uploadFileData()
{
    QMessageBox::information(&Index::getInstance(),"提示","上传文件完成");
    Index::getInstance().getFile()->flushFile();
}

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
    QString strFileName=QString(caFileName);
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
