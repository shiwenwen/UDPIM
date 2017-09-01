#include "imtool.h"
#include <QHostInfo>
#include <QDateTime>
#include <QProcess>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QUdpSocket>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#define SESSION_HISTORY_TABLE "tbl_session_history"
#include <QJsonObject>
#include <QJsonDocument>
// QJson >> QString
QString getStringFromJsonObject(const QJsonObject& jsonObject){
    return QString(QJsonDocument(jsonObject).toJson());
}
// QString >> QJson
QJsonObject getJsonObjectFromString(const QString jsonString){
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toLocal8Bit().data());
    if( jsonDocument.isNull() ){
        qDebug()<< "===> please check the string "<< jsonString.toLocal8Bit().data();
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}
IMTool::IMTool(){

    udpSocket = new QUdpSocket(this);
    port = 6666;
    //绑定端口
    udpSocket->bind(port,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    //连接信号槽 readyRead()信号在每当有新数据来临时被触发
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));
    sqltool = SQLiteTools();
    if (sqltool.open()) {
        qDebug()<< "数据库打开成功";
    }else{
        qDebug()<< "数据库打开失败";
    }
    createTable();
}
IMTool::IMTool(qint16 port){

    udpSocket = new QUdpSocket(this);
    this->port = port;
    //绑定端口
    udpSocket->bind(port,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    //连接信号槽 readyRead()信号在每当有新数据来临时被触发
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));
    sqltool = SQLiteTools();
    if (sqltool.open()) {
        qDebug()<< "数据库打开成功";
    }else{
        qDebug()<< "数据库打开失败";
    }

    createTable();
}
void IMTool::createTable(){

    //创建表
    QString create_sql = QString("create table if not exists %1 (username varchar(100), ipAddress varchar(50) , hostname varchar(100),message text,time varchar(50),is_self int,id integer primary key autoincrement)").arg(SESSION_HISTORY_TABLE);
    sqltool.query.prepare(create_sql);
    if(!sqltool.query.exec()){
        qDebug() <<  "Error: Fail to create table." << sqltool.query.lastError();
    }else{
        qDebug() << "创建表成功";
    }
}

IMTool::~IMTool(){
    logout();
    sqltool.close();
    udpSocket->close();
}
void IMTool::login(){
    //广播自己 告诉其他用户已经登录
    sendMessage(NewClient);
}
void IMTool::logout(){
    sendMessage(Logout);
}

void IMTool::sendMessage(MessageType type, QString clientIp, QString messageContent,QString fileName){
    QByteArray data;

    QString localHostName = QHostInfo::localHostName();//主机名
    QString address = getIP();//ip


    QJsonObject msgBody;
    msgBody.insert("type",QJsonValue(type));
    msgBody.insert("username",QJsonValue(getUserName()));
    msgBody.insert("hostname",QJsonValue(localHostName));
    msgBody.insert("ip",QJsonValue(address));
    switch (type) {
    case Message:
        if (messageContent == "") {
            QMessageBox::warning(0,"警告","发送内容不能为空",QMessageBox::Ok);
            return;
        }


        msgBody.insert("content",QJsonValue(messageContent));
        break;
    case NewClient:
    case Logout:
        data = getStringFromJsonObject(msgBody).toLocal8Bit();
        udpSocket->writeDatagram(data,data.length(),QHostAddress::Broadcast,port);
        return;
    case File:
        msgBody.insert("clientIp",QJsonValue(clientIp));
        msgBody.insert("fileName",QJsonValue(fileName));
        break;
    case Refuse:
        break;
    default:
        break;
    }
    data = getStringFromJsonObject(msgBody).toLocal8Bit();
    udpSocket->writeDatagram(data,data.length(),QHostAddress(clientIp),port);
}
QString IMTool::getIP(){
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    return 0;
}
QString IMTool::getUserName(){

    QString userName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    userName = userName.section("/", -1, -1);
    return userName;
}

/*
 * ----SLOT--------
 */
void IMTool::processPendingDatagrams(){
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram; //字节数组
        datagram.resize(udpSocket->pendingDatagramSize());//归一化size为报文的size
        udpSocket->readDatagram(datagram.data(),datagram.size());//读取不大于size的数据到daagram.data()中

        int messageType;

        QString userName,localHostName,ipAddress,message;

        QString jsonStr(datagram);
        QJsonObject msg = getJsonObjectFromString(jsonStr);
        messageType = msg.value("type").toInt();
        userName = msg.value("username").toString();
        localHostName = msg.value("hostname").toString();
        ipAddress = msg.value("ip").toString();

        if (ipAddress.contains("127.0.0.1") || ipAddress.contains("localhost") || ipAddress.contains(getIP())){
            return;
        }
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (messageType) {
        case Message:
             message = msg.value("content").toString();
             emit receiveNewMessage(userName,localHostName,ipAddress,message,time);
            break;
        case NewClient:
            emit findNewClient(userName,localHostName,ipAddress);
            break;
        case Logout:
            emit disconnectClient(userName,localHostName,ipAddress);
            break;
        case File:
            emit receiveNewFile(userName,localHostName,ipAddress,msg.value("clientIp").toString(),msg.value("fileName").toString(),time);
            break;
        case Refuse:
            break;
        default:
            break;
        }
    }
}

