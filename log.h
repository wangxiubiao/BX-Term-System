#ifndef LOG_H
#define LOG_H
#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QObject>
#include <QTextStream>

QString fileName = "log";
QDateTime date;
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)

{
    static QMutex mutex;
    mutex.lock();
    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;
    case QtWarningMsg:
        text = QString("Warning:");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:");
    }
    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(msg).arg(current_date);
    QFile file(fileName+".txt");
    file.open(QIODevice::ReadWrite | QIODevice::Append);
    if(file.size() > 1024*1024*10){
        file.close();
        fileName="log" + QDateTime::currentDateTime().toString("yyyyMMdd-hh-mm-ss")+".txt";
        //QFile file("log1.txt");
        file.open(QIODevice::ReadWrite | QIODevice::Append);
    }
    //file.size();
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();
    mutex.unlock();

}

#endif // LOG_H
