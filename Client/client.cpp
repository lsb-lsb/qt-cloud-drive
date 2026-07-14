#include "client.h"
#include "index.h"
#include "protocol.h"
#include "ui_client.h"

#include <QFile>
#include <QDebug>
#include <QHostAddress>
#include <QMessageBox>
#include <QRegExp>
Client::Client(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);
    m_prh=new ResHandler;
    loadConfig();
    m_socket.connectToHost(QHostAddress(m_strIP),m_usPort);
    connect(&m_socket,&QTcpSocket::connected,this,&Client::showConnect );
    connect(&m_socket,&QTcpSocket::readyRead,this,&Client::recvMsg);
}

Client::~Client()
{
    delete ui;
    delete m_prh;
}

void Client::loadConfig()
{
    QFile file(":/connect.config");
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"打开文件失败";
        return;
    }
    QByteArray baData=file.readAll();
    QString strData=QString(baData);
    qDebug()<<"strData"<<strData;
    QStringList strList=strData.split(QRegExp("[\r\n]+"), QString::SkipEmptyParts);
    m_strIP=strList[0];
    m_usPort=strList[1].toUShort();
    m_strRootPath=strList[2];
    qDebug()<<"ip"<<m_strIP<<"Port"<<m_usPort<<"strRootPath"<<m_strRootPath;
    file.close();
}

Client &Client::getInstance()
{
    static Client instance;
    return instance;
}

void Client::sendMsg(PDU *pdu)
{
    m_socket.write((char*)pdu,pdu->uiTotalLen);
     qDebug() << "send msg pdu->uiTotalLen"<<pdu->uiTotalLen
            << "pdu->uiMsgLen"<<pdu->uiMsgLen
           << "pdu->uiiType"<<pdu->uiType
            <<"pdu->caData"<<pdu->caData
              <<"pdu->caData+32"<<pdu->caData+32;
//            <<"pdu->caMsg"<<pdu->caMsg;
     free(pdu);
     pdu=NULL;
}

PDU *Client::readMsg()
{
    qDebug()<<"recvMsg 接收消息长度"<<m_socket.bytesAvailable();
    uint uiPDULen=0;
    m_socket.read((char*)&uiPDULen,sizeof(uint));
    uint uiMsgLen=uiPDULen-sizeof(PDU);
    PDU *pdu=mkPDU(uiMsgLen);
    m_socket.read((char*)pdu+sizeof(uint),uiPDULen-sizeof(uint));
    return pdu;
}

// 消息分派
void Client::handleMsg(PDU *pdu)
{
    qDebug()<<"handleMsg pdu->uiTotalLen"<<pdu->uiTotalLen
            <<"pdu->uiMsgLen"<<pdu->uiMsgLen
            <<"pdu->uiType"<<pdu->uiType
            <<"pdu->caData"<<pdu->caData
            <<"pdu->caData+32"<<pdu->caData+32
            <<"pdu->caMsg"<<pdu->caMsg;
    m_prh->pdu=pdu;
    switch(pdu->uiType){
        case ENUM_TYPE_REGIST_RESPOND:{
             m_prh->regist();
             break;
        }
    case ENUM_TYPE_LOGIN_RESPOND:{
         m_prh->login();
         break;
    }
    case ENUM_TYPE_FIND_USER_RESPOND:{
         m_prh->findUser();
         break;
    }
    case ENUM_TYPE_ONLINE_USER_RESPOND:{
        m_prh->onlineUser();
        break;
    }
    case ENUM_TYPE_ADD_FRIEND_RESPOND:{
        m_prh->addFriend();
        break;
    }
    case ENUM_TYPE_ADD_FRIEND_RESEND:{
        m_prh->addFriendResend();
        break;
    }
    case ENUM_TYPE_ADD_FRIEND_AGREE_RESPOND:{
        m_prh->addFriendAgree();
        break;
    }
    case ENUM_TYPE_FLUSH_FRIEND_RESPOND:{
        m_prh->flushFriend();
        break;
    }
    case ENUM_TYPE_DEL_FRIEND_RESPOND:{
        m_prh->delFriend();
        break;
    }
    case ENUM_TYPE_CHAT_RESEND:{
        m_prh->chat();
        break;
    }
    case ENUM_TYPE_MKDIR_RESPOND:{
        m_prh->mkdir();
        break;
    }
    case ENUM_TYPE_FLUSH_FILE_RESPOND:{
        m_prh->flushFile();
        break;
    }
    case ENUM_TYPE_DEL_FILE_RESPOND:{
        m_prh->delFile();
        break;
    }
    case ENUM_TYPE_RENAME_FILE_RESPOND:{
        m_prh->renameFile();
        break;
    }
    case ENUM_TYPE_UPLOAD_FILE_INIT_RESPOND:{
        m_prh->uploadFileInit();
        break;
    }
    case ENUM_TYPE_UPLOAD_FILE_DATA_RESPOND:{
        m_prh->uploadFileData();
        break;
    }
    case ENUM_TYPE_DOWNLOAD_FILE_RESPOND:{
        m_prh->downloadFile();
        break;
    }
    case ENUM_TYPE_SHARE_FILE_RESPOND:{
        m_prh->shareFile();
        break;
    }
    case ENUM_TYPE_SECURITY_SCAN_RESPOND:{
        m_prh->securityScan();
        break;
    }
    case ENUM_TYPE_UNSAFE_FILE_NOTIFY:{
        m_prh->unsafeFileNotify();
        break;
    }
    case ENUM_TYPE_AI_ANALYZE_RESPOND:{
        m_prh->aiAnalyze();
        break;
    }
    case ENUM_TYPE_AI_REPORT_RESPOND:{
        m_prh->aiReport();
        break;
    }
    default:
        break;
    }
}

