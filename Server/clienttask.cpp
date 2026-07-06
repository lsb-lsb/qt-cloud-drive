#include "clienttask.h"
#include "mytcpsocket.h"

#include <QTcpSocket>

ClientTask::ClientTask(MyTcpSocket* socket) : QObject(socket)
{
    m_socket=socket;
}

void ClientTask::run()
{
    connect(m_socket,&QTcpSocket::readyRead,m_socket,&MyTcpSocket::recvMsg);
    connect(m_socket,&QTcpSocket::disconnected,m_socket,&MyTcpSocket::clientOffline);
}
