#ifndef IMTOOL_H
#define IMTOOL_H
#include <QObject>
#include "sqlitetools.h"
class QUdpSocket;
class IMTool:public QObject
{
public:
    /*
     *消息类型
     */
     enum MessageType{
        Message, //消息
        NewClient,//新客户端上线
        Logout,//客户端退出
        File,//文件
        Refuse//拒绝
    };

    Q_OBJECT
public:
    IMTool();
    IMTool(qint16 port);
    ~IMTool();
    /*
     * 登录
     */
    void login();
    /*
     *发送消息
     */
    void sendMessage(MessageType type, QString clientIp = "", QString messageContent = "", QString fileName = "");
    /*
     * 退出登录
     */
    void logout();
    /*
     *保存文本文件 追加
     */
    bool appendTextFile(const QString &fileName,QString content);
    /*
     *保存文本文件 覆盖
     */
    bool saveTextFile(const QString &fileName,QString content);
    /*
     *读取文本文件
     */
    QString readTextFile(const QString &fileName);
    /*
     * 清除文本文件内容
     */
    bool cleanTextFile(const QString &fileName);
    /*
     * 插入聊天记录
     */
    void insertMessage(QString ip,QString username,QString hostname,QString message,QString time,bool isSelf);
    /*
     * 读取聊天记录
     */
    QString readAllMessag(QString ip);
    /*
     *清除聊天记录
     */
    void cleanMessageHistory(QString ip);

    /*
     * 获取ip
     */
    QString getIP();
    /*
     * 获取用户名
     */
    QString getUserName();
    /*
     * 创建聊天表格
     */
protected:
    void createTable();
private:
    QUdpSocket *udpSocket;
    qint16 port;
    SQLiteTools sqltool;

//信号槽
private slots:
    /*
     * 接收到udp消息
     */
    void processPendingDatagrams();

Q_SIGNALS:
    /*
     * 发现新客户端
     */
    void findNewClient(QString userName, QString localHostName, QString ipAddress);
    /*
     * 客户端断开
     */
    void disconnectClient(QString userName,QString localHostName, QString ipAddress);
    /*
     * 收到新消息
     */
    void receiveNewMessage(QString userName, QString localHostName, QString ipAddress,QString message,QString timeStr);
    /**
     * @brief receiveNewFile 收到文件请求
     */
    void receiveNewFile(QString userName,QString localHostName, QString serverAddress,
                        QString clientAddress, QString fileName, QString timeStr);
};

#endif // IMTOOL_H
