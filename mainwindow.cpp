#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QKeyEvent>
#include <QSound>
#include <QCoreApplication>
#include <QHostInfo>
#include "filesenderdialog.h"
#include <QFileDialog>
#include "filereceiverdialog.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    currentSessionIp = "";
    ui->setupUi(this);
    ui->textEdit->installEventFilter(this); //监听键盘
    initIM(); //初始化IM
       ui->selfIpLabel->setText("本机ip:" + imTool->getIP());
    //listWidget点击信号插槽
    QListWidget *listWidget = ui->listWidget;
    connect(listWidget,SIGNAL(clicked(QModelIndex)),this,SLOT(listClicked(QModelIndex)));

    filesenderdialog = new FileSenderDialog(this);
    connect(filesenderdialog, SIGNAL(sendFileName(QString)), this, SLOT(sendFileName(QString)));
}

MainWindow::~MainWindow()
{
//    imTool->saveTextFile(currentSessionIp,ui->textBrowser->toPlainText());

    delete ui;
    delete imTool;
    delete filesenderdialog;
}
void MainWindow::initIM()
{
    imTool = new IMTool();
    imTool->login();

    //信号槽
    connect(imTool,SIGNAL(findNewClient(QString,QString,QString)),this,SLOT(findNewClient(QString,QString,QString)));
    connect(imTool,SIGNAL(disconnectClient(QString,QString,QString)),this,SLOT(disconnectClient(QString,QString,QString)));
    connect(imTool,SIGNAL(receiveNewMessage(QString,QString,QString,QString,QString)),this,SLOT(receiveNewMessage(QString,QString,QString,QString,QString)));
    connect(imTool,SIGNAL(receiveNewFile(QString,QString,QString,QString,QString,QString)),this,SLOT(receiveNewFile(QString,QString,QString,QString,QString,QString)));
}
void MainWindow::findNewClient(QString userName, QString localHostName, QString ipAddress){
    qDebug() << userName + "登录";

    //检查列表是否已经有了
    bool isConnected = !(ui->listWidget->findItems(ipAddress,Qt::MatchContains).isEmpty());
    if(isConnected) {
        //如果已经有了 不做任何处理
        return;
    }
    imTool->login();//告诉新客户端我也是登录的
    //列表添加新用户
    QString itemStr = userName + " : " + ipAddress;
    ui->listWidget->addItem(new QListWidgetItem(itemStr));
    ui->countLabel->setText(QString("在线人数:%1").arg(ui->listWidget->count()));

}
void MainWindow::disconnectClient(QString userName, QString localHostName, QString ipAddress){
    qDebug() << userName + "退出";
    //如果退出的是当前会话用户 保存聊天记录 进行处理
    QList<QListWidgetItem*> items = ui->listWidget->findItems(ipAddress,Qt::MatchContains);
    foreach (QListWidgetItem *item, items) {
        if (item->text().contains(currentSessionIp)) {
            currentSessionIp = "";
            ui->currentSessionLabel->setText("暂无会话");
            ui->textBrowser->setText("");
        }
        ui->listWidget->removeItemWidget(item);
        delete item;
    }
    //更新在线人数
    ui->countLabel->setText(QString("在线人数:%1").arg(ui->listWidget->count()));
    if (ui->listWidget->count() == 0) {
        ui->currentSessionLabel->setText("暂无会话");
    }
}
void MainWindow::receiveNewMessage(QString userName, QString localHostName, QString ipAddress, QString message, QString timeStr){
    qDebug() << userName + "发来消息:" + message;
    QSound::play("./sound.wav");
    QList<QListWidgetItem*> items = ui->listWidget->findItems(ipAddress,Qt::MatchContains);
    bool isInList = items.length() > 0;
    if (!isInList) {
        //列表添加新用户
        QString itemStr = userName + " : " + ipAddress;
        QListWidgetItem *item = new QListWidgetItem(itemStr);
        ui->listWidget->addItem(item);
        items.append(item);
        ui->countLabel->setText(QString("在线人数:%1").arg(ui->listWidget->count()));

    }

    foreach (QListWidgetItem *item, items ) {
        if ( currentSessionIp == "") {
            //如果当前没有任何进行中会话则自动开始会话
            if(item->text().contains("[新消息]")){
                item->setText(item->text().mid(5,item->text().length() - 5));
            }
            currentSessionIp = ipAddress;
            item->setSelected(true);
            ui->currentSessionLabel->setText(QString("当前会话:") + item->text());
            ui->textBrowser->setText(imTool->readAllMessag(ipAddress));
        }
    }


    if (currentSessionIp == ipAddress ) {
        //如果是当前会话就显示
        ui->textBrowser->append(userName + "[" + ipAddress + "]-" + timeStr);
        ui->textBrowser->append(message);
        ui->textBrowser->append("");
        ui->textBrowser->moveCursor(QTextCursor::End); //滚动到最低端
    }else{
        //保存记录 提示有未读消息
        foreach (QListWidgetItem *item, items ) {
            if (item->text().contains(ipAddress)) {
              if (!item->text().contains("[新消息]")) {
                item->setText(QString("[新消息]") + item->text());
              }

            }
        }
    }
    imTool->insertMessage(ipAddress,userName,localHostName,message,timeStr,false);


}
void MainWindow::receiveNewFile(QString userName,QString localHostName, QString serverAddress, QString clientAddress, QString fileName, QString timeStr){
    QString ipAddress = imTool->getIP();
    if(ipAddress == clientAddress)
    {
        QSound::play("./sound.wav");
        QList<QListWidgetItem*> items = ui->listWidget->findItems(serverAddress,Qt::MatchContains);
        bool isInList = items.length() > 0;
        if (!isInList) {
            //列表添加新用户
            QString itemStr = userName + " : " + ipAddress;
            QListWidgetItem *item = new QListWidgetItem(itemStr);
            ui->listWidget->addItem(item);
            items.append(item);
            ui->countLabel->setText(QString("在线人数:%1").arg(ui->listWidget->count()));

        }

        int btn = QMessageBox::information(this,tr("接受文件"),
                                           tr("来自%1(%2)的文件：%3,是否接收？")
                                           .arg(userName).arg(serverAddress).arg(fileName),
                                           QMessageBox::Yes,QMessageBox::No);//弹出一个窗口
        if (btn == QMessageBox::Yes) {
            QString name = QFileDialog::getSaveFileName(0,"保存文件",fileName);//name为另存为的文件名
            if(!name.isEmpty())
            {
                FileReceiverDialog *receive = new FileReceiverDialog(this);
                receive->setFileName(name);    //客户端设置文件名
                receive->setHostAddress(QHostAddress(serverAddress));    //客户端设置服务器地址
                receive->show();
                QString messageContent = "【收到文件】：" + fileName + "(已保存)";
                if (currentSessionIp == serverAddress ) {
                    //如果是当前会话就显示
                    ui->textBrowser->append(userName + "[" + serverAddress + "]-" + timeStr);
                    ui->textBrowser->append(messageContent);
                    ui->textBrowser->append("");
                    ui->textBrowser->moveCursor(QTextCursor::End); //滚动到最低端
                }
                imTool->insertMessage(serverAddress,userName,localHostName,messageContent,timeStr,false);
            }else{
                QString messageContent = "【收到文件】：" + fileName + "(未保存)";
                imTool->sendMessage(IMTool::Refuse,serverAddress);
                if (currentSessionIp == serverAddress ) {
                    //如果是当前会话就显示
                    ui->textBrowser->append(userName + "[" + serverAddress + "]-" + timeStr);

                    ui->textBrowser->append(messageContent);
                    ui->textBrowser->append("");
                    ui->textBrowser->moveCursor(QTextCursor::End); //滚动到最低端
                }
                imTool->insertMessage(serverAddress,userName,localHostName,messageContent,timeStr,false);
            }
        } else {
            imTool->sendMessage(IMTool::Refuse, serverAddress);
            QString messageContent = "【收到文件】：" + fileName + "(已拒绝)";
            if (currentSessionIp == serverAddress ) {
                //如果是当前会话就显示
                ui->textBrowser->append(userName + "[" + serverAddress + "]-" + timeStr);

                ui->textBrowser->append(messageContent);
                ui->textBrowser->append("");
                ui->textBrowser->moveCursor(QTextCursor::End); //滚动到最低端
            }
            imTool->insertMessage(serverAddress,userName,localHostName,messageContent,timeStr,false);

        }
    }
}

