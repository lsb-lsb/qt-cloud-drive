#include "client.h"
#include "onlineuser.h"
#include "ui_onlineuser.h"

OnlineUser::OnlineUser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OnlineUser)
{
    ui->setupUi(this);
}

OnlineUser::~OnlineUser()
{
    delete ui;
}

void OnlineUser::updateLW(QStringList userList)
{
    ui->listWidget->clear();
    ui->listWidget->addItems(userList);
}



// 双击添加好友
void OnlineUser::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString strCurName=Client::getInstance().m_strLoginName;
    QString strTarName=item->text();
    PDU* pdu=mkPDU();
    pdu->uiType=ENUM_TYPE_ADD_FRIEND_REQUEST;
    memcpy(pdu->caData,strCurName.toUtf8().constData(),32);
    memcpy(pdu->caData+32,strTarName.toUtf8().constData(),32);
    Client::getInstance().sendMsg(pdu);
}
