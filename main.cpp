#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());
    a.setWindowIcon(QIcon("super.ico"));
    w.show();

    return a.exec();

}