/*
 *保存文件
 */
bool IMTool::saveTextFile(const QString &fileName,QString content){
    QFile file(QCoreApplication::applicationDirPath()+"/"+fileName+".txt");
    if(!file.open(QFile::WriteOnly|QFile::Text|QFile::Truncate)){
//        QMessageBox::warning(0,"保存文件",QString("无法保存文件 %1:\n%2").arg(fileName).arg(file.errorString()),QMessageBox::Ok);
        qDebug() << QString("无法保存文件 %1:\n%2").arg(fileName).arg(file.errorString());
        return false;
    }
    QTextStream out(&file);
    out << content;
    out.flush();
    file.close();
    return true;
}
bool IMTool::appendTextFile(const QString &fileName,QString content){
    QFile file(QCoreApplication::applicationDirPath()+"/txts/"+fileName+".txt");
    if(!file.open(QFile::WriteOnly|QFile::Text|QFile::Append)){
//        QMessageBox::warning(0,"保存文件",QString("无法保存文件 %1:\n%2").arg(fileName).arg(file.errorString()),QMessageBox::Ok);
        qDebug() << QString("无法保存文件 %1:\n%2").arg(fileName).arg(file.errorString());
        return false;
    }
    QTextStream out(&file);
    out << content;
    out.flush();
    file.close();
    return true;
}
QString IMTool::readTextFile(const QString &fileName){
    QFile file(QCoreApplication::applicationDirPath()+"/txts/"+fileName+".txt");
    if(!file.open(QFile::ReadOnly|QFile::Text)){
//        QMessageBox::warning(0,"保存文件",QString("无法读取文件 %1:\n%2").arg(fileName).arg(file.errorString()),QMessageBox::Ok);
        qDebug() << QString("无法读取文件 %1:\n%2").arg(fileName).arg(file.errorString());
        return "";
    }
    QTextStream in(&file);
    QString content = in.readAll();

    file.close();
    return content;
}
bool IMTool::cleanTextFile(const QString &fileName){
    QFile file(QCoreApplication::applicationDirPath()+"/txts/"+fileName+".txt");
    if(!file.open(QFile::WriteOnly|QFile::Text|QFile::Truncate)){
//        QMessageBox::warning(0,"保存文件",QString("无法清除文件 %1:\n%2").arg(fileName).arg(file.errorString()),QMessageBox::Ok);
        qDebug() << QString("无法清除文件 %1:\n%2").arg(fileName).arg(file.errorString());
        return false;
    }
    QTextStream out(&file);
    out << "";
    out.flush();
    file.close();
    return true;
}

/*
 * 插入聊天记录
 */
void IMTool::insertMessage(QString ip,QString username,QString hostname,QString message,QString time,bool isSelf){

    QString insert_sql =QString("insert into %1 values (?, ?, ?, ?, ?, ?, NULL)").arg(SESSION_HISTORY_TABLE);
    sqltool.query.prepare(insert_sql);

    sqltool.query.addBindValue(username);
    sqltool.query.addBindValue(ip);
    sqltool.query.addBindValue(hostname);
    sqltool.query.addBindValue(message);
    sqltool.query.addBindValue(time);
    sqltool.query.addBindValue(isSelf ? 1 : 0);

    if(!sqltool.query.exec())
    {
        qDebug() << sqltool.query.lastError();
    }
    else
    {
        qDebug() << "inserted success!";
    }
}

/*
 * 读取聊天记录
 */
QString IMTool::readAllMessag(QString ip){
    QString select_sql = QString("select * from %1 where ipAddress = '%2' order by id").arg(SESSION_HISTORY_TABLE).arg(ip);
    QString history;
    if(!sqltool.query.exec(select_sql))
    {
        qDebug()<<sqltool.query.lastError();

    }
    else
    {

        while(sqltool.query.next())
        {

            QString username  = sqltool.query.value(0).toString();
            QString ip  = sqltool.query.value(1).toString();
            QString hostname  = sqltool.query.value(2).toString();
            QString message  = sqltool.query.value(3).toString();
            QString time  = sqltool.query.value(4).toString();

            int isSelf = sqltool.query.value(5).toInt();
//            qDebug() << sqltool.query.value(6).toInt();
            if (isSelf == 1) {
                history.append("我-"+time+"\n"+message+"\n");
            }else{
                history.append(username+"["+ip+"]-"+time+"\n"+message+"\n\n");
            }


        }

    }
    return history;
}

/*
 *清除聊天记录
 */
void IMTool::cleanMessageHistory(QString ip){
     QString select_sql = QString("delete from %1 where ipAddress = '%2'").arg(SESSION_HISTORY_TABLE).arg(ip);
     if(!sqltool.query.exec(select_sql))
     {
         qDebug()<<sqltool.query.lastError();

     }else{
        qDebug() << "记录清除成功";
     }
}
