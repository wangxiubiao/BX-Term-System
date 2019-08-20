#ifndef DBUTIL_H
#define DBUTIL_H

#include <QObject>
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <mydata.h>

class DBUtil : public QObject
{
    Q_OBJECT
public:
    explicit DBUtil(QObject *parent = nullptr);

    static QSqlDatabase createConnection();

    static void closeDB();

    static QSqlDatabase getSqLite();

    static bool initSqlServerConn(QString addr,QString dbName,QString userName,QString pwd);

    static bool sqlServerIsOpen();


signals:

public slots:

private:
    static QSqlDatabase db;
    static QSqlDatabase sqLite;
};


#endif // DBUTIL_H
