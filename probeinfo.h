#ifndef PROBEINFO_H
#define PROBEINFO_H

#include <QString>



class ProbeInfo
{
public:
    ProbeInfo();
    ProbeInfo(QString sn,unsigned char id);
    //ProbeInfo(QString type,QString sn,QString stutus,unsigned char id);
    QString getSn();
    unsigned char getId();

private:
    QString sn;
    unsigned char id;
};

#endif // PROBEINFO_H
