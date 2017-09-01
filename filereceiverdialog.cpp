#include "filereceiverdialog.h"
#include "ui_filereceiverdialog.h"
#include <QTcpSocket>
#include <QDebug>
#include <QMessageBox>
#ifdef Q_OS_WIN
#define KSYSTEM_HEX 1024
#else
#define KSYSTEM_HEX 1000
#endif
FileReceiverDialog::FileReceiverDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileReceiverDialog)
{
    ui->setupUi(this);
    TotalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;

    tcpClient = new QTcpSocket(this);
    tcpPort = 6666;
    connect(tcpClient, SIGNAL(readyRead()), this, SLOT(readFile()));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}

FileReceiverDialog::~FileReceiverDialog()
{
    delete ui;
}

void FileReceiverDialog::on_cancelButton_clicked()
{
    tcpClient->abort();
    if (localFile->isOpen())
        localFile->close();
    close();
}
// 设置文件名
void FileReceiverDialog::setFileName(QString fileName)
{
    localFile = new QFile(fileName);
}

// 设置地址
void FileReceiverDialog::setHostAddress(QHostAddress address)
{
    hostAddress = address;
    newConnect();
}

// 创建新连接
void FileReceiverDialog::newConnect()
{
    blockSize = 0;
    tcpClient->abort();        //取消已有的连接
    tcpClient->connectToHost(hostAddress, tcpPort);//连接到指定ip地址和端口的主机
    time.start();
}

// 读取数据
void FileReceiverDialog::readFile()
{
    QDataStream in(tcpClient);    //这里的QDataStream可以直接用QTcpSocket对象做参数
    in.setVersion(QDataStream::Qt_5_9);

    float useTime = time.elapsed();

    if (bytesReceived <= sizeof(qint64)*2) {    //说明刚开始接受数据
        if ((tcpClient->bytesAvailable()    //bytesAvailable为返回将要被读取的字节数
             >= sizeof(qint64)*2) && (fileNameSize == 0))
        {
            //接受数据总大小信息和文件名大小信息
            in>>TotalBytes>>fileNameSize;
            bytesReceived += sizeof(qint64)*2;
        }
        if((tcpClient->bytesAvailable() >= fileNameSize) && (fileNameSize != 0)){
            //开始接受文件，并建立文件
            in>>fileName;
            bytesReceived +=fileNameSize;

            if(!localFile->open(QFile::WriteOnly)){
                QMessageBox::warning(this,tr("应用程序"),tr("无法读取文件 %1:\n%2.")
                                     .arg(fileName).arg(localFile->errorString()));
                return;
            }
        } else {
            return;
        }
    }
    if (bytesReceived < TotalBytes) {
        bytesReceived += tcpClient->bytesAvailable();//返回tcpClient中字节的总数
        inBlock = tcpClient->readAll();    //返回读到的所有数据
        localFile->write(inBlock);
        inBlock.resize(0);
    }
    ui->progressBar->setMaximum(TotalBytes);
    ui->progressBar->setValue(bytesReceived);

    double speed = bytesReceived / useTime;
    ui->titleLabel->setText(tr("已接收 %1MB (%2MB/s) "
                                         "\n共%3MB 已用时:%4秒\n估计剩余时间：%5秒")
                                      .arg(bytesReceived / (KSYSTEM_HEX*KSYSTEM_HEX))
                                      .arg(speed*1000/(KSYSTEM_HEX*KSYSTEM_HEX),0,'f',2)
                                      .arg(TotalBytes / (KSYSTEM_HEX * KSYSTEM_HEX))
                                      .arg(useTime/1000,0,'f',0)
                                      .arg(TotalBytes/speed/1000 - useTime/1000,0,'f',0));

    if(bytesReceived == TotalBytes)
    {
        localFile->close();
        tcpClient->close();
        ui->titleLabel->setText(tr("接收文件 %1 完毕")
                                          .arg(fileName));
        ui->cancelButton->setText("完成");
     }
}

// 错误处理
//QAbstractSocket类提供了所有scoket的通用功能，socketError为枚举型
void FileReceiverDialog::displayError(QAbstractSocket::SocketError socketError)
{
    switch(socketError)
    {
    //RemoteHostClosedError为远处主机关闭了连接时发出的错误信号
    case QAbstractSocket::RemoteHostClosedError : break;
    default : qDebug() << tcpClient->errorString();
    }
}



// 关闭事件
void FileReceiverDialog::closeEvent(QCloseEvent *)
{
    on_cancelButton_clicked();
}
