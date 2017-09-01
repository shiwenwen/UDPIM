#include "sqlitetools.h"

SQLiteTools::~SQLiteTools(){
    close();
}

SQLiteTools::SQLiteTools(QString name, QString databaseName ,QString userName, QString password ){
    connectDatabase(name,databaseName,userName,password);
}

/*
 * 链接数据库
 */
void SQLiteTools::connectDatabase(QString name, QString databaseName , QString userName,QString password){
    if (QSqlDatabase::contains(name)) {
        database = QSqlDatabase::database(name);
    }else {
        database = QSqlDatabase::addDatabase("QSQLITE");
        database.setDatabaseName(databaseName);
        database.setUserName(userName);
        database.setPassword(password);
    }
    query = QSqlQuery(database);

}

bool SQLiteTools::open(){
    return database.open();
}

void SQLiteTools::close(){
    database.close();
}
