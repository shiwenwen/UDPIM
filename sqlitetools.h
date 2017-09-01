#ifndef SQLITETOOLS_H
#define SQLITETOOLS_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>


class SQLiteTools
{

public:
    ~SQLiteTools();
    QSqlDatabase database;
    QSqlQuery query;
    SQLiteTools(QString name = "qt_sql_default_connection", QString databaseName = "UDPDataBase.db", QString userName = "sww",QString password = "123456");
    /*
     * 链接数据库
     */
    void connectDatabase(QString name = "qt_sql_default_connection", QString databaseName = "UDPDataBase.db", QString userName = "sww",QString password = "123456");
    bool open();
    void close();

};

#endif // SQLITETOOLS_H
