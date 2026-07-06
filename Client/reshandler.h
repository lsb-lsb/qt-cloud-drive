#ifndef RESHANDLER_H
#define RESHANDLER_H

#include "protocol.h"



class ResHandler
{
public:
    PDU* pdu;
    ResHandler();
    void regist();
    void login();
    void findUser();
    void onlineUser();
    void addFriend();
    void addFriendResend();
    void addFriendAgree();
    void flushFriend();
    void delFriend();
    void chat();
    void mkdir();
    void flushFile();
    void delFile();
    void renameFile();
    void uploadFileInit();
    void uploadFileData();
    void downloadFile();
    void shareFile();

};

#endif // RESHANDLER_H
