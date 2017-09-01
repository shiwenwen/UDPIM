#include "filesenderdialog.h"
#include "ui_filesenderdialog.h"
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#ifdef Q_OS_WIN
#define KSYSTEM_HEX 1024
#else
#define KSYSTEM_HEX 1000
#endif

FileSenderDialog::FileSenderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileSenderDialog)
{
    qDebug() << KSYSTEM_HEX;
    ui->setupUi(this);
    tcpPort = 6666;        //tcp通信端口
    tcpServer = new QTcpServer(this);
    //newConnection表示当tcp有新连接时就发送信号
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(sendFile()));

    initServer();
}
FileSenderDialog::~FileSenderDialog()
{
    delete ui;
}

void FileSenderDialog::initServer()
{
    payloadSize = 64*KSYSTEM_HEX;
    TotalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;

    ui->titleLabel->setText(tr("请选择要传送的文件"));
    ui->progressBar->reset();//进度条复位
    ui->progressBar->setHidden(true);
    ui->openFileButton->setEnabled(true);//open按钮可用
    ui->sendButton->setEnabled(false);//发送按钮不可用
    ui->cancelButton->setText("取消");
    tcpServer->close();//tcp传送文件窗口不显示
}


void FileSenderDialog::sendFile()
{
    ui->progressBar->setHidden(false);
    ui->sendButton->setEnabled(false);    //当在传送文件的过程中，发送按钮不可用
    clientConnection = tcpServer->nextPendingConnection();    //用来获取一个已连接的TcpSocket
    //bytesWritten为qint64类型，即长整型
    connect(clientConnection, SIGNAL(bytesWritten(qint64)),    //?
            this, SLOT(updateClientProgress(qint64)));
    connect(clientConnection, SIGNAL(readChannelFinished()),    //?
            this, SLOT(readChannelFinished()));
    ui->titleLabel->setText(tr("开始传送文件 %1 ！").arg(fileName));

    localFile = new QFile(filePath);    //localFile代表的是文件内容本身
    if(!localFile->open((QFile::ReadOnly))){
        QMessageBox::warning(this, tr("应用程序"), tr("无法读取文件 %1:\n%2")
                             .arg(filePath).arg(localFile->errorString()));//errorString是系统自带的信息
        return;
    }
    TotalBytes = localFile->size();//文件总大小
    //头文件中的定义QByteArray outBlock;
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);//设置输出流属性
    sendOut.setVersion(QDataStream::Qt_5_9);//设置Qt版本，不同版本的数据流格式不同
    time.start();  // 开始计时
    QString currentFile = filePath.right(filePath.size()    //currentFile代表所选文件的文件名
                                         - filePath.lastIndexOf('/')-1);
    //qint64(0)表示将0转换成qint64类型,与(qint64)0等价
    //如果是，则此处为依次写入总大小信息空间，文件名大小信息空间，文件名
    sendOut << qint64(0) << qint64(0) << currentFile;
    TotalBytes += outBlock.size();//文件名大小等信息+实际文件大小
    //sendOut.device()为返回io设备的当前设置，seek(0)表示设置当前pos为0
    sendOut.device()->seek(0);//返回到outBlock的开始，执行覆盖操作
    //发送总大小空间和文件名大小空间
    sendOut << TotalBytes << qint64((outBlock.size() - sizeof(qint64)*2));
    //qint64 bytesWritten;bytesToWrite表示还剩下的没发送完的数据
    //clientConnection->write(outBlock)为套接字将内容发送出去，返回实际发送出去的字节数
    clientConnection->write(outBlock);
    bytesToWrite = TotalBytes - outBlock.size();
    outBlock.resize(0);
}