// TCP连接成功
void Client::showConnect()
{
    qDebug()<<"连接成功";
}

// 接收TCP消息
void Client::recvMsg()
{
    qDebug()<<"recvMsg 接受消息长度"<<m_socket.bytesAvailable();
    QByteArray data= m_socket.readAll();
    buffer.append(data);
    while(buffer.size()>=int(sizeof(PDU))){
        PDU* pdu=(PDU*)buffer.data();
        if(buffer.size()<int(pdu->uiTotalLen)){
            break;
        }
        handleMsg(pdu);
        buffer.remove(0,pdu->uiTotalLen);
    }

}

//void Client::on_send_PB_clicked()
//{
//   QString strMsg=ui->input_LE->text();
//    PDU *pdu=mkPDU(strMsg.toUtf8().size());
//    pdu->uiType=ENUM_TYPE_REGIST_REQUEST;
//    memcpy(pdu->caMsg,strMsg.toUtf8().constData(),strMsg.toUtf8().size());
//    m_socket.write((char*)pdu,pdu->uiTotalLen);
//    qDebug() << "send msg pdu->uiTotalLen"<<pdu->uiTotalLen
//           << "pdu->uiMsgLen"<<pdu->uiMsgLen
//           << "pdu->uiiType"<<pdu->uiType
//           <<"pdu->caData"<<pdu->caData
//           <<"pdu->caMsg"<<pdu->caMsg;
//    free(pdu);
//    pdu=NULL;
//}




// 注册
void Client::on_regist_PB_clicked()
{
   QString strName= ui->name_LE->text();
   QString strPwd= ui->pwd_LE->text();
   if(strName.isEmpty()||strPwd.isEmpty()||strName.toUtf8().size()>32||strPwd.toUtf8().size()>32){
       QMessageBox::warning(this,"注册","用户名或密码长度非法");
       return;
   }
   PDU* pdu=mkPDU();
   memcpy(pdu->caData,strName.toUtf8().constData(),32);
   memcpy(pdu->caData+32,strPwd.toUtf8().constData(),32);
   pdu->uiType=ENUM_TYPE_REGIST_REQUEST;
   sendMsg(pdu);
}

// 登录
void Client::on_login_PB_clicked()
{
    QString strName= ui->name_LE->text();
    QString strPwd= ui->pwd_LE->text();
    if(strName.isEmpty()||strPwd.isEmpty()||strName.toUtf8().size()>32||strPwd.toUtf8().size()>32){
        QMessageBox::warning(this,"登录","用户名或密码长度非法");
        return;
    }
    m_strLoginName=strName;
    PDU* pdu=mkPDU();
    memcpy(pdu->caData,strName.toUtf8().constData(),32);
    memcpy(pdu->caData+32,strPwd.toUtf8().constData(),32);
    pdu->uiType=ENUM_TYPE_LOGIN_REQUEST;
    sendMsg(pdu);
}
