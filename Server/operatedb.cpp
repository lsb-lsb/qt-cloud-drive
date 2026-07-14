#include "operatedb.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
OperateDB &OperateDB::getInstance()
{
    static OperateDB instance;
    return instance;
}

// 连接MySQL数据库
void OperateDB::connectSQL()
{
    m_db.setHostName("localhost");
    m_db.setPort(3306);
    m_db.setUserName("root");
    m_db.setPassword("123456");
    m_db.setDatabaseName("lsb");
    if(m_db.open()){
        qDebug()<<"数据库连接成功";
    }else{
        qDebug()<<"数据库连接失败"<<m_db.lastError().text();
    }
}

OperateDB::~OperateDB()
{
    m_db.close();
}

// 处理注册
bool OperateDB::handleRegist(const char *caName, const char *caPwd)
{
    if(caName==NULL||caPwd==NULL){
        return false;
    }
    QString strName = QString::fromUtf8(caName);
    QString strPwd = QString::fromUtf8(caPwd);

    QSqlQuery q;
    q.prepare("select * from user_info where name=?");
    q.addBindValue(strName);
    qDebug()<<"查找用户是否存在";
    if(!q.exec() || q.next()){
        return false;
    }

    q.prepare("insert into user_info(name,pwd) values(?,?)");
    q.addBindValue(strName);
    q.addBindValue(strPwd);
    qDebug()<<"插入一条用户记录";
    return q.exec();
}

// 处理登录
bool OperateDB::handleLogin(const char *caName, const char *caPwd)
{
     if(caName==NULL||caPwd==NULL){
        return false;
    }
    QString strName = QString::fromUtf8(caName);
    QString strPwd = QString::fromUtf8(caPwd);

    QSqlQuery q;
    q.prepare("select * from user_info where name=? and pwd=?");
    q.addBindValue(strName);
    q.addBindValue(strPwd);
    qDebug()<<"查找用户名和密码是否存在";
    if(!q.exec() || !q.next()){
        return false;
    }

    q.prepare("update user_info set online=1 where name=?");
    q.addBindValue(strName);
    qDebug()<<"将用户的online字段置为1";
    return q.exec();
}

// 处理离线
void OperateDB::handleOffline(const char *caName)
{
    if(caName==NULL){
       return;
   }
    QString strName = QString::fromUtf8(caName);

    QSqlQuery q;
    q.prepare("update user_info set online=0 where name=?");
    q.addBindValue(strName);
    qDebug()<<"将用户的online字段置为0";
    bool ret=q.exec();
    qDebug()<<"handleOffline ret"<<ret;
}

int OperateDB::handleFindUser(const char *caName)
{
    if(caName==NULL){
       return -1;
   }
    QString strName = QString::fromUtf8(caName);

    QSqlQuery q;
    q.prepare("select online from user_info where name=?");
    q.addBindValue(strName);
    qDebug()<<"查找用户的online字段";
    if(!q.exec()){
        return -1;
    }
    if(q.next()){
        return q.value(0).toInt();
    }
    return 2;
}

QStringList OperateDB::handleOnlineUser()
{
    QSqlQuery q;
    q.exec("select name from user_info where online=1");
    QStringList res;
    while(q.next()){
        res.append(q.value(0).toString());
    }
    return res;
}

int OperateDB::handleAddFriend(const char *caCurName, const char *caTarName)
{
    if(caCurName==NULL||caTarName==NULL){
        return -1;
    }
    QString strCur = QString::fromUtf8(caCurName);
    QString strTar = QString::fromUtf8(caTarName);

    QSqlQuery q;
    q.prepare(R"(
        select * from friend where
        (
          user_id=(select id from user_info where name=?)
          and
          friend_id=(select id from user_info where name=?)
        )
        or
        (
          user_id=(select id from user_info where name=?)
          and
          friend_id=(select id from user_info where name=?)
        )
    )");
    q.addBindValue(strCur);
    q.addBindValue(strTar);
    q.addBindValue(strTar);
    q.addBindValue(strCur);
    qDebug()<<"判断是否是好友";
    q.exec();
    if(q.next()){
        return -2;
    }

    q.prepare("select online from user_info where name=?");
    q.addBindValue(strTar);
    q.exec();
    if(q.next()){
        return q.value(0).toInt();
    }
    return -1;
}

bool OperateDB::handleAddFriendAgree(const char *caCurName, const char *caTarName)
{
    if(caCurName==NULL||caTarName==NULL){
        return false;
    }
    QString strCur = QString::fromUtf8(caCurName);
    QString strTar = QString::fromUtf8(caTarName);

    QSqlQuery q;
    q.prepare(R"(
        insert into friend(user_id, friend_id)
        select u1.id, u2.id
        from user_info u1, user_info u2
        where u1.name=? and u2.name=?;
    )");
    q.addBindValue(strCur);
    q.addBindValue(strTar);
    return q.exec();
}

QStringList OperateDB::handleFlushFriend(const char *caCurName)
{
    QStringList res;
    if(caCurName==NULL){
        return res;
    }
    QString strCur = QString::fromUtf8(caCurName);

    QSqlQuery q;
    q.prepare(R"(
        select name from user_info
        where id in(
        select user_id from friend where friend_id=(select id from user_info where name=?)
        union
        select friend_id from friend where user_id=(select id from user_info where name=?)
        )and online =1;
    )");
    q.addBindValue(strCur);
    q.addBindValue(strCur);
    q.exec();
    while(q.next()){
        res.append(q.value(0).toString());
    }
    return res;
}


bool OperateDB::handleDelFriend(const char *caCurName, const char *caTarName)
{
    if(caCurName==NULL||caTarName==NULL){
        return false;
    }
    QString strCur = QString::fromUtf8(caCurName);
    QString strTar = QString::fromUtf8(caTarName);

    QSqlQuery q;
    q.prepare(R"(
        delete from friend where
        (
        user_id=(select id from user_info where name=?)
        and
        friend_id=(select id from user_info where name=?)
        )
        or
        (
        friend_id=(select id from user_info where name=?)
        and
        user_id=(select id from user_info where name=?)
        );
    )");
    q.addBindValue(strCur);
    q.addBindValue(strTar);
    q.addBindValue(strCur);
    q.addBindValue(strTar);
    return q.exec();
}


OperateDB::OperateDB(QObject *parent) : QObject(parent)
{
    m_db=QSqlDatabase::addDatabase("QMYSQL");
}
