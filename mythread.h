#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QByteArray>
#include <QObject>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <QSettings>
#include <math.h>
#include <QReadWriteLock>
#include "dbutil.h"
#include "probeinfo.h"
#include "mydata.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class MyThread : public QObject
{
  Q_OBJECT
public:
  explicit MyThread(QObject *parent = nullptr);
  ~MyThread();
    //void init_port();  //初始化串口

public slots:
  void handle_data();  //处理接收到的数据
  void init_port(QString portName,qint32 baudRate);
  void close_port();
  void receive_id(int id);
  void setCalParameter(unsigned char real_id,float k,float b,unsigned char addr,unsigned char len);
  void setRotationPeriod(unsigned char real_id,unsigned short time,QString type); //设置转动周期
  void startMeasure();
  void stopMeasure();

  void setPHCal(float ph);  //开始PH校准，用于三点校准
  void setOrpCal(float k,float b);  //色设置orp参数
  void setPHPole(float k1,float k2,float k3,float k4,float k5,float k6);  //设置PH电极参数
  void getPHCalStatus();      //获取校准状态

  //水中油一点定标
  void setOilCal(unsigned char real_id,float standare);
  void openNaHongBrush(unsigned char real_id,int mode);
  void setOilClearNum(unsigned char real_id,unsigned char num);

  void setNaNhCal(unsigned char real_id,float standare,QString calType);

  //post数据应答槽
  void SltLoginReplay(QNetworkReply* reply);


  /******************************************/
  void run(bool isRun);

  void setSlaveId(unsigned char id_update,unsigned char id_real,QString type);

  float getChla(unsigned char real_id,QString type);

  void singleSearch();

  void defaultSearch();

  void clear(unsigned char real_id,QString type);

  void getCalParameter(unsigned char real_id,QString type,unsigned char addr);

  void cycleRead(QList<ProbeInfo> *onlineList);

  void getTestData(unsigned char real_id,QString type,bool one,QString param);

  void sqlConn(QString addr,QString userName,QString pwd,bool conn,int time_delay);

  float do_value_switch(float TMEP,float bdo);

  QByteArray getSN(uint8_t id_real,unsigned char count,QString type);

  QByteArray getSondeSN(uint8_t real_id,unsigned char count);

  QByteArray getNaHongSN(uint8_t real_id,unsigned char count);

  void getSNSignal(unsigned char real_id,QString type);

  QByteArray getNaHongDevType(uint8_t real_id,unsigned char count);

  float getNaNhNH4_N(uint8_t real_id,unsigned char count,QString type);
  void getNaNhpH(uint8_t real_id,unsigned char count,QString type);
  void getNaNhTemp(uint8_t real_id,unsigned char count,QString type);





signals:
  //接收数据
  void id_ok(int id);
  void time_ok(int time);
  void data_type(QByteArray data,QString type);
  void single_search_ok(QString snType,unsigned char id);
  void default_search_ok(QString *snTypes,unsigned char count);
  void test1_ok(float data);
  void oil_one_cal_ok(QString str);
  void com_open_ok(QString portName,bool ok);
  void cal_param_ok(float k,float b,int ok);            //1：校准成功  0：读取成功  -1：校准失败
  void oil_clear_num_ok(unsigned char num,bool ok);
  void sql_conn_ok(bool ok,bool conn);
  void changeStatus(QString type,QString status);
  void save_data_ok();
  void get_sn_ok(QString sn);
  void post_ok(QString msg);


private:
//  QThread *my_thread;
  QSerialPort *port;
  QTimer *timer;
  QByteArray hexData;
  unsigned char slaveId;
  int id;
  QString portName;
  qint32 comBaudRate;
  bool isPortOpen;
  int time;
  QReadWriteLock lock;


  int time_delay = 2000;    //间隔多长时间保存一次数据
  bool condDelay = false;
  bool isSave = false;

  QString readType;         //正在读取的探头类型

  int codCount = 0;
  int nhCount = 0;
  int ldoCount = 0;
  int turbCount = 0;
  int condCount = 0;
  int phCount = 0;
  int orpCount = 0;
  int chlaCount = 0;
  int bgacount = 0;
  int oilCount = 0;
  int sondeCount = 0;

  int portCount = 0;
  int sqlCount = 0;






  char convertCharToHex(char ch);
  void convertStringToHex(const QString &str, QByteArray &byteData);
  void sendInfo(char* info,int len);
  //void sendInfo( QString &info);
  void sendInfo(QString &info);
  void sleep(int msec);
  uint16_t GetCRC16(unsigned char *puchMsg, unsigned short usDataLen);
  QByteArray command(uint8_t *data,uint8_t sendLen,uint8_t recLen,uint8_t count,uint8_t end,QString type);  //用于发送命令的处理，如计算CRC
  void openBrush(uint8_t real_id);
  void openSondeBrush(uint8_t real_id);
  float getTempAndTurb(unsigned char real_id,QString type);
  float getTempAndOxygen(unsigned char real_id,QString type);

  float getTempAndConductance(unsigned char real_id,QString type);
  float getTempAndBga(unsigned char real_id,QString type);
  void getTempAndOil(unsigned char real_id,QString type);
  float getOiwAndTemp(unsigned char real_id,QString type);
  float getTempAndCodAndToc(unsigned char real_id,QString type);
  float getTurbOfCod(unsigned char real_id,QString type);


  QByteArray getVersion();
  QByteArray getNaHongVersion();

  QByteArray getSlaveId();
  QByteArray getNaHongSlaveId();
  QByteArray getNaHongName(uint8_t id_real,unsigned char count);

  //氨氮探头
  float getOrpAndPh(unsigned char real_id,QString type);       //获取电位值和PH值
  void getNhTemp(unsigned char real_id,QString type);         //获取探头的温度值
  void getOrpCal();                 //获取orp参数
  void getPHPole();                //获取PH电极参数

  float getNH4plusAndKplus(unsigned char real_id,QString type);
  float getNH4_N(unsigned char real_id,QString type);


  //PH探头
  float readOrpAndPhOfPH(unsigned char real_id,QString type);
  void getPHTemp(unsigned char real_id,QString type);         //获取探头的温度值

  //orp探头
  float readOrp(unsigned char real_id,QString type);
  void getOrpTemp(unsigned char real_id,QString type);

  //多合一探头
  float getSondeData(unsigned char real_id,QString type,QString param);

//  COD，氨氮，溶解氧，电导率，水温，浊度，PH
  void postData(QString time,float cod,float an,float ldo,float wt,float cond,float turb,float ph);


};

#endif // MYTHREAD_H
