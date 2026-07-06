#ifndef CLIENT_H
#define CLIENT_H

#include <QTcpSocket>
#include <QWidget>
#include "protocol.h"
#include "reshandler.h"
#include "ui_client.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QWidget
{
    Q_OBJECT

public:

    ~Client();
    void loadConfig();
    static Client& getInstance();
    PDU* readMsg();
    void handleMsg(PDU* pdu);
    QString m_strIP;
    quint16 m_usPort;
    QString m_strRootPath;
    QTcpSocket m_socket;
    QString m_strLoginName;
    ResHandler* m_prh;
    QByteArray buffer;
public slots:
    void showConnect();
    void recvMsg();
    void sendMsg(PDU* pdu);

private slots:
 //   void on_send_PB_clicked();


    void on_regist_PB_clicked();

    void on_login_PB_clicked();

private:
    Ui::Client *ui;
    Client(QWidget *parent = nullptr);
    Client(const Client& instance)=delete;
    Client& operator=(const Client&)=delete;
};
#endif // CLIENT_H
