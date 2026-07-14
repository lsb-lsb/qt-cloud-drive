#include "client.h"
#include "file.h"
#include "ui_file.h"
#include "uploader.h"

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <qinputdialog.h>

File::File(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::File)
{
    ui->setupUi(this);
    m_strUserPath=QString("%1/%2").arg(Client::getInstance().m_strRootPath).arg(Client::getInstance().m_strLoginName);
    m_strCurPath=m_strUserPath;
    flushFile();
}

File::~File()
{
    delete ui;
}

// 刷新文件列表
void File::flushFile()
{
    qDebug()<<"File::flushFile() m_strCurPath:"<<m_strCurPath;
    PDU* pdu=mkPDU(m_strCurPath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_FLUSH_FILE_REQUEST;
    memcpy(pdu->caMsg,m_strCurPath.toUtf8().constData(),m_strCurPath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);
}

// 更新文件列表UI
void File::updateFileList(QList<FileInfo *> pFileInfoList)
{
    qDebug()<<"File::updateFileList() count:"<<pFileInfoList.size();
    foreach(FileInfo* pFileInfo,m_pFileInfoList){
        delete pFileInfo;
    }
    m_pFileInfoList.clear();
    m_pFileInfoList=pFileInfoList;

    ui->listWidget->clear();
    foreach(FileInfo* pFileInfo,pFileInfoList){
        qDebug()<<"  file:"<<pFileInfo->caName<<" type:"<<pFileInfo->iFileType;
        QListWidgetItem* pItem=new QListWidgetItem;
        if(pFileInfo->iFileType==0){
            pItem->setIcon(QIcon(QPixmap(":/dir.png")));
        }else{
            pItem->setIcon(QIcon(QPixmap(":/file.png")));
        }
        pItem->setText(QString::fromUtf8(pFileInfo->caName));
        ui->listWidget->addItem(pItem);
    }
}

void File::uploadFile()
{
    Uploader* uploader=new Uploader(m_strUploadFilePath);
    connect(uploader,&Uploader::errorMsg,this,&File::uploadErrorBox,Qt::QueuedConnection);
    connect(uploader,&Uploader::uploadPDU,&Client::getInstance(),&Client::sendMsg ,Qt::QueuedConnection);
    connect(uploader,&Uploader::finished,uploader,&QObject::deleteLater);

    uploader->start();

}

void File::uploadErrorBox(const QString &msg)
{
    QMessageBox::information(this,"提示",msg);

}

// 创建文件夹
void File::on_mkdir_PB_clicked()
{
    QString strDirName=QInputDialog::getText(this,"新建文件夹","新建文件夹名：");
    if(strDirName.isEmpty()||strDirName.toUtf8().size()>32){
        QMessageBox::information(this,"提示","文件夹名长度非法");
        return;
    }
    PDU* pdu=mkPDU(m_strCurPath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_MKDIR_REQUEST;
    memcpy(pdu->caData,strDirName.toUtf8().constData(),32);
    memcpy(pdu->caMsg,m_strCurPath.toUtf8().constData(),m_strCurPath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);

}

void File::on_flush_PB_clicked()
{
    flushFile();
}

// 删除文件
void File::on_del_PB_clicked()
{
    QListWidgetItem* pItem=ui->listWidget->currentItem();
    if(!pItem){
        return;
    }
    int ret=QMessageBox::question(this,"删除文件",QString("是否确认删除文件：%1?").arg(pItem->text()));
    if(ret!=QMessageBox::Yes){
        return;
    }
    QString strPath=QString("%1/%2").arg(m_strCurPath).arg(pItem->text());
    PDU* pdu=mkPDU(strPath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_DEL_FILE_REQUEST;
    memcpy(pdu->caMsg,strPath.toUtf8().constData(),strPath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);

}

// 重命名文件
void File::on_rename_PB_clicked()
{
    QListWidgetItem* pItem=ui->listWidget->currentItem();
    if(!pItem){
        return;
    }
    QString strNewName= QInputDialog::getText(this,"重命名","新文件名：");
    if(strNewName.isEmpty()||strNewName.toUtf8().size()>32){
        QMessageBox::information(this,"提示","新文件名长度非法");
        return;
    }
    PDU* pdu=mkPDU(m_strCurPath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_RENAME_FILE_REQUEST;
    memcpy(pdu->caData,pItem->text().toUtf8().constData(),32);
    memcpy(pdu->caData+32,strNewName.toUtf8().constData(),32);
    memcpy(pdu->caMsg,m_strCurPath.toUtf8().constData(),m_strCurPath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);
}

void File::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    foreach(FileInfo* pFileInfo,m_pFileInfoList){
        if(QString::fromUtf8(pFileInfo->caName)==item->text()&&pFileInfo->iFileType!=0){
            return;
        }
    }
    m_strCurPath=QString("%1/%2").arg(m_strCurPath).arg(item->text());
    flushFile();
}

// 返回上级目录
void File::on_return_PB_clicked()
{
    if(m_strCurPath==m_strUserPath){
        return;
    }
    int index=m_strCurPath.lastIndexOf('/');
    m_strCurPath.remove(index,m_strCurPath.size()-index);
    flushFile();
}

// 上传文件
void File::on_upload_PB_clicked()
{
    m_strUploadFilePath.clear();
    m_strUploadFilePath=QFileDialog::getOpenFileName();
    qDebug()<<"m_strUploadFilePath"<<m_strUploadFilePath;
    if(m_strUploadFilePath.isEmpty()){
        return;
    }
    QFileInfo fileInfo(m_strUploadFilePath);
    QString strFileName = fileInfo.fileName();
    qint64 iFileSize = fileInfo.size();
    PDU* pdu=mkPDU(m_strCurPath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_UPLOAD_FILE_INIT_REQUEST;
    memcpy(pdu->caData,strFileName.toUtf8().constData(),32);
    memcpy(pdu->caData+32,&iFileSize,sizeof (qint64));
    memcpy(pdu->caMsg,m_strCurPath.toUtf8().constData(),m_strCurPath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);
}

// 下载文件
void File::on_download_PB_clicked()
{
    QListWidgetItem* pItem=ui->listWidget->currentItem();
    if(!pItem){
        QMessageBox::information(this,"提示","请先选中一个文件");
        return;
    }
    foreach(FileInfo* pFileInfo,m_pFileInfoList){
        if(QString::fromUtf8(pFileInfo->caName)==pItem->text()&&pFileInfo->iFileType==0){
            QMessageBox::information(this,"提示","不能下载文件夹");
            return;
        }
    }
    QString strFilePath=QString("%1/%2").arg(m_strCurPath).arg(pItem->text());
    qDebug()<<"download strFilePath:"<<strFilePath;
    PDU* pdu=mkPDU(strFilePath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_DOWNLOAD_FILE_REQUEST;
    memcpy(pdu->caMsg,strFilePath.toUtf8().constData(),strFilePath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);
}

// 分享文件
void File::on_share_PB_clicked()
{
    QListWidgetItem* pItem=ui->listWidget->currentItem();
    if(!pItem){
        QMessageBox::information(this,"提示","请先选中一个文件");
        return;
    }
    foreach(FileInfo* pFileInfo,m_pFileInfoList){
        if(QString::fromUtf8(pFileInfo->caName)==pItem->text()&&pFileInfo->iFileType==0){
            QMessageBox::information(this,"提示","不能分享文件夹");
            return;
        }
    }
    QString strTarName=QInputDialog::getText(this,"分享文件","请输入要分享的好友用户名：");
    if(strTarName.isEmpty()||strTarName.toUtf8().size()>32){
        QMessageBox::information(this,"提示","用户名长度非法");
        return;
    }
    QString strFilePath=QString("%1/%2").arg(m_strCurPath).arg(pItem->text());
    qDebug()<<"share strFilePath:"<<strFilePath<<"tarName:"<<strTarName;
    PDU* pdu=mkPDU(strFilePath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_SHARE_FILE_REQUEST;
    memcpy(pdu->caData,strTarName.toUtf8().constData(),32);
    memcpy(pdu->caData+32,pItem->text().toUtf8().constData(),32);
    memcpy(pdu->caMsg,strFilePath.toUtf8().constData(),strFilePath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);
}

// 安全检查
void File::on_securityScan_PB_clicked()
{
    int ret=QMessageBox::question(this,"安全检查","是否对云盘内所有文件进行安全检查？\n\n扫描内容包括:\n· 文件名安全性\n· 扩展名黑名单\n· 文件大小限制\n· 文件头魔数校验\n· AI 敏感信息深度扫描");
    if(ret!=QMessageBox::Yes){
        return;
    }
    PDU* pdu=mkPDU(m_strUserPath.toUtf8().size()+1);
    pdu->uiType=ENUM_TYPE_SECURITY_SCAN_REQUEST;
    memcpy(pdu->caMsg,m_strUserPath.toUtf8().constData(),m_strUserPath.toUtf8().size());
    Client::getInstance().sendMsg(pdu);
}
