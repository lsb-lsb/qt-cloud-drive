#include "chat.h"
#include "client.h"
#include "protocol.h"
#include "ui_chat.h"

Chat::Chat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chat)
{
    ui->setupUi(this);
}

Chat::~Chat()
{
    delete ui;
}

void Chat::updateShow_TE(QString strMsg)
{
    ui->show_TE->append(strMsg);
}

void Chat::on_send_PB_clicked()
{
    QString strMsg=ui->input_LE->text();
    if(strMsg.isEmpty()){
        return;
    }
    ui->input_LE->clear();
    // 本地显示自己发送的消息
    ui->show_TE->append(QString("%1 : %2").arg(Client::getInstance().m_strLoginName).arg(strMsg));
    PDU* pdu=mkPDU(strMsg.toStdString().size()+1);
    pdu->uiType=ENUM_TYPE_CHAT_REQUEST;
    memcpy(pdu->caData,Client::getInstance().m_strLoginName.toStdString().c_str(),32);
    memcpy(pdu->caData+32,m_strChatName.toStdString().c_str(),32);
    memcpy(pdu->caMsg,strMsg.toStdString().c_str(),strMsg.toStdString().size());
    Client::getInstance().sendMsg(pdu);
}
