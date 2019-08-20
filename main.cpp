#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QMutex>
#include "log.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //qInstallMessageHandler(outputMessage);
    MainWindow w;   
    w.show();
//    QString appName = QApplication::applicationName();//程序名称
//    QString appPath = QApplication::applicationFilePath();// 程序路径
//    appPath = appPath.replace("/","\\");
//    QSettings *reg=new QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",QSettings::NativeFormat);
//    QString val = reg->value(appName).toString();// 如果此键不存在，则返回的是空字符串
//    if(val != appPath)
//       reg->setValue(appName,appPath);// 如果移除的话，reg->remove(applicationName);
//    reg->deleteLater();
    return a.exec();
}
