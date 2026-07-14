#include "aichecker.h"
#include "mytcpserver.h"
#include "server.h"

#include <QFile>
#include <QDebug>
#include <QRegExp>
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

// 加载配置文件
void Server::loadConfig()
{
    QFile file(":/connect.config");
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"Cannot open config";
        return;
    }
    QByteArray baData=file.readAll();
    QString strData=QString(baData);
    qDebug()<<"strData"<<strData;
    QStringList strList=strData.split(QRegExp("[\r\n]+"), QString::SkipEmptyParts);
    if (strList.size() < 3) {
        qDebug() << "Config file has fewer than 3 lines, using defaults";
        return;
    }
    m_strIP=strList[0];
    m_usPort=strList[1].toUShort();
    m_strRootPath=strList[2];
    if (strList.size() >= 4 && !strList[3].isEmpty()) {
        AIChecker::getInstance().setApiKey(strList[3].trimmed());
    }
    qDebug()<<"ip"<<m_strIP<<"Port"<<m_usPort<<"strRootPath"<<m_strRootPath;
    file.close();
}

