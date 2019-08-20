#include "dbutil.h"

#include <QCoreApplication>
#include <QSqlRecord>


QSqlDatabase DBUtil::db = QSqlDatabase::addDatabase("QODBC","sqlserver");
QSqlDatabase DBUtil::sqLite = QSqlDatabase::addDatabase("QSQLITE", "sqLite");

DBUtil::DBUtil(QObject *parent) : QObject(parent)
{

}

QSqlDatabase DBUtil::getSqLite(){
    if(sqLite.isOpen()){
        qDebug()<<"sqLite数据库已连接";
        return sqLite;
    }
    sqLite.setDatabaseName("./config.db");
    if(!sqLite.open ()){
        qDebug()<<"sqLite连接失败"<<sqLite.lastError().text()<<endl;
        return sqLite;
     }

    //查询是否有
    QSqlQuery query = QSqlQuery(sqLite);
    if(!query.exec("select * from userConfig")){
        qDebug()<<"自动读取："<<query.lastError();
        QString createTable = "CREATE TABLE userConfig(id INTEGER  PRIMARY KEY AUTOINCREMENT,type NVARCHAR NOT NULL,value NVARCHAR NOT NULL)";
        if(!query.exec(createTable)){
            qDebug()<<"创建表失败！";
        }
        else {
            qDebug()<<"创建成功";
            QVariantList typeList,valueList;
            typeList<<"autoRead"<<"deafaultPort"<<"sqlName"<<"sqlPwd"<<"sqlIP"<<"sqlTime";
            valueList<<"1"<<"COM3"<<"sa"<<"123456"<<"127.0.0.1"<<"5";
            query.prepare("insert into userConfig (type,value) values(?,?)");
            query.addBindValue(typeList);
            query.addBindValue(valueList);
            if(!query.execBatch()){
                qDebug()<<"插入数据："<<query.lastError();
            }
        }
    }

    return sqLite;
}

QSqlDatabase DBUtil::createConnection(){
    //QString serverName = "132.232.237.188,1433";
    if(db.isOpen()){
        return db;
    }
    QStringList drivers = QSqlDatabase::drivers();

    if(!db.open ()){
        qDebug()<<"sql server重新连接失败"<<db.lastError().text()<<endl;
        return db;
     }   
    return db;
}


/**
 * @brief DBUtil::initSqlServerConn 连接sqlServer数据库
 * @param addr
 * @param dbName
 * @param userName
 * @param pwd
 * @return
 */
bool DBUtil::initSqlServerConn(QString addr,QString dbName,QString userName,QString pwd){
    QStringList drivers = QSqlDatabase::drivers();
    QSqlQuery *sql=new QSqlQuery(db);
    QString dsn = QString("Driver={SQL Server};Server=%1,1433;DataBase=%2;").arg(addr).arg(dbName);
    db.setDatabaseName(dsn);
    db.setUserName(userName);
    db.setPassword(pwd);

    if(!db.open ()){
        qDebug()<<"sql serve 初始化连接失败,正在试图创建数据库"<<db.lastError().text()<<endl;
        dsn = QString("Driver={SQL Server};Server=%1,1433;").arg(addr);
        db.setDatabaseName(dsn);
        if(db.open ()){
            sql->exec("create database BXdate");
            if(!db.open()){
                qDebug()<<"创建数据库失败！";
                return false;
            }
            dsn = QString("Driver={SQL Server};Server=%1,1433;DataBase=%2;").arg(addr).arg(dbName);
            db.setDatabaseName(dsn);
            if(db.open()){
              qDebug()<<"创建数据库成功";
              QString createTable = "CREATE TABLE [dbo].[paramInfo]([id] [int] IDENTITY(1,1) NOT NULL,"
                                    "[time] [datetime2](0) NOT NULL,	[probeType] [nvarchar](30) NOT NULL,"
                                    "[paramType] [nvarchar](30) NOT NULL,	[data] [float] NOT NULL,"
                                    "[unit] [nvarchar](20) NOT NULL, CONSTRAINT [PK_paramInfo] PRIMARY KEY "
                                    "CLUSTERED (	[id] ASC)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  "
                                    "= OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON)"
                                    " ON [PRIMARY]) ON [PRIMARY]";
              if(!sql->exec(createTable)){
                  qDebug()<<"创建表失败！";
                  return false;
              }
              return true;
            }
        }
        return false;
     }    
    qDebug()<<"sqlServer 数据库连接成功"<<endl;
    MyData::sqlServerStatus = 1;
    return true;
}

/**
 * @brief DBUtil::sqlServerIsOpen   判断数据库是否打开
 * @return
 */
bool DBUtil::sqlServerIsOpen(){
    if(db.isOpen()){
        return true;
    }
    return false;
}

/**
 * @brief DBUtil::closeDB 关闭数据库连接
 */
void DBUtil::closeDB(){
    if(db.isOpen()){
        db.close();
        MyData::sqlServerStatus = 0;
        qDebug()<<"sqlServer数据库断开";
    }
}
