#ifndef MYDATA_H
#define MYDATA_H

#include <QTime>
#include <QVariantList>



class MyData
{
public:
    MyData();

    static QString codSn;
    static QString nhSn;
    static QString nhNaSn;
    static QString ldoSn;
    static QString condSn;
    static QString phSn;
    static QString orpSn;
    static QString chlaSn;
    static QString bgaSn;
    static QString turbSn;
    static QString oilSn;
    static QString oiwSn;
    static QString sondeSn;
    static QString specSn;
    static QString chloSn;
    static QString nitrSn;

    static float currentK;
    static float currentB;
    static float currentPeriod;

    static float chlaTemp;      //叶绿素探头温度
    static float chlaChla;      //叶绿素

    static float condTemp;      //电导率温度
    static float condCond;      //电导率

    static float sondeLdo;      //多合一溶解氧
    static float sondeTurb;     //多合一浊度
    static float sondeCond;     //多合一电导率
    static float sondePH;       //多合一PH
    static float sondeChla;     //多合一叶绿素
    static float sondeBga;      //多合一蓝绿藻
    static float sondeOrp;      //多合一ORP
    static float sondeTemp;     //多合一温度

    static float turbTemp;      //浊度温度
    static float turbTurb;      //浊度

    static float ldoTemp;       //溶解氧温度
    static float ldoLdo;        //溶解氧
    static float ldoSaturation; //溶解氧饱和度

    static float oilOil;        //水中
    static float oilTemp;       //水中油温度

    static float codCod;        //COD
    static float codToc;
    static float codTurb;
    static float codTemp;       //Cod温度

    static float bgaBga;
    static float bgaTemp;

    static float nhOrp;
    static float nhPH;
    static float nhNh4Plus;
    static float nhKPlus;
    static float nhNh4_N;
    static float nhTemp;

    static float phPH;
    static float phOrp;
    static float phTemp;

    static float orpOrp;
    static float orpPH;
    static float orpTemp;


    static QString testSn1;      //

    static bool clickedEvent;
    static bool isRun;
    static QDateTime sqlTime;   //保存数据库间隔时间
    static int timePeroid;
    static int firstSave;
    static QVariantList timeList, probeList, paramList, dataList, unitList;    //保存到数据库的数据列表

    static bool isSondeCal;
    static QString calParam;

    static QString oilType;
    static QString nhType;

    static int sqlServerStatus;        //数据库连接状态 0:未连接 1：已连接

    static bool idRepeat;

};

#endif // MYDATA_H
