#include "mytcpserver.h"
#include "server.h"

#include <QFile>
#include <QDebug>
Server::Server(QWidget *parent)
    : QWidget(parent)
{
    loadConfig();
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usPort);
}

Server &Server::getInstance()
{
    static Server instance;
    return instance;
}

Server::~Server()
{
}

void Server::loadConfig()
{
    QFile file(":/connect.config");
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"打开文件失败";
        return;
    }
    QByteArray baData=file.readAll();
    QString strData=QString(baData);
    qDebug()<<"strData"<<strData;
    QStringList strList=strData.split("\r\n");
    m_strIP=strList[0];
    m_usPort=strList[1].toUShort();
    m_strRootPath=strList[2];
    qDebug()<<"ip"<<m_strIP<<"Port"<<m_usPort<<"strRootPath"<<m_strRootPath;
    file.close();
}

