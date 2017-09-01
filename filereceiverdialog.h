#ifndef FILERECEIVERDIALOG_H
#define FILERECEIVERDIALOG_H

#include <QDialog>
#include <QHostAddress>
#include <QFile>
#include <QTime>
class QTcpSocket;
namespace Ui {
class FileReceiverDialog;
}

class FileReceiverDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileReceiverDialog(QWidget *parent = 0);
    ~FileReceiverDialog();
    void setHostAddress(QHostAddress address);
    void setFileName(QString fileName);

protected:
    void closeEvent(QCloseEvent *);
private:
    Ui::FileReceiverDialog *ui;
    QTcpSocket *tcpClient;
    quint16 blockSize;
    QHostAddress hostAddress;
    qint16 tcpPort;

    qint64 TotalBytes;
    qint64 bytesReceived;
    qint64 fileNameSize;
    QString fileName;
    QFile *localFile;
    QByteArray inBlock;

    QTime time;
private slots:

    void newConnect();
    void readFile();
    void displayError(QAbstractSocket::SocketError);
    void on_cancelButton_clicked();
};

#endif // FILERECEIVERDIALOG_H