void FileSenderDialog::updateClientProgress(qint64 numBytes)
{
    //qApp为指向一个应用对象的全局指针
    qApp->processEvents();  //强制刷新
    bytesWritten += (int)numBytes;
    if (bytesToWrite > 0) {    //没发送完毕
        //初始化时payloadSize = 64*KSYSTEM_HEX;qMin为返回参数中较小的值，每次最多发送64K的大小
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));
        clientConnection->write(outBlock);
        bytesToWrite -= outBlock.size();
        outBlock.resize(0);//清空发送缓冲区
    } else {
        localFile->close();
    }
    ui->progressBar->setMaximum(TotalBytes);//进度条的最大值为所发送信息的所有长度(包括附加信息)
    ui->progressBar->setValue(bytesWritten);//进度条显示的进度长度为bytesWritten实时的长度

    float useTime = time.elapsed();//从time.start()到当前所用的时间记录在useTime中
    double speed = bytesWritten / useTime;
    ui->titleLabel->setText(tr("已发送 %1MB (%2MB/s) "
                   "\n共%3MB 已用时:%4秒\n估计剩余时间：%5秒")
                   .arg(bytesWritten / (KSYSTEM_HEX*KSYSTEM_HEX))    //转化成MB
                   .arg(speed*1000 / (KSYSTEM_HEX*KSYSTEM_HEX), 0, 'f', 2)
                   .arg(TotalBytes / (KSYSTEM_HEX * KSYSTEM_HEX))
                   .arg(useTime/1000, 0, 'f', 0)
                   .arg(TotalBytes/speed/1000 - useTime/1000, 0, 'f', 0));

    if(bytesWritten == TotalBytes) {    //当需发送文件的总长度等于已发送长度时，表示发送完毕！
        localFile->close();
        tcpServer->close();
        ui->titleLabel->setText(tr("传送文件 %1 成功").arg(fileName));
        ui->cancelButton->setText("完成");
    }
}

void FileSenderDialog::on_openFileButton_clicked()
{
    //QString fileName;QFileDialog是一个提供给用户选择文件或目录的对话框
    initServer();
    filePath = QFileDialog::getOpenFileName(this);    //filename为所选择的文件名(包含了路径名)
    if(!filePath.isEmpty())
    {
        //fileName.right为返回filename最右边参数大小个字文件名，theFileName为所选真正的文件名
        fileName = filePath.right(filePath.size() - filePath.lastIndexOf('/')-1);
        ui->titleLabel->setText(tr("要传送的文件为：%1 ").arg(fileName));
        ui->sendButton->setEnabled(true);//发送按钮可用
//        ui->openFileButton->setEnabled(false);//open按钮禁用
    }
}

void FileSenderDialog::on_sendButton_clicked()
{
    //tcpServer->listen函数如果监听到有连接，则返回1，否则返回0
    if(!tcpServer->listen(QHostAddress::Any,tcpPort))//开始监听6666端口
    {
        qDebug() << tcpServer->errorString();//此处的errorString是指？
        close();
        return;
    }

    ui->titleLabel->setText(tr("等待对方接收... ..."));
    emit sendFileName(fileName);//发送已传送文件的信号，在widget.cpp构造函数中的connect()触发槽函数
}

void FileSenderDialog::on_cancelButton_clicked()
{
    if(tcpServer->isListening())
    {
        //当tcp正在监听时，关闭tcp服务器端应用，即按下close键时就不监听tcp请求了
        tcpServer->close();
        if (localFile->isOpen())//如果所选择的文件已经打开，则关闭掉
            localFile->close();
        clientConnection->abort();
    }
    close();//关闭本ui，即本对话框
}

void FileSenderDialog::readChannelFinished()
{
    localFile->close();
    tcpServer->close();
    ui->progressBar->setValue(TotalBytes);
    ui->titleLabel->setText(tr("传送文件 %1 成功").arg(fileName));
    ui->cancelButton->setText("完成");
}
void FileSenderDialog::refused()
{
    tcpServer->close();
    ui->titleLabel->setText(tr("对方拒绝接收！！！"));
}

void FileSenderDialog::closeEvent(QCloseEvent *)
{
    on_cancelButton_clicked();
}
