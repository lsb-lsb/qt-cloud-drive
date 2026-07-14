 #ifndef MSGHANDLER_H
#define MSGHANDLER_H

#include "protocol.h"

#include <QFile>
#include <QString>



class MsgHandler
{
public:
    PDU* pdu;
    qint64 m_iUploadFileSize;
    qint64 m_iUploadFileReceived;
    QFile m_fUploadFile;
    QString m_strUploadFileName;
    QString m_strUploadDirPath;

    MsgHandler();
    PDU* regist();
    PDU* login(QString& strLoginName);
    PDU* findUser();
    PDU* onlineUser();
    PDU* addFriend();
    PDU* addFriendAgree();
    PDU* flushFriend();
    PDU* delFriend();
    PDU* chat();
    PDU* mkdir();
    PDU* flushFile();
    PDU* delFile();
    PDU* renameFile();
    PDU* uploadFileInit();
    PDU* uploadFileData();
    PDU* downloadFile();
    PDU* shareFile();

    PDU* securityScan();
    PDU* unsafeFileDel();

    PDU* aiAnalyze();
    PDU* aiReport();

    PDU* makeUnsafeNotify(const QString& strReason);
};

#endif
