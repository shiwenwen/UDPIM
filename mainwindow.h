#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "imtool.h"
#include <QMainWindow>
#include <QModelIndex>
namespace Ui {
class MainWindow;
}
class FileSenderDialog;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    IMTool *imTool;
    void initIM();
    QString currentSessionIp;//当前回话ip
    FileSenderDialog *filesenderdialog;

private slots:
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
    /*
     * 收到文件
     */
    void receiveNewFile(QString userName,QString localHostName, QString serverAddress, QString clientAddress, QString fileName, QString timeStr);

    void listClicked(QModelIndex index);
    void on_pushButton_clicked();
    void on_cleanHistoryButton_clicked();

    void on_sendFileButton_clicked();

    void sendFileName(QString fileName);
protected:
    bool eventFilter(QObject *watched, QEvent *event);
};

#endif // MAINWINDOW_H
