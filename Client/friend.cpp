#include "client.h"
#include "friend.h"
#include "protocol.h"
#include "ui_friend.h"

#include <QInputDialog>
#include <QMessageBox>

Friend::Friend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Friend)
{
    ui->setupUi(this);
    m_pOnlineUser=new OnlineUser;
    m_pChat=new Chat;
    flushFriend();
}

Friend::~Friend()
{
    delete ui;
    delete m_pOnlineUser;
    delete m_pChat;
}

void Friend::flushFriend()
{
    PDU* pdu=mkPDU();
    pdu->uiType=ENUM_TYPE_FLUSH_FRIEND_REQUEST;
    memcpy(pdu->caData,Client::getInstance().m_strLoginName.toStdString().c_str(),32);
    Client::getInstance().sendMsg(pdu);
}

void Friend::update_LW(QStringList friendList)
{
    ui->listWidget->clear();
    ui->listWidget->addItems(friendList);
}

void Friend::on_findUser_PB_clicked()
{
    QString strName=QInputDialog::getText(this,"提示","用户名：");
    if(strName.isEmpty()||strName.toStdString().size()>32){
        QMessageBox::warning(this,"提示","长度非法");
        return;
    }
    PDU* pdu=mkPDU();
    memcpy(pdu->caData,strName.toStdString().c_str(),32);
    pdu->uiType=ENUM_TYPE_FIND_USER_REQUEST;
    Client::getInstance().sendMsg(pdu);
}

void Friend::on_onlineUser_PB_clicked()
{
    if(m_pOnlineUser->isHidden()){
        m_pOnlineUser->show();
    }
    PDU* pdu=mkPDU();
    pdu->uiType=ENUM_TYPE_ONLINE_USER_REQUEST;
    Client::getInstance().sendMsg(pdu);
}

void Friend::on_flush_PB_clicked()
{
    flushFriend();
}

void Friend::on_del_PB_clicked()
{
    QListWidgetItem* pItem=ui->listWidget->currentItem();
    if(!pItem){
        return;
    }
    QString strTarName=pItem->text();
    int ret=QMessageBox::question(this,"删除好友",QString("是否确定删除好友 %1").arg(strTarName));
    if(ret!=QMessageBox::Yes){
        return;
    }
    PDU* pdu=mkPDU(0);
    pdu->uiType=ENUM_TYPE_DEL_FRIEND_REQUEST;
    QString strCurName=Client::getInstance().m_strLoginName;
    memcpy(pdu->caData,strCurName.toStdString().c_str(),strCurName.size());
    memcpy(pdu->caData+32,strTarName.toStdString().c_str(),strTarName.size());
    Client::getInstance().sendMsg(pdu);
}

void Friend::on_chat_PB_clicked()
{
    QListWidgetItem* pItem=ui->listWidget->currentItem();
    if(!pItem){
        return;
    }
    if(m_pChat->isHidden()){
        m_pChat->show();
    }
    m_pChat->m_strChatName=pItem->text();
    m_pChat->setWindowTitle(QString("与 %1 聊天中").arg(pItem->text()));
}
