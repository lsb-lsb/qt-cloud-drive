#include "clienttask.h"
#include "mytcpserver.h"
#include "mytcpsocket.h"
#include <QDebug>
MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

// 新客户端连接
void MyTcpServer::incomingConnection(qintptr handle)
{
    qDebug()<<"新客户端连接";
    MyTcpSocket* pTcpSocket=new MyTcpSocket;
    pTcpSocket->setSocketDescriptor(handle);
    m_tcpSocketList.append(pTcpSocket);
    ClientTask* task=new ClientTask(pTcpSocket);
    threadPool.start(task);

}

void MyTcpServer::removeSocket(MyTcpSocket *mySocket)
{
    m_tcpSocketList.removeOne(mySocket);
    mySocket->deleteLater();
    mySocket=NULL;

}

// 转发消息给目标用户
void MyTcpServer::resend(char *caTarName, PDU *pdu)
{
    if(caTarName==NULL||pdu==NULL){
        return;
    }
    for(int i=0;i<m_tcpSocketList.size();i++){
        if(QString::fromUtf8(caTarName,32) == m_tcpSocketList[i]->m_strLoginName){
            m_tcpSocketList[i]->write((char*)pdu,pdu->uiTotalLen);
            qDebug()<<"resend pdu->uiTotalLen"<<pdu->uiTotalLen
                    <<"pdu->uiMsgLen"<<pdu->uiMsgLen
                    <<"pdu->uiType"<<pdu->uiType
                    <<"pdu->caData"<<pdu->caData
                    <<"pdu->caData+32"<<pdu->caData+32
                    <<"pdu->caMsg"<<pdu->caMsg;
            break;
        }
    }

}

MyTcpServer::MyTcpServer()
{
    threadPool.setMaxThreadCount(8);
}
