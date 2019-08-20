#include "probeinfo.h"

ProbeInfo::ProbeInfo()
{

}
ProbeInfo::ProbeInfo(QString sn,unsigned char id){
    this->sn = sn;
    this->id = id;
}


QString ProbeInfo::getSn(){
    return this->sn;
}

unsigned char ProbeInfo::getId(){
    return  this->id;
}
