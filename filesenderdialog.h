#ifndef FILESENDERDIALOG_H
#define FILESENDERDIALOG_H

#include <QDialog>
#include <QTime>
class QFile;
class QTcpServer;
class QTcpSocket;
namespace Ui {
class FileSenderDialog;
}

class FileSenderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileSenderDialog(QWidget *parent = 0);
    ~FileSenderDialog();
    void initServer();
private:
    Ui::FileSenderDialog *ui;

    void refused();
    void closeEvent(QCloseEvent *);

    qint16 tcpPort;
    QTcpServer *tcpServer;
    QString filePath;
    QString fileName;
    QFile *localFile;

    qint64 TotalBytes;
    qint64 bytesWritten;
    qint64 bytesToWrite;
    qint64 payloadSize;
    QByteArray outBlock;

    QTcpSocket *clientConnection;

    QTime time;
private slots:
    void sendFile();
    void updateClientProgress(qint64 numBytes);

    void on_openFileButton_clicked();

    void on_sendButton_clicked();

    void on_cancelButton_clicked();
    void readChannelFinished();
signals:
    void sendFileName(QString filePath);
};

#endif // FILESENDERDIALOG_H