void MainWindow::listClicked(QModelIndex index){
    QListWidgetItem *item = ui->listWidget->item(index.row());
    qDebug() << item->text();
    //如果有未读消息 清除提示
    if(item->text().contains("[新消息]")){
        item->setText(item->text().mid(5,item->text().length() - 5));
    }

    QString itemStr = item->text();
    QString ip = itemStr.mid(itemStr.indexOf(":")+1).trimmed();
    if(ip == currentSessionIp) {

    }else {
        currentSessionIp = ip;
        ui->textBrowser->setText(imTool->readAllMessag(ip));
        ui->textBrowser->moveCursor(QTextCursor::End); //滚动到最低端
    }
    ui->currentSessionLabel->setText(QString("当前会话:") + itemStr);
}
/*
 * 发送消息
 */
void MainWindow::on_pushButton_clicked()
{
    QString msg = ui->textEdit->toPlainText();

    if (ui->textEdit->toPlainText().trimmed() == "") {
        QMessageBox::warning(0,"警告","发送内容不能为空",QMessageBox::Ok);
        return;
    }
    if (currentSessionIp == "") {
        QMessageBox::warning(0,"警告","请选择聊天对象",QMessageBox::Ok);
        return;
    }
    imTool->sendMessage(IMTool::Message,currentSessionIp,msg);
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->textBrowser->append("我-" + timeStr);
    ui->textBrowser->append(msg);
    ui->textBrowser->append("");
    ui->textEdit->setText("");
    ui->textBrowser->moveCursor(QTextCursor::End); //滚动到最低端
    imTool->insertMessage(currentSessionIp,imTool->getUserName(),QHostInfo::localHostName(),msg,timeStr,true);

}
bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj != ui->textEdit){
        return false;
    }
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *event = static_cast<QKeyEvent*>(e);
        if (event->key() == Qt::Key_Return)
        {
            if (event->modifiers() & Qt::ControlModifier) {
                ui->textEdit->append(""); //ctrl + enter 按键输入换行
            }else{
                //enter直接发送消息
                on_pushButton_clicked(); //发送消息的槽
            }
            return true;
        }

    }
    return false;
}

void MainWindow::on_cleanHistoryButton_clicked()
{
    //清空聊天记录
    if (currentSessionIp != ""){
        imTool->cleanMessageHistory(currentSessionIp);
    }

    ui->textBrowser->setText("");
}

void MainWindow::on_sendFileButton_clicked()
{
    if(ui->listWidget->selectedItems().isEmpty())//传送文件前需选择用户
       {
           QMessageBox::warning(0, "选择用户",
                          "请先从用户列表选择要传送的用户！", QMessageBox::Ok);
           return;
       }
    filesenderdialog->initServer();
    filesenderdialog->show();
}

void MainWindow::sendFileName(QString fileName)
{
    imTool->sendMessage(IMTool::File,currentSessionIp,"",fileName);
}
