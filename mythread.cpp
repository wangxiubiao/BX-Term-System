#include "mythread.h"

#include <QSqlRecord>

MyThread::MyThread(QObject *parent) : QObject(parent)
{
    //slaveId = 0x01;
    //my_thread = new QThread();
    port = new QSerialPort();
    isPortOpen = false;   
}

MyThread::~MyThread()
{
    port->close();
    port->deleteLater();
//    my_thread->quit();
//    my_thread->wait();
//    my_thread->deleteLater();
}


void MyThread::close_port(){
    if(isPortOpen){
        if(port->isOpen()){            
            port->close();
            isPortOpen = false;
            qDebug()<<"串口已关闭";
            emit com_open_ok(portName,false);
            this->portName = "";
        }
    }
}

/**
 * @brief MyThread::init_port 打开串口
 * @param portName
 * @param baudRate
 */
void MyThread::init_port(QString portName,qint32 baudRate)
{
    if(port->isOpen()){
        return;
    }

    port = new QSerialPort();
    isPortOpen = true;

    port->setPortName(portName);                   //串口名 windows下写作COM1
    port->setBaudRate(baudRate);                           //波特率
    port->setDataBits(QSerialPort::Data8);             //数据位
    port->setStopBits(QSerialPort::OneStop);           //停止位
    port->setParity(QSerialPort::NoParity);            //奇偶校验
    port->setFlowControl(QSerialPort::NoFlowControl);  //流控制
    if (port->open(QIODevice::ReadWrite))
    {
        qDebug() << "Port have been opened";
        emit com_open_ok(portName,true);
        this->portName = portName;
        this->comBaudRate = baudRate;
    }
    else
    {
        qDebug() << "open port failed";
        emit com_open_ok(portName,false);
    }

//    this->moveToThread(my_thread);
//    port->moveToThread(my_thread);
//    my_thread->start();      //启动线程
}

/**
 * @brief MyThread::run 线程主函数，循环读取探头数据
 */
void MyThread::run(bool isRun){
    qDebug()<<"run被调用"<<isRun;
    QList<ProbeInfo> *onlineList = new QList<ProbeInfo>();
    QSqlDatabase sqLite = DBUtil::getSqLite();
    QSqlQuery query(sqLite);
    MyData::isRun = isRun;
    while (1) {
        while (!MyData::isRun) {
            sleep(10);
        }
        query.clear();
        if(!query.exec("select * from probeInfo where status = 'online'")){
            qDebug()<<"sqLite查询失败,数据库异常"<<sqLite.lastError().text();
            return;
        }
        onlineList->clear();
        while(query.next()){        //获取当前存在的探头列表
            ProbeInfo *info = new ProbeInfo(query.value(2).toString(),query.value(4).toInt() & 0xff);
            onlineList->append(*info);
            delete info;
        }       
        cycleRead(onlineList);
        if(MyData::clickedEvent){
            MyData::clickedEvent = false;
            qDebug()<<"跳出子线程主循环";
            break;
        }
        sleep(100);
    }
    delete onlineList;
}

/**
 * @brief MyThread::cycleRead   读取所有在线的探头数据
 * @param onlineList            在线的探头列表
 */
void MyThread::cycleRead(QList<ProbeInfo> *onlineList){
    QDateTime currentTime;
    QDateTime result;
    QString strTime;    
    QSqlQuery insert;

    if(portName != "" && !port->isOpen()){      //串口已连接但由异常断开则重连

        if(portCount < 5){
            init_port(portName,comBaudRate);
            if(port->isOpen()){
                portCount = 0;
                qDebug()<<"串口异常断开重连成功";
            }
            else {
                portCount++;
                qDebug()<<"第"<<portCount<<"次串口异常断开重连失败";
            }
        }
    }

    if(MyData::sqlServerStatus == 1 && !DBUtil::sqlServerIsOpen()){     //数据库已连接但由网络故障等原因断开则重连
        if(sqlCount < 5){
            DBUtil::createConnection();
            if(DBUtil::sqlServerIsOpen()){
                sqlCount = 0;
                qDebug()<<"数据库异常断开重连成功";
            }
            else {
                sqlCount++;
                qDebug()<<"第"<<sqlCount<<"次数据库异常断开重连失败";
            }
        }
    }

    if(DBUtil::sqlServerIsOpen()){
        lock.lockForWrite();
        currentTime = QDateTime::currentDateTime();
        QString sqlTimeString = MyData::sqlTime.toString("yyyy/MM/dd hh:mm:00");
        MyData::sqlTime = QDateTime::fromString(sqlTimeString,"yyyy/MM/dd hh:mm:00");
        result = QDateTime::fromMSecsSinceEpoch(currentTime.toMSecsSinceEpoch() - MyData::sqlTime.toMSecsSinceEpoch()).toUTC();
        strTime = currentTime.toString("yyyy/MM/dd hh:mm:00");
        time = result.toString("mm").toInt() + result.toString("hh").toInt()*60+(result.toString("dd").toInt()-1)*60*24;

        if (MyData::firstSave == 2) {      //按间隔时间保存
            if(time >= MyData::timePeroid){                
                isSave = true;
                //qDebug()<<"按保存间隔时间存储";
            }
        }
        else if(MyData::firstSave == 0){             //第一次连接数据库，则在连接之后的1分钟保存一次数据                        
            result = QDateTime::fromMSecsSinceEpoch(currentTime.toMSecsSinceEpoch() - MyData::sqlTime.toMSecsSinceEpoch()).toUTC();
            strTime = currentTime.toString("yyyy/MM/dd hh:mm:00");
            time= result.toString("mm").toInt() + result.toString("hh").toInt()*60+(result.toString("dd").toInt()-1)*60*24;

            qDebug()<<"间隔时间："<<time<<"sqlTime"<<MyData::sqlTime.toString("yyyy/MM/dd hh:mm:00");
            qDebug()<<"保存次数"<<MyData::firstSave;
            if(time >= 1){
                isSave = true;
                if(MyData::timePeroid == 2){    //间隔为2分钟，保证在整偶数分保存
                    if(currentTime.toString("mm").toInt()%2 == 0){
                        MyData::firstSave = 2;
                    }
                    else {
                        MyData::firstSave = 1;
                    }
                }
                else if (MyData::timePeroid%5 == 0 && MyData::timePeroid%10 != 0) {     //间隔为5的倍数，保证在5*n分保存，n为整数
                    if(currentTime.toString("mm").toInt()%5 == 0){
                        MyData::firstSave = 2;
                    }
                    else {
                        MyData::firstSave = 1;
                    }
                }
                else if (MyData::timePeroid%10 == 0){       //间隔为10的倍数，保证在10*n分保存，n为整数
                    if(currentTime.toString("mm").toInt()%10 == 0){
                        MyData::firstSave = 2;
                    }
                    else {
                        MyData::firstSave = 1;
                    }
                }
                else {
                    MyData::firstSave = 2;
                }
            }
        }
        else if(MyData::firstSave == 1) {       //第二次保存,如果是2，5，10的倍数，需要在分钟为其倍数是第二次保存
            if(MyData::timePeroid == 2 ){
                if(currentTime.toString("mm").toInt()%2 == 0){
                    MyData::firstSave = 2;
                    isSave = true;
                }
            }
            else if(MyData::timePeroid <= 60) { //小于60分钟和大于60分钟做不同处理
                if(MyData::timePeroid%5 == 0 && MyData::timePeroid%10 != 0){
                    if(currentTime.toString("mm").toInt()%5 == 0 && currentTime.toString("mm").toInt()%10 != 0){
                        MyData::firstSave = 2;
                        isSave = true;
                    }
                }
                else if ((60%MyData::timePeroid) == 0) {
                    if(currentTime.toString("mm").toInt()%MyData::timePeroid == 0){
                        MyData::firstSave = 2;
                        isSave = true;
                    }
                }
                else if ((60%MyData::timePeroid) != 0) {
                    if(currentTime.toString("mm").toInt()%10 == 0){
                        MyData::firstSave = 2;
                        isSave = true;
                    }
                }
            }
            else if (MyData::timePeroid > 60) {
                if(MyData::timePeroid%5 == 0 && MyData::timePeroid%10 != 0){
                    if(currentTime.toString("mm").toInt()%5 == 0 && currentTime.toString("mm").toInt()%10 != 0){
                        MyData::firstSave = 2;
                        isSave = true;
                    }
                }
                else if (MyData::timePeroid%60 == 0) {
                    if(currentTime.toString("mm").toInt() == 0){
                        MyData::firstSave = 2;
                        isSave = true;
                    }
                }
                else if (MyData::timePeroid%10 == 0) {
                    if(currentTime.toString("mm").toInt()%10 == 0){
                        MyData::firstSave = 2;
                        isSave = true;
                    }
                }
            }
        }
        lock.unlock();
    }
    lock.lockForWrite();
    //qDebug()<<"循环读取所有在线探头数据";
    for(int i = 0; i < onlineList->size(); i++){    //循环读取当前存在的探头的数据
        if(!QString::compare(MyData::codSn,onlineList->value(i).getSn())){   //cod
            //qDebug()<<"开始读取Cod探头数据，探头id："<<onlineList->value(i).getId();
            readType = "codCod";
            if(getTurbOfCod(onlineList->value(i).getId(),MyData::codSn) != -1){
                readType = "codTurb";

                getTempAndCodAndToc(onlineList->value(i).getId(),MyData::codSn);
            }

            //qDebug()<<"结束读取Cod探头数据，探头id："<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime<<strTime<<strTime;
                MyData::probeList<<"COD"<<"COD"<<"COD"<<"COD";
                MyData::paramList<<"cod"<<"toc"<<"turb"<<"temp";
                MyData::dataList<<QString::number(MyData::codCod)<<QString::number(MyData::codToc)<<QString::number(MyData::codTurb)
                       <<QString::number(MyData::codTemp);
                MyData::unitList<<"mg/L"<<"mg/L"<<"NTU"<<"℃";
            }
        }
        else if (!QString::compare(MyData::nhSn,onlineList->value(i).getSn())) {        //氨氮
            //qDebug()<<"开始读取氨氮探头数据，探头id："<<onlineList->value(i).getId();
            readType = "NH4_4";
            if(getNH4_N(onlineList->value(i).getId(),onlineList->value(i).getSn()) != -1){  //减少循环时间
                readType = "nhOrpAndPH";
                getOrpAndPh(onlineList->value(i).getId(),onlineList->value(i).getSn());
                readType = "NH4+";
                getNH4plusAndKplus(onlineList->value(i).getId(),onlineList->value(i).getSn());
                readType = "nhTemp";
                getNhTemp(onlineList->value(i).getId(),onlineList->value(i).getSn());
            }
             //qDebug()<<"结束读取氨氮探头数据，探头id："<<onlineList->value(i).getId();
             if(DBUtil::sqlServerIsOpen()){      //如果数据库是出于连接状态，则保存数据
                 if(isSave){
                     MyData::timeList<<strTime<<strTime<<strTime<<strTime<<strTime;
                     MyData::probeList<<"NH"<<"NH"<<"NH"<<"NH"<<"NH";
                     MyData::paramList<<"orp"<<"ph"<<"NH4+"<<"NH4_N"<<"temp";
                     MyData::dataList<<QString::number(MyData::nhOrp)<<QString::number(MyData::nhPH)<<QString::number(MyData::nhNh4Plus)
                            <<QString::number(MyData::nhNh4_N)<<QString::number(MyData::nhTemp);
                     MyData::unitList<<"mV"<<"pH"<<"mV"<<"mg/L"<<"℃";
                 }
             }
        }
        else if (!QString::compare(MyData::nhNaSn,onlineList->value(i).getSn())) {        //氨氮
            //qDebug()<<"开始读取氨氮探头数据，探头id："<<onlineList->value(i).getId();
            readType = "NH4_4";
            if(getNaNhNH4_N(onlineList->value(i).getId(),1,onlineList->value(i).getSn()) != -1){  //减少循环时间
                readType = "nhOrpAndPH";
                getNaNhpH(onlineList->value(i).getId(),1,onlineList->value(i).getSn());
                readType = "nhTemp";
                getNaNhTemp(onlineList->value(i).getId(),1,onlineList->value(i).getSn());
            }
             //qDebug()<<"结束读取氨氮探头数据，探头id："<<onlineList->value(i).getId();
             if(DBUtil::sqlServerIsOpen()){      //如果数据库是出于连接状态，则保存数据
                 if(isSave){
                     MyData::timeList<<strTime<<strTime<<strTime;
                     MyData::probeList<<"NH"<<"NH"<<"NH";
                     MyData::paramList<<"ph"<<"NH4_N"<<"temp";
                     MyData::dataList<<QString::number(MyData::nhPH)<<QString::number(MyData::nhNh4_N)<<QString::number(MyData::nhTemp);
                     MyData::unitList<<"pH"<<"mg/L"<<"℃";
                 }
             }
        }



        else if (!QString::compare(MyData::ldoSn,onlineList->value(i).getSn())) {        //溶解氧
            //qDebug()<<"开始读取溶解氧探头数据，探头id："<<onlineList->value(i).getId();
            readType = "ldo";
            getTempAndOxygen(onlineList->value(i).getId(),MyData::ldoSn);
            //qDebug()<<"结束读取溶解氧探头数据，探头id："<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime<<strTime;
                MyData::probeList<<"LDO"<<"LDO"<<"LDO";
                MyData::paramList<<"ldo"<<"temp"<<"saturation";
                MyData::dataList<<QString::number(MyData::ldoLdo)<<QString::number(MyData::ldoTemp)<<QString::number(MyData::ldoSaturation);
                MyData::unitList<<"mg/L"<<"℃"<<"%";
            }
        }       
        else if (!QString::compare(MyData::condSn,onlineList->value(i).getSn())) {        //电导率
            //qDebug()<<"开始获取电导率探头数据，探头id:"<<onlineList->value(i).getId();
            condDelay = true;
            readType = "cond";
            sleep(200);
            getTempAndConductance(onlineList->value(i).getId(),MyData::condSn);
            condDelay = false;
            //qDebug()<<"结束获取电导率探头数据，探头id:"<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"Cond"<<"Cond";
                MyData::paramList<<"cond"<<"temp";
                MyData::dataList<<QString::number(MyData::condCond)<<QString::number(MyData::condTemp);
                MyData::unitList<<"μS/cm"<<"℃";
            }
        }
        else if (!QString::compare(MyData::turbSn,onlineList->value(i).getSn())) {        //浊度
            //qDebug()<<"开始获取浊度探头数据，探头id："<<onlineList->value(i).getId();
            readType = "turb";
            getTempAndTurb(onlineList->value(i).getId(),MyData::turbSn);
            //qDebug()<<"结束获取浊度探头数据，探头id："<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"Turb"<<"Turb";
                MyData::paramList<<"turb"<<"temp";
                MyData::dataList<<QString::number(MyData::turbTurb)<<QString::number(MyData::turbTemp);
                MyData::unitList<<"NTU"<<"℃";
            }
        }

        else if (!QString::compare(MyData::phSn,onlineList->value(i).getSn())) {        //ph
            //qDebug()<<"开始获取pH探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "phOrpAndPH";
            if(readOrpAndPhOfPH(onlineList->value(i).getId(),MyData::phSn) != -1){
                readType = "phTemp";
                getPHTemp(onlineList->value(i).getId(),MyData::phSn);
            }

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime<<strTime;
                MyData::probeList<<"pH"<<"pH"<<"pH";
                MyData::paramList<<"ph"<<"orp"<<"temp";
                MyData::dataList<<QString::number(MyData::phPH)<<QString::number(MyData::phOrp)<<QString::number(MyData::phTemp);
                MyData::unitList<<"pH"<<"mV"<<"℃";
            }
        }
        else if (!QString::compare(MyData::orpSn,onlineList->value(i).getSn())) {        //orp
            //qDebug()<<"开始获取orp探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "orpOrp";
            if(readOrp(onlineList->value(i).getId(),MyData::orpSn) != -1){
                readType = "orpTemp";
                getOrpTemp(onlineList->value(i).getId(),MyData::orpSn);
            }
            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"Orp"<<"Orp";
                MyData::paramList<<"orp"<<"temp";
                MyData::dataList<<QString::number(MyData::orpOrp)<<QString::number(MyData::orpTemp);
                MyData::unitList<<"mV"<<"℃";
            }
        }
        else if (!QString::compare(MyData::chlaSn,onlineList->value(i).getSn())) {        //叶绿素a
            //qDebug()<<"开始获取叶绿素a探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "chla";
            getChla(onlineList->value(i).getId(),MyData::chlaSn);
            //qDebug()<<"结束获取叶绿素a探头数据，探头id:"<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"Chla"<<"Chla";
                MyData::paramList<<"chla"<<"temp";
                MyData::dataList<<QString::number(MyData::chlaChla)<<QString::number(MyData::chlaTemp);
                MyData::unitList<<"μg/L"<<"℃";
            }
        }
        else if (!QString::compare(MyData::bgaSn,onlineList->value(i).getSn())) {        //蓝绿藻
            //qDebug()<<"开始获取蓝绿藻探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "bga";
            getTempAndBga(onlineList->value(i).getId(),onlineList->value(i).getSn());
            //qDebug()<<"结束获取蓝绿藻探头数据，探头id:"<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"Bga"<<"Bga";
                MyData::paramList<<"bga"<<"temp";
                MyData::dataList<<QString::number(MyData::bgaBga)<<QString::number(MyData::bgaTemp);
                MyData::unitList<<"cells/mL"<<"℃";
            }
        }
        else if (!QString::compare(MyData::oilSn,onlineList->value(i).getSn())) {        //水中油
            //qDebug()<<"开始获取水中油探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "oil";
            getTempAndOil(onlineList->value(i).getId(),MyData::oilSn);
            //qDebug()<<"结束获取水中油探头数据，探头id:"<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"OIL"<<"OIL";
                MyData::paramList<<"oil"<<"temp";
                MyData::dataList<<QString::number(MyData::oilOil)<<QString::number(MyData::oilTemp);
                MyData::unitList<<"mg/L"<<"℃";

            }

        }
        else if (!QString::compare(MyData::oiwSn,onlineList->value(i).getSn())) {        //水中油
            //qDebug()<<"开始获取水中油探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "oiw";
            getOiwAndTemp(onlineList->value(i).getId(),MyData::oiwSn);
            //qDebug()<<"结束获取水中油探头数据，探头id:"<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime;
                MyData::probeList<<"oiw"<<"oiw";
                MyData::paramList<<"oiw"<<"temp";
                MyData::dataList<<QString::number(MyData::oilOil)<<QString::number(MyData::oilTemp);
                MyData::unitList<<"ppm"<<"℃";
            }

        }
        else if (!QString::compare("38",onlineList->value(i).getSn())) {        //多合一
            //qDebug()<<"开始获取多合一探头数据，探头id:"<<onlineList->value(i).getId();
            readType = "sonde";
            getSondeData(onlineList->value(i).getId(),"38","ldo");
            //qDebug()<<"结束获取多合一探头数据，探头id:"<<onlineList->value(i).getId();

            if(DBUtil::sqlServerIsOpen() && isSave){      //如果数据库是出于连接状态，则保存数据
                MyData::timeList<<strTime<<strTime<<strTime<<strTime<<strTime<<strTime<<strTime<<strTime;
                MyData::probeList<<"Sonde"<<"Sonde"<<"Sonde"<<"Sonde"<<"Sonde"<<"Sonde"<<"Sonde"<<"Sonde";
                MyData::paramList<<"ldo"<<"turb"<<"cond"<<"pH"<<"chla"<<"bga"<<"orp"<<"temp";
                MyData::dataList<<QString::number(MyData::sondeLdo)<<QString::number(MyData::sondeTurb)<<QString::number(MyData::sondeCond)
                        <<QString::number(MyData::sondePH)<<QString::number(MyData::sondeChla)<<QString::number(MyData::sondeBga)
                        <<QString::number(MyData::sondeOrp)<<QString::number(MyData::sondeTemp);
                MyData::unitList<<"mg/L"<<"NTU"<<"μS/cm"<<"pH"<<"mg/L"<<"cells/mL"<<"mV"<<"℃";

            }
        }
        else if (!QString::compare(MyData::nhSn,onlineList->value(i).getSn())) {        //

        }
        else if (!QString::compare(MyData::nhSn,onlineList->value(i).getSn())) {        //

        }

        if(MyData::clickedEvent){
            //MyData::clickedEvent = false;
            qDebug()<<"获取测试数据，跳出循环读取在线探头数据";
            break;  //测试时跳出当前循环
        }
    }
    lock.unlock();

    if(DBUtil::sqlServerIsOpen() && isSave && MyData::timeList.size() > 0){
        if (DBUtil::createConnection().transaction()){
            insert = QSqlQuery((DBUtil::createConnection()));
            insert.prepare("insert into paramInfo values(?,?,?,?,?)");
            insert.addBindValue(MyData::timeList);
            insert.addBindValue(MyData::probeList);
            insert.addBindValue(MyData::paramList);
            insert.addBindValue(MyData::dataList);
            insert.addBindValue(MyData::unitList);

            //qDebug()<<"保存数据  MyData::firstSave = "<<MyData::firstSave <<"MyData::timePeroid = "<<MyData::timePeroid;            
            postData(MyData::timeList[0].toString(),MyData::codCod,MyData::nhNh4_N,MyData::ldoLdo,MyData::ldoTemp,MyData::condCond,MyData::codTurb,MyData::nhPH);
            if(!insert.execBatch()){
                qDebug()<<"数据库批量插入错误："<<insert.lastError();
                if(!DBUtil::createConnection().rollback()){
                    qDebug()<<"Error,回滚失败";
                }

            }
            else {
                if(!DBUtil::createConnection().commit()){
                    qDebug()<<"Error,提交失败，将要回滚";
                    if(!DBUtil::createConnection().rollback()){
                        qDebug()<<"Error,回滚失败";
                    }
                }
                emit save_data_ok();
            }

            insert.clear();
            MyData::sqlTime = currentTime;
            isSave = false;
        }


    }
}
/**
 * @brief MyThread::postData    将数据post到宁波云端
 * @param timeList
 * @param paramList
 * @param dataList
 */
QNetworkAccessManager *networkAccessManager;
void MyThread::postData(QString time,float cod,float an,float ldo,float wt,float cond,float turb,float ph){
    QByteArray postData;
    QNetworkRequest request;
    networkAccessManager = new QNetworkAccessManager(this);
    connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(SltLoginReplay(QNetworkReply*)));
    //request.setUrl(QUrl("http://127.0.0.1:8000/historyWarning"));
    request.setUrl(QUrl("http://47.105.193.134:8088/dataExternals/upload?equipType=1&equipId=21803005"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));
    request.setRawHeader("X-TH-TOKEN","eyJpZCI6Ijc4YzQ0YTBjLTk5ZDAtNDgzYy1hNmU5LWFhZmZiNjQ1M2FiMyIsInV"
                                      "zZXJSZWFsbmFtZSI6IuWugeazoua1t-e7teaVsOaNriIsImNyZWF0ZVRpbWVNaW"
                                      "xsaXMiOjE1NTEzMjU2NTA3NTgsImV4cGlyZVRpbWVNaWxsaXMiOjE2NDU5MzM2N"
                                      "TA3NTgsInNlbGZDaGVja0V4cGlyZVRpbWVNaWxsaXMiOjkwMDAwMCwiaXNBZG1p"
                                      "biI6ZmFsc2UsInBsYXRmb3JtIjoid2ViIiwidXNlclR5cGUiOiI1IiwiYWxnIjo"
                                      "iSFMzODQiLCJ6aXAiOiJMVEFTIn0.-H4Uq4ytZkFUlCVH8NFOxacbIbfb8ENQqB"
                                      "6coqZV6tfwPQSaLMIiMfrI8txV66n8Ycp_RoJ8ZaiOHrjQt0eoc46ZCr0jyPCjQ"
                                      "H6gZ5C4g_glMQwshjIdhrOWrKmqImElG3MZSWfiRL4IEXcOUZnyqVIGoOCXmoWs"
                                      "Pz74F8iVxq1ckGQYn7xUCIQikCKC7ypw6oGZMoNgCZGDBXr-6Kyy6m_yl7_NS53"
                                      "jZS7k8gb6cMWR3Ecf4kXr13ltuRSC7aiw3fmbU9uzkhgMXHdMKrDgift9NPaOkW"
                                      "iyYupjKxHiXgnKP-8-r5yDH3rx4auzFYfsTBJE-AYmyZwIEmRT2KycbZAYmasHQ"
                                      "myB0iCZjbtkyV0pkIaVa3Xl8FOc9SGTMmwmPs5Tg7fwGUzLbJa_sEM8e2PhzxdG"
                                      "yl1sXRXTy0aoPTOCmwH_qm25BeHN09FA1RbgcOfWYG4aQkqV4LA9aPFFcO-qT3O"
                                      "7d4YPgBNoZ4hg0Q5PaeKbXSCYonWfj6KxGiLJb3OP6q_sUR06E8-LQ3cVK_jt6a"
                                      "cvRIRx57JGwaiiudI.WoR-gFwI6a5dymL9r9yi0fr5Q_-j8IkipAgiufr34RUMJ"
                                      "_ITQbxw8dqiMlEW4tCR");
    time = time.replace(QRegExp("\\/"), "-");
    qDebug()<<"上传时间："<<time<<"温度："<<wt<<"  cod:"<<cod<<"  an:"<<an<<"   ph:"<<ph<<"  ldo:"<<ldo<<"  cond:"<<cond<<"  turb:"<<turb;

    postData.append("{\"timestamp\":\""+time+"\",\"ph\":\""+QString::number(ph)+"\",\"wt\":\""+QString::number(wt)+"\",\"do\":\""+QString::number(ldo)+
                    "\",\"turbid\":\""+QString::number(turb)+"\",\"con\":\""+QString::number(cond)+"\",\"cod\":\""+QString::number(cod)+"\",\"an\":\""+QString::number(an)+"\"}");
    QNetworkReply* reply = networkAccessManager->post(request,postData);
}

/**
 * @brief MyThread::SltLoginReplay post应答槽，判断数据上传是否成功
 * @param reply
 */
void MyThread::SltLoginReplay(QNetworkReply* reply){
    if (reply->error() == QNetworkReply::NoError){
        QByteArray str = reply->readAll();
        qDebug()<<"post上传成功："<<str;
        emit post_ok(str);
    }
    else {
        qDebug() << "post上传失败";
        qDebug( "found error .... code: %d\n", (int)reply->error());
        qDebug()<<"net :"<<qPrintable(reply->errorString());
    }
    reply->deleteLater();
}


/**
 * @brief MyThread::getTestData 获取校准时的测试数据
 * @param real_id   探头id
 * @param type      探头类型
 * @param one       == true代表获取的是测试1的测试数据，== false 测试2的数据
 */
void MyThread::getTestData(unsigned char real_id,QString type,bool one,QString param){    
    float data = 0;

    if(one){
       qDebug()<<"开始获取测试1数据";
       if(!QString::compare(MyData::codSn,type)){   //cod
           if(param == "Cod"){
               qDebug()<<"开始获取codCod测试1数据";
               data = getTempAndCodAndToc(real_id,type);
           }
           else{
               qDebug()<<"开始获取codTurb测试1数据";
               data = getTurbOfCod(real_id,type);
           }
       }

       else if (!QString::compare(MyData::nhSn,type)) {        //氨氮
           if("NH4_N" == param){
               qDebug()<<"开始获取NH4_N测试1数据";
               data = getNH4_N(real_id,type);
           }
           else if ("pH" == param) {
               qDebug()<<"开始获取nhPH测试1数据";
               data = getOrpAndPh(real_id,type);
           }
           else if("NH4+" == param){
               qDebug()<<"开始获取NH4+测试1数据";
               data = getNH4plusAndKplus(real_id,type);
           }
       }

       else if (!QString::compare(MyData::sondeSn,type)) {        //氨氮
           qDebug()<<"开始获取sonde 测试1数据";
           data = getSondeData(real_id,type,param);
       }

       else if (!QString::compare(MyData::ldoSn,type)) {        //溶解氧
           qDebug()<<"开始获取ldoLdo测试1数据";
           data = getTempAndOxygen(real_id,MyData::ldoSn);
       }

       else if (!QString::compare(MyData::turbSn,type)) {        //浊度
           qDebug()<<"开始获取turbTurb测试1数据";
           data = getTempAndTurb(real_id,MyData::turbSn);
       }

       else if (!QString::compare(MyData::condSn,type)) {        //电导率
           qDebug()<<"开始获取condCond测试1数据";
           data = getTempAndConductance(real_id,MyData::condSn);
       }

       else if (!QString::compare(MyData::phSn,type)) {        //ph
           if(param == "pH"){
               qDebug()<<"开始获取phPH测试1数据";
               data =  readOrpAndPhOfPH(real_id,MyData::phSn);
           }
           else if (param == "orp") {
               qDebug()<<"开始获取pHOrp测试1数据";
               data = readOrp(real_id,MyData::phSn);
           }
       }

       else if (!QString::compare(MyData::orpSn,type)) {        //orp
           qDebug()<<"开始获取orpOrp测试1数据";
           data = readOrp(real_id,MyData::orpSn);
       }

       else if (!QString::compare(MyData::chlaSn,type)) {        //叶绿素a
            qDebug()<<"开始获取chlaChla测试1数据";
           data = getChla(real_id,MyData::chlaSn);
       }
       else if (!QString::compare(MyData::bgaSn,type)) {        //蓝绿藻
            qDebug()<<"开始获取bgaBga测试1数据";
           data = getTempAndBga(real_id,MyData::bgaSn);
       }
       else if (!QString::compare(MyData::oilSn,type)) {        //蓝绿藻
            qDebug()<<"开始获取虞山水中游测试1数据";
           data = getOiwAndTemp(real_id,MyData::oilSn);
       }
       emit test1_ok(data);
       qDebug()<<"结束获取测试1数据";
    }
}

/**
 * @brief MyThread::sqlConn 数据库连接
 * @param addr
 * @param dbName
 * @param userName
 * @param pwd
 * @param conn  true:连接数据库  false:关闭数据库
 */
void MyThread::sqlConn(QString addr,QString userName,QString pwd,bool conn,int time_delay){
    MyData::timePeroid = time_delay;
    qDebug()<<"接收连接信号";
    if(conn){
        emit sql_conn_ok(DBUtil::initSqlServerConn(addr,"BXdate",userName,pwd),conn);
    }
    else {
        if(DBUtil::sqlServerIsOpen()){
            DBUtil::closeDB();
        }
        emit sql_conn_ok(!DBUtil::sqlServerIsOpen(),conn);
        isSave = false;        
        time = 0;
        //MyData::timeList.clear();
    }
}


/**
 * @brief MyThread::singleRearch
 * 单探头搜索
 */
void MyThread::singleSearch(){
    qDebug()<<"开始单探头搜索";
    uint8_t data1[8] = {0xff,0x03,0x30,00,00,01,0x9e,0xd4};     //虞山探头
    uint8_t data2[8] ={0x00, 0x03, 0x00, 0x23, 0x00, 0x01, 0x74, 0x11};  //纳宏探头
    QByteArray idArray = command(data1,8,7,1,3,"");
    QByteArray snArray,type;
    uint8_t id;
    QString *snTypes = new QString[1];    //存放搜索到的探头sn
    uint8_t count = 0;
    QString snString;
    QSqlQuery update(DBUtil::getSqLite());
    bool success = false;
    if(idArray != nullptr){
        id = idArray[3];
        snArray = getSN(id,1,"");
        if(snArray != nullptr){
            type.resize(2);
            type[0] = snArray[6];
            type[1] = snArray[7];
            snString = QString(type);
            qDebug()<<"COD sn:"<<snString;
            if(!QString::compare(MyData::codSn,snString)){       //cod
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                update.exec("update probeInfo set status = 'online' where sn = '47'");
            }
            else if(!QString::compare(MyData::nhSn,snString)){ //氨氮
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '68'");
            }
            else if(!QString::compare(MyData::ldoSn,snString)){ //溶解氧
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '01'");
            }
            else if(!QString::compare(MyData::turbSn,snString)){ //浊度
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '29'");
            }
            else if(!QString::compare(MyData::condSn,snString)){ //电导率
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '09'");
            }
            else if(!QString::compare(MyData::phSn,snString)){ //pH
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '43'");
            }
            else if(!QString::compare(MyData::orpSn,snString)){ //orp
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
            }
            else if(!QString::compare(MyData::chlaSn,snString)){ //叶绿素
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '36'");
            }
            else if(!QString::compare(MyData::bgaSn,snString)){ //蓝绿藻
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                 update.exec("update probeInfo set status = 'online' where sn = '62'");
            }
            else if(!QString::compare(MyData::oiwSn,snString)){ //虞山水中油
                emit single_search_ok(snString,id);
                snTypes[count++] = snString;
                success = true;
                update.exec("update probeInfo set status = 'online' where sn = '63'");
            }
        }
    }else if (idArray == nullptr) {         //纳宏探头查询        
       idArray = command(data2,8,7,1,3,"BX-Oil-1N");
       QByteArray nhnash;
       nhnash.resize(2);
       nhnash[0] = 0x03;
       nhnash[1] = 0x00;
       if(idArray != nullptr){
            id = idArray[4];
            //if(false){
            if(!QString::compare(MyData::oilSn,getNaHongName(id,1))){
                emit single_search_ok(MyData::oilSn,id);
                snTypes[count++] = MyData::oilSn;
                success = true;
                update.exec("update probeInfo set status = 'online' where sn = 'BX-Oil-1N'");
            }
//            int m;
//            memcpy(&m,getNaHongDevType(id,1).data(),2);
//            qDebug()<<getNaHongDevType(id,1).toHex()<<"v    "<<int(m);
            else if(getNaHongDevType(id,1) == nhnash){
              //else if(true){
                emit single_search_ok(MyData::nhNaSn,id);
                snTypes[count++] = MyData::nhNaSn;
                success = true;
                update.exec("update probeInfo set status = 'online' where sn = '768'");
            }
       }
    }
    if (!success) {    //多合一探头查询
        qDebug()<<"多合一单探头搜索";
        uint8_t cmd[8] = {0x01,0x03,0x14,0,0,06};
        for (uint8_t i =1; i < 20; i++) {
            cmd[0] = i;

            snArray = command(cmd,8,17,1,3,"38");
            if(snArray != nullptr){
                emit single_search_ok(MyData::sondeSn,i);
                snArray = getSondeSN(i,1);
                snTypes[count++] = "38";
                update.exec("update probeInfo set status = 'online' where sn = '38'");
                break;
            }
        }
    }
    emit default_search_ok(snTypes,count);
}


/**
 * @brief MyThread::defaultSearch
 * 默认地址搜索
 */
void MyThread::defaultSearch(){
    qDebug()<<"默认搜索被调用";
    QString *snTypes = new QString[14];    //存放搜索到的探头sn
    uint8_t count = 0;          //共搜索到多少个探头
    uint8_t id;
    QByteArray snArray,type;
    QString snString;
    QSqlQuery update(DBUtil::getSqLite());
    QHash<QString,int> *idMap = new  QHash<QString,int>();
    //QSqlDatabase sqLite = DBUtil::getSqLite();
    if(!update.exec("select * from probeInfo")){
        qDebug()<<"查询失败,sqLite数据库异常"<<update.lastError();
        return;
    }
    while(update.next()){        //获取所有探头列表
        idMap->insert(update.value(2).toString(),update.value(4).toInt());
    }
    /**Cod探头 start**********************************/

    id = idMap->value(MyData::codSn) & 0xff;
    readType = "cod";
    snArray = getSN(id,1,MyData::codSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::codSn,snString)){       //cod
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '47'");
        }
    }
    /**Cod探头 end**********************************/

    /**氨氮探头 start**********************************/
    readType = "nh";
    id = idMap->value("68") & 0xff;
    snArray = getSN(id,1,MyData::nhSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::nhSn,snString)){       //cod
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '68'");
        }
    }

    id = idMap->value(MyData::nhNaSn) & 0xff;
    snArray = getNaHongDevType(id,1);
    if(snArray != nullptr){
        QByteArray nhnash;
        nhnash.resize(2);
        nhnash[0] = 0x03;
        nhnash[1] = 0x00;
        if(getNaHongDevType(id,1) == nhnash){
            emit single_search_ok(MyData::nhNaSn,id);
            snTypes[count++] = MyData::nhNaSn;
            update.exec("update probeInfo set status = 'online' where sn = '768'");
        }
    }
    /**氨氮探头 end**********************************/

    /**溶解氧探头 start**********************************/
    readType = "ldo";
    id = idMap->value(MyData::ldoSn) & 0xff;
    snArray = getSN(id,1,MyData::ldoSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::ldoSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '01'");
        }
    }
    /**溶解氧探头 end**********************************/

    /**浊度探头 start**********************************/
    readType = "turb";
    id = idMap->value(MyData::turbSn) & 0xff;
    snArray = getSN(id,1,MyData::turbSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::turbSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '29'");
        }
    }
    /**浊度探头 end**********************************/

    /**电导率探头 start**********************************/
    id = idMap->value(MyData::condSn) & 0xff;
    readType = "cond";
    snArray = getSN(id,1,MyData::condSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::condSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '09'");
        }
    }
    /**电导率探头 end**********************************/

    /**ph探头 start**********************************/
    readType = "ph";
    id = idMap->value(MyData::phSn) & 0xff;
    snArray = getSN(id,1,MyData::phSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::phSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '43'");
        }
    }
    /**ph探头 end**********************************/

    /**orp探头 start**********************************/
    readType = "orp";
    id = idMap->value(MyData::orpSn) & 0xff;
    snArray = getSN(id,1,MyData::orpSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::orpSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '44'");
        }
    }
    /**orp探头 end**********************************/

    /**叶绿素探头 start**********************************/
    readType = "chla";
    id = idMap->value(MyData::chlaSn) & 0xff;
    snArray = getSN(id,1,MyData::chlaSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::chlaSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '36'");
        }
    }
    /**叶绿素探头 end**********************************/

    /**蓝绿藻探头 start**********************************/
    readType = "bga";
    id = idMap->value(MyData::bgaSn) & 0xff;
    snArray = getSN(id,1,MyData::bgaSn);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        if(!QString::compare(MyData::bgaSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '62'");
        }
    }
    /**蓝绿藻探头 end**********************************/

    /**水中油探头 start**********************************/
    readType = "oil";
    id = idMap->value(MyData::oilSn) & 0xff;
    snArray = getNaHongName(id,9);
    if(snArray != nullptr){
        snString = QString(snArray);
        if(!QString::compare(MyData::oilSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = 'BX-Oil-1N'");
        }
    }

    //虞山水中油
    id = idMap->value(MyData::oiwSn) & 0xff;
    snArray = getSN(id,1,MyData::oiwSn);
    qDebug()<<"虞山水中油";
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[6];
        type[1] = snArray[7];
        snString = QString(type);
        qDebug()<<"虞山水中油sn"<<snString;
        if(!QString::compare(MyData::oiwSn,snString)){
            emit single_search_ok(snString,id);
            snTypes[count++] = snString;
            update.exec("update probeInfo set status = 'online' where sn = '63'");
        }
    }
    /**水中油探头 end**********************************/

    /**多合一探头 start**********************************/
    readType = "sonde";
    id = idMap->value("38") & 0xff;
    snArray = getSondeSN(id,9);
    if(snArray != nullptr){
        type.resize(2);
        type[0] = snArray[5];
        type[1] = snArray[6];
        snString = QString(type);
        qDebug()<<"多合一SN:"<<snString;
        if(!QString::compare("38",snString)){
            emit single_search_ok(MyData::sondeSn,id);
            snTypes[count++] = snString;
            qDebug()<<"snString:"<<snTypes[0];
            update.exec("update probeInfo set status = 'online' where sn = '38'");
        }
    }
    /**多合一探头 end**********************************/
       qDebug()<<"count :"<<count<<"sntype[0]:"<<snTypes[0];
       emit default_search_ok(snTypes,count);  //发送所有搜索到的探头

}


QByteArray MyThread::getSondeSN(uint8_t real_id,unsigned char count){
    uint8_t data[8] = {real_id,0x03,0x14,0x00,0x00,0x06};
    QByteArray array;
    QString info;
    unsigned char recLen = 17;
    unsigned char len = 8;
    uint8_t recData[17];
    uint16_t tmp = GetCRC16(data,len-2);   //获取CRC校验码
    data[len-2] = tmp>>8;
    data[len-1] = tmp & 0xff;

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    sendInfo(info);
    sleep(500);
    handle_data();                            //读取下位机返回的数据
    memcpy(recData,hexData.data(),recLen);   //将QByteArray 转换成uint8类型的数组
    tmp = GetCRC16(recData,recLen-2);
    if(recData[recLen-2] == (tmp>>8) && recData[recLen-1] == (tmp & 0xff)){
        return hexData;
    }
    else if (count <= 2) {       //查询3次不成功则认为该探头不存在
        count++;
        qDebug()<<"第"<<count<<"次查询sonde SN";
        return getSondeSN(real_id,count);
    }
    else {
        qDebug()<<"已查询sonde SN 2次，未找到，请检查探头是否存在";
        //QSqlQuery update(DBUtil::getSqLite());
        //update.exec("update probeInfo set status = 'drop' where slave_id = "+QString::number(real_id));
        return nullptr;
    }
}


void MyThread::handle_data()
{
    QByteArray data = port->readAll();    
    hexData.clear();
//    if(readType == "sonde"){
//        qDebug()<<"读取之前"<<hexData.toHex();
//    }
    if(data.size() != 0){
        hexData = data;
        //qDebug() << QStringLiteral("data received(收到的数据):") << hexData;
        //qDebug() << "handing thread is:" << QThread::currentThreadId();
//        if(readType == "sonde"){
//            qDebug()<<"读取之后"<<data.toHex();
//        }
    }
}

void MyThread::receive_id(int id){

}

/**
 * @brief clear          转动刷子，清洗探头
 * @param real_id        探头id
 * @param type           探头类型
 */
void MyThread::clear(unsigned char real_id,QString type){
    if(!QString::compare(MyData::oilSn,type)){       
        if(MyData::oilType == "oiw"){
            openBrush(real_id);
        }
        else {
            openNaHongBrush(real_id,2);
        }
    }
    else if (!QString::compare("38",type)) {
        openSondeBrush(real_id);        
    }
    else {
        openBrush(real_id);
    }
}

/**
 * @brief MyThread::getCalParameter 获取用户校准参数
 * @param real_id   探头id
 * @param type      探头类型
 */
void MyThread::getCalParameter(unsigned char real_id,QString type,unsigned char addr){
    uint8_t data[8] = {real_id,0x03,addr,0x00,0x00,0x04};
    QByteArray tmp;
    QByteArray k,b;
    if(type == "38"){
        data[2] = 0x11;
        data[3] = addr;
    }
    tmp = command(data,8,13,1,3,type);
    if(tmp != nullptr){
        k.resize(4);
        b.resize(4);
        k = tmp.mid(3,4);
        b = tmp.mid(7,4);
        memcpy(&MyData::currentK,k,4);
        memcpy(&MyData::currentB,b,4);
        MyData::currentK = QString::number(MyData::currentK,'f',3).toFloat();
        MyData::currentB = QString::number(MyData::currentB,'f',3).toFloat();
        qDebug()<<"k值："<<MyData::currentK<<"   b值："<<MyData::currentB;
        emit cal_param_ok(MyData::currentK,MyData::currentB,0);
    }
    else {
        getSN(real_id,1,type);
    }
}


/*开启禹山探头刷子*/
void MyThread::openBrush(uint8_t real_id){
    QByteArray array;
    QString info;
    uint8_t data[9] = {real_id,0x10,0x31,0x00,0x00,0x00,0x00};
    uint16_t tmp = GetCRC16(data,7);   //获取CRC校验码
    data[7] = tmp>>8;
    data[8] = tmp & 0xff;
    array.resize(sizeof(data));
    memcpy(array.data(), data, sizeof(data));  //将uint8类型数据转换为qByteArray
    QByteArray hexArray = array.toHex();
    info.prepend(hexArray);
    port->readAll();
    sendInfo(info);
    sleep(100);
    handle_data();
}

/**
 * @brief MyThread::openNaHongBrush 打开纳宏探头刷子，目前只有水中油
 * @param real_id
 * @param mode  清扫模式 0：自动清扫 1：手动清扫  2：手动转刷一次
 */
void MyThread::openNaHongBrush(unsigned char real_id ,int mode){
    uint8_t *data;
    qDebug()<<"设置清扫模式:"<<mode;
    if(mode == 0){      //自动清扫
         qDebug()<<"设置清扫自动模式:"<<mode;
        data = new uint8_t[11]{real_id,0x10,00,0x08,00,01,02,00,00};
        command(data,11,8,1,3,"BX-Oil-1N");
        delete []data;

    }
    else if (mode == 1) {   //手动清扫
        data = new uint8_t[11]{real_id,0x10,00,0x08,00,01,02,00,01};
        if(command(data,11,8,1,3,"BX-Oil-1N") != nullptr){
//            openNaHongBrush(real_id,mode);
            delete [] data;
//            data = new uint8_t[11]{real_id,0x10,00,0x08,00,01,02,00,02};
//            command(data,11,8,1,3);
//            delete [] data;
        }
        else {
           delete [] data;
        }

    }
    else  if(mode == 2){
        data = new uint8_t[11]{real_id,0x10,00,0x08,00,01,02,00,02};
        command(data,11,8,1,3,"BX-Oil-1N");
        delete [] data;
    }

}


/**
 * @brief MyThread::setOilClearNum  设置水中油清扫次数
 * @param real_id
 */
void MyThread::setOilClearNum(unsigned char real_id,unsigned char num){
    qDebug()<<"设置清扫次数:"<<num;
    uint8_t data[11] = {real_id,0x10,0x00,0x09,0x00,0x01,0x02,0x00,num};
    if(command(data,11,8,1,3,"BX-Oil-1N") != nullptr){
        emit oil_clear_num_ok(num,true);
        return;
    }
    emit oil_clear_num_ok(1,false);
}

/**
 * @brief openSondeBrush    打开多合一探头刷子
 * @param real_id           探头id
 */
void MyThread::openSondeBrush(uint8_t real_id){
    QByteArray array;
    QString info;
    uint8_t data[9] = {real_id,0x10,0x2f,0x00,0x00,0x00,0x00};
    uint16_t tmp = GetCRC16(data,7);   //获取CRC校验码
    data[7] = tmp>>8;
    data[8] = tmp & 0xff;
    array.resize(sizeof(data));
    memcpy(array.data(), data, sizeof(data));  //将uint8类型数据转换为qByteArray
    QByteArray hexArray = array.toHex();
    info.prepend(hexArray);
    port->readAll();
    sendInfo(info);
    sleep(100);
    handle_data();
}


/**获取自清洁浊度探头浊度和温度命令*/
float MyThread::getTempAndTurb(unsigned char real_id,QString type){
    QByteArray temp,turb;
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x05};
    QByteArray tmp = command(data,8,15,1,3,type);
    if(tmp != nullptr){      
        temp.resize(4);
        temp.resize(4);
        temp = tmp.mid(3,4);
        turb = tmp.mid(7,4);
        memcpy(&MyData::turbTemp,temp.data(),4);
        memcpy(&MyData::turbTurb,turb.data(),4);
        MyData::turbTurb = QString::number(MyData::turbTurb,'f',1).toFloat();
        MyData::turbTemp = QString::number(MyData::turbTemp,'f',1).toFloat();
        emit data_type(tmp,type);   //发送信号，更新界面数据
        turbCount = 0;
        emit changeStatus(type,"online");
        return MyData::turbTurb;
    }
    else {
        turbCount++;
        if (turbCount >= 3) {
            emit changeStatus(type,"drop");
            turbCount == 3;
        }
        return -1;
    }
}

/*获取DO溶解氧参数*/
float MyThread::getTempAndOxygen(unsigned char real_id,QString type){
    QByteArray temp,ldo;
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x04};
    QByteArray tmp = command(data,8,13,1,3,type);
    if(tmp != nullptr){
        temp.resize(4);
        temp.resize(4);
        temp = tmp.mid(3,4);
        ldo = tmp.mid(7,4);
        memcpy(&MyData::ldoTemp,temp.data(),4);
        memcpy(&MyData::ldoLdo,ldo.data(),4);
        MyData::ldoSaturation = MyData::ldoLdo * 100;
        MyData::ldoLdo = do_value_switch(MyData::ldoTemp,MyData::ldoLdo);   //溶解氧需要经过计算        
        MyData::ldoTemp = QString::number(MyData::ldoTemp,'f',1).toFloat();
        MyData::ldoLdo = QString::number(MyData::ldoLdo,'f',2).toFloat();
        MyData::ldoSaturation = QString::number(MyData::ldoSaturation,'f',2).toFloat();
        emit data_type(tmp,type);
        ldoCount = 0;
        emit changeStatus(type,"online");
        return MyData::ldoLdo;
    }
    else {
        ldoCount++;
        if(ldoCount >= 3){
            emit changeStatus(type,"drop");
            ldoCount = 3;
        }
        return -1;
    }
}

/*获取叶绿素参数*/
float MyThread::getChla(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x05};
    QByteArray tmp;
    QByteArray temp,chla;
    tmp.clear();
    tmp = command(data,8,15,1,3,type);
    if(tmp != nullptr){
        temp.resize(4);
        chla.resize(4);
        temp = tmp.mid(3,4);
        chla = tmp.mid(7,4);
        memcpy(&MyData::chlaTemp,temp.data(),4);
        memcpy(&MyData::chlaChla,chla.data(),4);
        MyData::chlaTemp = QString::number(MyData::chlaTemp,'f',1).toFloat();
        MyData::chlaChla = QString::number(MyData::chlaChla,'f',1).toFloat();

        emit data_type(tmp,type);
        chlaCount = 0;
        emit changeStatus(type,"online");
        return MyData::chlaChla;
    }
    else {
        chlaCount++;
        if(chlaCount >= 3){
            emit changeStatus(type,"drop");
            chlaCount = 3;
        }
        return -1;
    }
}


/**
 * @brief MyThread::getTempAndConductance   获取电导率探头数据
 * @param real_id   探头id
 * @param type      返回探头类型
 */
float MyThread::getTempAndConductance(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x05};
    QByteArray tmp;
    QByteArray temp,cond;
    tmp.clear();
    tmp = command(data,8,15,1,10,type);
    if(tmp != nullptr){
        temp.resize(4);
        cond.resize(4);
        temp = tmp.mid(3,4);
        cond = tmp.mid(7,4);
        memcpy(&MyData::condTemp,temp.data(),4);
        memcpy(&MyData::condCond,cond.data(),4);
        MyData::condCond*=1000;
        MyData::condTemp = QString::number(MyData::condTemp,'f',1).toFloat();
        MyData::condCond = QString::number(MyData::condCond,'f',1).toFloat();

        emit data_type(tmp,type);
        sleep(50);
        condCount = 0;
        emit changeStatus(type,"online");
        return MyData::condCond;
    }
    else {
        condCount++;
        if(condCount >= 3){
            emit changeStatus(type,"drop");
            condCount = 3;
        }
        return -1;
    }
}


/**
 * @brief getSondeData  获取多合一探头数据
 * @param real_id       探头id
 * @param type          返回数据类型
 */
float MyThread::getSondeData(unsigned char real_id,QString type,QString param){
    uint8_t data[8] = {real_id,0x03,0x26,0x01,0x00,0x10};
    QByteArray tmp;
    QByteArray ldo,turb,cond,ph,chla,bga,orp,temp;
    tmp.clear();
    tmp = command(data,8,37,1,3,type);
    if(tmp != nullptr){
        ldo.resize(4);
        turb.resize(4);
        cond.resize(4);
        ph.resize(4);
        temp.resize(4);
        orp.resize(4);
        chla.resize(4);
        bga.resize(4);

        ldo = tmp.mid(3,4);
        turb = tmp.mid(7,4);
        cond = tmp.mid(11,4);
        ph = tmp.mid(15,4);
        temp = tmp.mid(19,4);
        orp = tmp.mid(23,4);
        chla = tmp.mid(27,4);
        bga = tmp.mid(31,4);

        memcpy(&MyData::sondeLdo,ldo.data(),4);
        memcpy(&MyData::sondeTurb,turb.data(),4);
        memcpy(&MyData::sondeCond,cond.data(),4);
        memcpy(&MyData::sondePH,ph.data(),4);
        memcpy(&MyData::sondeTemp,temp.data(),4);
        memcpy(&MyData::sondeOrp,orp.data(),4);
        memcpy(&MyData::sondeChla,chla.data(),4);
        memcpy(&MyData::sondeBga,bga.data(),4);
        MyData::sondeCond*=1000;                    //电导率要*1000

        MyData::sondeLdo = QString::number(MyData::sondeLdo,'f',2).toFloat();
        MyData::sondeTurb = QString::number(MyData::sondeTurb,'f',1).toFloat();
        MyData::sondeCond = QString::number(MyData::sondeCond,'f',1).toFloat();
        MyData::sondePH = QString::number(MyData::sondePH,'f',2).toFloat();
        MyData::sondeTemp = QString::number(MyData::sondeTemp,'f',1).toFloat();
        MyData::sondeOrp = QString::number(MyData::sondeOrp,'f',1).toFloat();
        MyData::sondeChla = QString::number(MyData::sondeChla,'f',1).toFloat();
        MyData::sondeBga = QString::number(MyData::sondeBga,'f',0).toFloat();


        emit data_type(tmp,type);
        emit changeStatus(type,"online");

        if(param == "ldo"){
            return MyData::sondeLdo;
        }else if (param == "turb") {
            return  MyData::sondeTurb;
        }else if (param == "cond") {
            return  MyData::sondeCond;
        }
        else if (param == "ph") {
            return  MyData::sondePH;
        }
        else if (param == "orp") {
            return  MyData::sondeOrp;
        }
        else if (param == "chla") {
            return  MyData::sondeChla;
        }
        else if (param == "bga") {
            return  MyData::sondeBga;
        }
    }
    else {
        sondeCount++;
        if(sondeCount >= 3){
            emit changeStatus(type,"drop");
            sondeCount = 3;
        }
        return -1;
    }
}


/*获取电位值和PH值*/
float MyThread::getOrpAndPh(unsigned char real_id,QString type){
    QByteArray orp,ph;
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x04};
    QByteArray tmp = command(data,8,13,1,3,type);
    if(tmp != nullptr){
        orp.resize(4);
        ph.resize(4);
        orp = tmp.mid(3,4);
        ph = tmp.mid(7,4);
        memcpy(&MyData::nhOrp,orp.data(),4);
        memcpy(&MyData::nhPH,ph.data(),4);

        MyData::nhOrp = QString::number(MyData::nhOrp,'f',1).toFloat();
        MyData::nhPH = QString::number(MyData::nhPH,'f',2).toFloat();
        return MyData::nhPH;
    }
    else {
        return -1;
    }
}

/*获取NH4+和K+值*/
float MyThread::getNH4plusAndKplus(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x37,0x00,0x00,0x04};
    QByteArray tmp = command(data,8,13,1,3,type);
    QByteArray nhNh4Plus,kPlus;
    if(tmp != nullptr){
        nhNh4Plus.resize(4);
        kPlus.resize(4);
        nhNh4Plus = tmp.mid(3,4);
        kPlus = tmp.mid(7,4);
        memcpy(&MyData::nhNh4Plus,nhNh4Plus.data(),4);
        memcpy(&MyData::nhKPlus,kPlus.data(),4);

        MyData::nhNh4Plus = QString::number(MyData::nhNh4Plus,'f',1).toFloat();
        MyData::nhKPlus = QString::number(MyData::nhKPlus,'f',1).toFloat();
        return MyData::nhNh4Plus;
    }
    else {
        return -1;
    }
}

float MyThread::getNH4_N(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x28,0x00,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray nhNh4_N;
    if(tmp != nullptr){
        nhNh4_N.resize(4);
        nhNh4_N = tmp.mid(3,4);
        memcpy(&MyData::nhNh4_N,nhNh4_N.data(),4);

        MyData::nhNh4_N = QString::number(MyData::nhNh4_N,'f',2).toFloat();
        emit changeStatus(type,"online");
        nhCount = 0;
        return MyData::nhNh4_N;
    }
    else {
        nhCount++;
        if(nhCount >= 3){
            emit changeStatus(type,"drop");
            nhCount = 3;
        }

        return -1;
    }
}

/**
 * @brief MyThread::readOrpAndPhOfPH    读取PH探头的orp值和ph值
 * @param real_id
 * @param type
 * @return
 */
float MyThread::readOrpAndPhOfPH(unsigned char real_id, QString type){
    QByteArray orp,ph;
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x04};
    QByteArray tmp = command(data,8,13,1,3,type);
    if(tmp != nullptr){
        orp.resize(4);
        ph.resize(4);
        orp = tmp.mid(3,4);
        ph = tmp.mid(7,4);
        memcpy(&MyData::phOrp,orp.data(),4);
        memcpy(&MyData::phPH,ph.data(),4);

        MyData::phOrp = QString::number(MyData::phOrp,'f',1).toFloat();
        MyData::phPH = QString::number(MyData::phPH,'f',2).toFloat();
        emit changeStatus(type,"online");
        phCount = 0;
        return MyData::phPH;
    }
    else {
        phCount++;
        if(phCount >= 3){
            emit changeStatus(type,"drop");
            phCount = 3;
        }
        return -1;
    }
}


/**
 * @brief MyThread::getPHTemp   获取氨氮探头的溶液温度值
 * @param real_id
 * @param type
 */
void MyThread::getPHTemp(unsigned char real_id, QString type){
    uint8_t data[8] = {real_id,0x03,0x24,0x00,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray temp;
    if(tmp != nullptr){
        temp.resize(4);
        temp = tmp.mid(3,4);
        memcpy(&MyData::phTemp,temp.data(),4);
        MyData::phTemp = QString::number(MyData::phTemp,'f',1).toFloat();
        emit data_type(tmp,type);


        if(DBUtil::sqlServerIsOpen()){      //如果数据库是出于连接状态，则保存数据
            QDateTime current_date_time;
            QString current_date;
            current_date_time =QDateTime::currentDateTime();
            current_date =current_date_time.toString("yyyy/MM/dd hh:mm:00");
        }
    }
}

/**
 * @brief MyThread::readOrp 获取orp探头的orp值
 * @param real_id
 * @param type
 * @return
 */
float MyThread::readOrp(unsigned char real_id, QString type){
    uint8_t data[8] = {real_id,0x03,0x12,0x00,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray orp;
    if(tmp != nullptr){
        orp.resize(4);
        orp = tmp.mid(3,4);
        memcpy(&MyData::orpOrp,orp.data(),4);
        MyData::orpOrp = QString::number(MyData::orpOrp,'f',1).toFloat();
        //emit data_type(tmp,type);
        orpCount = 0;
        emit changeStatus(type,"online");
        return MyData::orpOrp;
    }
    else {
        orpCount++;
        if(orpCount >= 3){
            emit changeStatus(type,"drop");
            orpCount =3;
        }
        return -1;
    }
}


/**
 * @brief MyThread::getOrpTemp  获取orp探头的温度值
 * @param real_id
 * @param type
 */
void MyThread::getOrpTemp(unsigned char real_id, QString type){
    uint8_t data[8] = {real_id,0x03,0x24,0x00,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray temp;
    if(tmp != nullptr){
        temp.resize(4);
        temp = tmp.mid(3,4);
        memcpy(&MyData::orpTemp,temp.data(),4);
        MyData::orpTemp = QString::number(MyData::orpTemp,'f',1).toFloat();
        emit data_type(tmp,type);
    }
}


/**
 * @brief MyThread::getNhTemp   获取氨氮探头测量的溶液温度值
 * @param real_id
 * @param type
 */
void MyThread::getNhTemp(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x24,0x00,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray temp;
    if(tmp != nullptr){
        temp.resize(4);
        temp = tmp.mid(3,4);
        memcpy(&MyData::nhTemp,temp.data(),4);
        MyData::nhTemp = QString::number(MyData::nhTemp,'f',1).toFloat();
        emit data_type(tmp,type);
    }
}

/*获取蓝绿藻探头数据*/
float MyThread::getTempAndBga(unsigned char real_id,QString type){
    QByteArray temp,bga;
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x05};
    QByteArray tmp = command(data,8,15,1,3,type);
    if(tmp != nullptr){
        temp.resize(4);
        bga.resize(4);
        temp = tmp.mid(3,4);
        bga = tmp.mid(7,4);
        memcpy(&MyData::bgaTemp,temp.data(),4);
        memcpy(&MyData::bgaBga,bga.data(),4);

        MyData::bgaTemp = QString::number(MyData::bgaTemp,'f',1).toFloat();
        MyData::bgaBga = QString::number(MyData::bgaBga,'f',0).toFloat();

        emit data_type(tmp,type);
        bgacount = 0;
        emit changeStatus(type,"online");
        return MyData::bgaBga;
    }
    else {
        bgacount++;
        if(bgacount >= 3){
            emit changeStatus(type,"drop");
            bgacount = 3;
        }
        return -1;
    }
}



/*获取纳宏水中油参数与温度*/
void MyThread::getTempAndOil(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x00,0x01,0x00,0x04};
    QByteArray tmp = command(data,8,13,1,3,type);
    QByteArray toLarge;                         //从小端格式转到大端格式
    if(tmp != nullptr){       
        toLarge.resize(13);
        for (int i = 0,j = 10; i < 11; i++) {
            if(i < 3){
              toLarge[i] = tmp[i];
            }
            else {
                toLarge[i] = tmp[j--];
            }
        }
        toLarge[11] = tmp[11];
        toLarge[12] = tmp[12];
        memcpy(&MyData::oilOil,toLarge.mid(7,4).data(),4);
        memcpy(&MyData::oilTemp,toLarge.mid(3,4).data(),4);

        MyData::oilOil = QString::number(MyData::oilOil,'f',2).toFloat();
        MyData::oilTemp = QString::number(MyData::oilTemp,'f',1).toFloat();
        emit data_type(toLarge,type);
        oilCount = 0;
        emit changeStatus(type,"online");
    }
    else {

        oilCount++;
        qDebug()<<"查找水中油错误次数："<<oilCount;
        if(oilCount >= 3){
            emit changeStatus(type,"drop");
            oilCount = 3;
        }
        return;
    }
}

/**
 * @brief MyThread::getOiwAndTemp   获取纳宏水中油
 * @param real_id
 * @param type
 * @return
 */
float MyThread::getOiwAndTemp(unsigned char real_id, QString type){
    QByteArray temp,oiw;
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x05};
    QByteArray tmp = command(data,8,15,1,3,type);
    if(tmp != nullptr){
        temp.resize(4);
        temp.resize(4);
        temp = tmp.mid(3,4);
        oiw = tmp.mid(7,4);
        memcpy(&MyData::oilTemp,temp.data(),4);
        memcpy(&MyData::oilOil,oiw.data(),4);
        MyData::oilOil = QString::number(MyData::oilOil,'f',2).toFloat();
        MyData::oilTemp = QString::number(MyData::oilTemp,'f',1).toFloat();
        emit data_type(tmp,type);   //发送信号，更新界面数据
        turbCount = 0;
        emit changeStatus(type,"online");
        return MyData::oilOil;
    }
    else {
        turbCount++;
        if (turbCount >= 3) {
            emit changeStatus(type,"drop");
            turbCount = 3;
        }
        return -1;
    }
}

/**
 * @brief MyThread::getTempAndCodAndToc 读取COD探头的COD，TOC和温度参数
 * @param real_id
 * @param type
 */
float MyThread::getTempAndCodAndToc(unsigned char real_id,QString type){
    uint8_t data[8] = {real_id,0x03,0x26,0x00,0x00,0x07};
    QByteArray tmp;
    QByteArray temp,cod,toc;
    tmp.clear();
    tmp = command(data,8,19,1,3,type);
    if(tmp != nullptr){
        temp.resize(4);
        cod.resize(4);
        toc.resize(4);
        temp = tmp.mid(3,4);
        cod = tmp.mid(7,4);
        toc = tmp.mid(11,4);
        memcpy(&MyData::codTemp,temp.data(),4);
        memcpy(&MyData::codCod,cod.data(),4);
        memcpy(&MyData::codToc,toc.data(),4);
        MyData::codTemp = QString::number(MyData::codTemp,'f',1).toFloat();
        MyData::codCod = QString::number(MyData::codCod,'f',2).toFloat();
        MyData::codToc = QString::number(MyData::codToc,'f',1).toFloat();
        return MyData::codCod;
    }
    else {
        return -1;
    }
}


/**
 * @brief MyThread::getTurbOfCod    读取COD探头的浊度参数
 * @param real_id
 * @param type
 */
float MyThread::getTurbOfCod(unsigned char real_id, QString type){
    uint8_t data[8] = {real_id,0x03,0x12,0x00,0x00,0x02};
    QByteArray tmp;
    QByteArray turb;
    tmp.clear();
    tmp = command(data,8,10,1,3,type);
    if(tmp != nullptr){
        turb.resize(4);
        turb = tmp.mid(3,4);
        memcpy(&MyData::codTurb,turb.data(),4);
        MyData::codTurb = QString::number(MyData::codTurb,'f',1).toFloat();
        emit data_type(tmp,type);
        codCount = 0;
        emit changeStatus(type,"online");
        return MyData::codTurb;
    }
    else {
        codCount++;
        if(codCount >= 3){
            emit changeStatus(type,"drop");
            codCount = 3;
        }
        return -1;
    }
}


/*获取SN*/
QByteArray MyThread::getSN(uint8_t real_id,unsigned char count,QString type){
    uint8_t data[8] = {real_id,0x03,0x09,0x00,0x00,0x07};
    QByteArray array;
    QString info;
    unsigned char recLen = 19;
    unsigned char len = 8;
    uint8_t recData[19];
    uint16_t tmp = GetCRC16(data,len-2);   //获取CRC校验码
    data[len-2] = tmp>>8;
    data[len-1] = tmp & 0xff;

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    port->readAll();
    sendInfo(info);
    sleep(300);
    if(condDelay){
        sleep(150);
    }
    handle_data();                            //读取下位机返回的数据
    memcpy(recData,hexData.data(),recLen);   //将QByteArray 转换成uint8类型的数组
    tmp = GetCRC16(recData,recLen-2);
    if(recData[recLen-2] == (tmp>>8) && recData[recLen-1] == (tmp & 0xff)){
        return hexData;
    }
    else if (count <= 2) {       //查询10次不成功则认为该探头已经掉线或损坏
        //出错的话则从数据库查询探头id,防止是由于修改id而导致收不到数据
        QSqlQuery query(DBUtil::getSqLite());
        query.prepare("select slave_id from probeInfo where sn = ? and status = 'online'");
        query.addBindValue(type);
        query.exec();
        while (query.next()) {
            real_id = query.value(0).toInt();
        }
        qDebug()<<"第"<<count<<"次查找"<<readType<<" SN,发送数据："<<info<<"  ,返回数据："<<hexData.toHex();
        count++;
        return getSN(real_id,count,type);
    }
    qDebug()<<"已查询10次查找"<<readType<<" SN，未找到，请确认探头存在或完整";
    //QSqlQuery update(DBUtil::getSqLite());
    //update.exec("update probeInfo set status = 'drop' where slave_id = "+QString::number(real_id));
    return nullptr;

}

/*获取纳宏设备名称*/
QByteArray MyThread::getNaHongName(uint8_t real_id,unsigned char count){
    uint8_t data[8] = {real_id, 0x03,0x00, 0x10, 0x00, 0x08};
    QByteArray array;
    QByteArray name;
    QString info;
    unsigned char recLen = 21;
    unsigned char len = 8;
    uint8_t recData[21];
    uint16_t tmp = GetCRC16(data,len-2);   //获取CRC校验码
    data[len-2] = tmp>>8;
    data[len-1] = tmp & 0xff;

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    port->readAll();
    sendInfo(info);
    sleep(500);
    handle_data();                            //读取下位机返回的数据
    memcpy(recData,hexData.data(),recLen);   //将QByteArray 转换成uint8类型的数组
    tmp = GetCRC16(recData,recLen-2);
    if(recData[recLen-2] == (tmp>>8) && recData[recLen-1] == (tmp & 0xff)){
        name.resize(16);
        for (int i = 0; i < 16; i++) {
            name[i] = hexData[i + 3];
        }
        return name;
    }
    else if (count < 2) {       //查询3次不成功则认为该探头不存在
        qDebug()<<"第"<<QString::number(count)<<"查询"<<readType<<"SN 错误，发送数据"<<info<<"  ,返回数据："<<hexData.toHex();
        count++;
        return getNaHongName(real_id,count);
    }
    else {
        qDebug()<<"第2"<<"查询"<<readType<<"SN 错误，请确认探头是否存在或完好";
        return nullptr;
    }
}

/*获取纳宏SN*/
QByteArray MyThread::getNaHongSN(uint8_t id_real,unsigned char count){
    uint8_t data[8] = {id_real, 0x03,0x00, 0x18, 0x00, 0x08};
    return command(data,8,21,1,3,"");
    //return nullptr;
}

/**
 * @brief getNaHongDevType   获取纳宏探头的设备类型
 * @param real_id
 * @return
 */
QByteArray MyThread::getNaHongDevType(uint8_t real_id,unsigned char count){
    uint8_t data[8] = {real_id, 0x03,0x00, 0x25, 0x00, 0x01};
    QByteArray array;
    QByteArray devType;
    QString info;
    unsigned char recLen = 7;
    unsigned char len = 8;
    uint8_t recData[7] = {};                //0x0a,0x03,0x02,0x03,0,0x1d,0x75
    uint16_t tmp = GetCRC16(data,len-2);    //获取CRC校验码
    data[len-2] = tmp>>8;
    data[len-1] = tmp & 0xff;

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    port->readAll();
    sendInfo(info);
    sleep(500);
    handle_data();                            //读取下位机返回的数据
    memcpy(recData,hexData.data(),recLen);   //将QByteArray 转换成uint8类型的数组
    tmp = GetCRC16(recData,recLen-2);
    if(recData[recLen-2] == (tmp>>8) && recData[recLen-1] == (tmp & 0xff)){
        devType.resize(2);
        devType[0] = recData[3];
        devType[1] = recData[4];
        return devType;
    }
    else if (count < 2) {       //查询3次不成功则认为该探头不存在
        qDebug()<<"第"<<QString::number(count)<<"查询"<<readType<<"SN 错误，发送数据"<<info<<"  ,返回数据："<<hexData.toHex();
        count++;
        return getNaHongDevType(real_id,count);
    }
    else {
        qDebug()<<"第2"<<"查询"<<readType<<"SN 错误，请确认探头是否存在或完好";
        return nullptr;
    }
    return command(data,8,7,1,3,"");
}



float MyThread::getNaNhNH4_N(uint8_t real_id,unsigned char count,QString type){
    uint8_t data[8] = {real_id,0x03,0x00,0x03,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray toLarge;                         //从小端格式转到大端格式
    if(tmp != nullptr){
        toLarge.resize(4);
        toLarge[0] = tmp[6];
        toLarge[1] = tmp[5];
        toLarge[2] = tmp[4];
        toLarge[3] = tmp[3];
        memcpy(&MyData::nhNh4_N,toLarge.data(),4);

        MyData::nhNh4_N = QString::number(MyData::nhNh4_N,'f',2).toFloat();
        emit data_type(toLarge,type);
        //oilCount = 0;
        emit changeStatus(type,"online");
        return MyData::nhNh4_N;
    }
    else {

        //oilCount++;
        qDebug()<<"查找水中油错误次数："<<oilCount;
//        if(oilCount >= 3){
//            emit changeStatus(type,"drop");
//            oilCount = 3;
//        }
        return -1;
    }
}



void MyThread::getNaNhpH(uint8_t real_id,unsigned char count,QString type){
    uint8_t data[8] = {real_id,0x03,0x00,0x01,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray toLarge;                         //从小端格式转到大端格式
    if(tmp != nullptr){
        toLarge.resize(4);
        toLarge[0] = tmp[6];
        toLarge[1] = tmp[5];
        toLarge[2] = tmp[4];
        toLarge[3] = tmp[3];
        memcpy(&MyData::nhPH,toLarge.data(),4);

        MyData::nhPH = QString::number(MyData::nhPH,'f',2).toFloat();
        emit data_type(toLarge,type);
        //oilCount = 0;
        emit changeStatus(type,"online");
    }
    else {

//        oilCount++;
//        qDebug()<<"查找水中油错误次数："<<oilCount;
//        if(oilCount >= 3){
//            emit changeStatus(type,"drop");
//            oilCount = 3;
//        }
    }
}


void MyThread::getNaNhTemp(uint8_t real_id,unsigned char count,QString type){
    uint8_t data[8] = {real_id,0x03,0x00,0x09,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,type);
    QByteArray toLarge;                         //从小端格式转到大端格式
    if(tmp != nullptr){
        toLarge.resize(4);
        toLarge[0] = tmp[6];
        toLarge[1] = tmp[5];
        toLarge[2] = tmp[4];
        toLarge[3] = tmp[3];
        memcpy(&MyData::nhTemp,toLarge.data(),4);

        MyData::nhTemp = QString::number(MyData::nhTemp,'f',2).toFloat();
        emit data_type(toLarge,type);
        //oilCount = 0;
        emit changeStatus(type,"online");
    }
    else {

//        oilCount++;
//        qDebug()<<"查找水中油错误次数："<<oilCount;
//        if(oilCount >= 3){
//            emit changeStatus(type,"drop");
//            oilCount = 3;
//        }
    }
}


/**
 * @brief MyThread::getSNSignal 获取显示到用户界面的序列号
 * @param real_id
 * @param count
 * @param type
 */
void MyThread::getSNSignal(unsigned char real_id,QString type){
    QByteArray sn;
    if(type == "naHong"){
        sn = getNaHongSN(real_id,1);
        qDebug()<<"水中油sn1:"<<QString(sn);
        sn = sn.mid(5,13);
        qDebug()<<"水中油sn2:"<<QString(sn);
        emit get_sn_ok("BX"+QString(sn));
    }
    else if (type == "sonde") {
        sn = getSondeSN(real_id,1);
        sn = sn.mid(5,10);
        qDebug()<<"BX"+QString(sn);
        emit get_sn_ok("BX"+QString(sn));
    }
    else {
        sn = getSN(real_id,1,type);
        sn = sn.mid(6,10);
        emit get_sn_ok("BX"+QString(sn));
    }
}

/*获取软硬件版本号*/
QByteArray MyThread::getVersion(){
    uint8_t data[8] = {slaveId,0x03,0x07,0x00,0x00,0x02};
    QByteArray tmp = command(data,8,9,1,3,"");
    return tmp;
}

QByteArray MyThread::getNaHongVersion(){
    uint8_t data[8] = {slaveId,03,00,0x20,00,03};
    QByteArray tmp = command(data,8,11,1,3,"");
    return tmp;
}

void MyThread::setSlaveId(unsigned char id_update,unsigned char real_id,QString type){
    QByteArray tmp;
    uint8_t *data;
    if(!QString::compare(MyData::oilSn,type)){       //纳宏公司探头
        if(MyData::oilType == "oil"){
            tmp = getNaHongName(real_id,1);
            data = new uint8_t[11]{real_id,0x10,0x00,0x23,0x00,0x01,0x02,0x00,id_update};
            qDebug()<<"修改纳宏水中油探头id：sn = "<<type;
        }
        else {
            tmp = getSN(real_id,1,type);
            data = new uint8_t[11]{real_id,0x10,0x30,0x00,0x00,0x01,0x02,id_update,0x00};
            type = MyData::oiwSn;
            qDebug()<<"修改虞山水中油探头id：sn = "<<type;
        }
    }
    else if(!QString::compare(MyData::nhSn,type)){       //纳宏公司探头
        if(MyData::nhType == "nhn"){
            tmp = getNaHongName(real_id,1);
            data = new uint8_t[11]{real_id,0x10,0x00,0x23,0x00,0x01,0x02,0x00,id_update};
            qDebug()<<"修改纳宏氨氮探头id：sn = "<<type;
        }
        else {
            tmp = getSN(real_id,1,type);
            data = new uint8_t[11]{real_id,0x10,0x30,0x00,0x00,0x01,0x02,id_update,0x00};
            type = MyData::oiwSn;
            qDebug()<<"修改虞山氨氮探头id：sn = "<<type;
        }
    }
    else {                                  //虞山公司探头
        tmp = getSN(real_id,1,type);
        data = new uint8_t[11]{real_id,0x10,0x30,0x00,0x00,0x01,0x02,id_update,0x00};
        qDebug()<<"修改虞山探头id：sn = "<<type;
    }
    if(tmp != nullptr){
        QSqlQuery update(DBUtil::getSqLite());
        if(MyData::idRepeat){       //允许修改为重复的值，方便修改探头数据
            update.prepare("update probeInfo set slave_id = ? where sn = ?");
            update.addBindValue(QString::number(id_update));
            update.addBindValue(type);
            //update.addBindValue(QString::number(id_update));
        }
        else {      //不允许修改为重复的值
            update.prepare("update probeInfo set slave_id = ? where sn = ? and ? "
                           "not in(select slave_id from probeInfo)");
            update.addBindValue(QString::number(id_update));
            update.addBindValue(type);
            update.addBindValue(QString::number(id_update));
        }
        if(update.exec()){
            if(update.numRowsAffected() == 1){
                tmp = command(data,11,8,1,3,type);  //10代表接收的数据是设置从机id返回的信息
                if(tmp != nullptr){
                    emit id_ok(id_update);
                }
                else {
                    emit id_ok(-1);     //修改失败,地址与其他地址重复
                    update.prepare("update probeInfo set slave_id = ? where sn = ?");
                    update.addBindValue(QString::number(real_id));
                    update.addBindValue(type);
                }
            }
            else {
                emit id_ok(-1);     //修改失败,地址与其他地址重复
            }
        }

    }else{
        emit id_ok(0);          //探头以掉线或损坏，地址不正确
    }
    delete [] data;
}

/*获取所有虞山探头的id*/
QByteArray MyThread::getSlaveId(){
    QByteArray array;
    QByteArray arrayId;
    QByteArray arrayCRC;
    unsigned char len = 8;
    unsigned char recLen = 7;
    QString info;
    unsigned char recData[7];
    unsigned short tmp;
    int size;
    uint8_t data[8] = {0xFF,0x03,0x30,0x00,0x00,0x01,0x9e,0xd4};

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    port->readAll();
    sendInfo(info);
    sleep(200);
    handle_data();                            //读取下位机返回的数据
    if(hexData.size()%7 == 0){
       size = hexData.size()/7;
       arrayCRC.resize(7);
       for (int i = 0; i < size; i++) {
           for (int j = 0; j < 7; j++) {
               arrayCRC[j] = hexData[j+7*i];
           }
           memcpy(recData,arrayCRC.data(),recLen);
           tmp = GetCRC16(recData,recLen-2);
           if(recData[recLen-2] != (tmp>>8) || recData[recLen-1] != (tmp & 0xff)){
               getSlaveId();    //只要存在错误就重发
               return nullptr;
           }
       }
       arrayId.resize(size);    //长度与校验码都正确将保存到QByteArray里
       for(int i = 0; i < size; i++){
           arrayId[i] = hexData[i*7 + 3];
       }
        qDebug()<<"所有探头id:  "<<arrayId.toHex();
        return arrayId;

    }
    return nullptr;
}

/*获取纳宏公司的探头地址*/
QByteArray MyThread::getNaHongSlaveId(){
    uint8_t data[8] = {0x00, 0x03, 0x00, 0x23, 0x00, 0x01, 0x74, 0x11};
    QByteArray array;
    QByteArray arrayId;
    QByteArray arrayCRC;
    unsigned char len = 8;
    unsigned char recLen = 7;
    QString info;
    unsigned char recData[7];
    unsigned short tmp;
    int size;

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    port->readAll();
    sendInfo(info);
    sleep(200);
    handle_data();                            //读取下位机返回的数据
    if(hexData.size()%7 == 0){
       size = hexData.size()/7;
       arrayCRC.resize(7);
       for (int i = 0; i < size; i++) {
           for (int j = 0; j < 7; j++) {
               arrayCRC[j] = hexData[j+7*i];
           }
           memcpy(recData,arrayCRC.data(),recLen);
           tmp = GetCRC16(recData,recLen-2);
           if(recData[recLen-2] != (tmp>>8) || recData[recLen-1] != (tmp & 0xff)){
               getSlaveId();    //只要存在错误就重发
               return nullptr;
           }
       }
       arrayId.resize(size);    //长度与校验码都正确将保存到QByteArray里
       for(int i = 0; i < size; i++){
           arrayId[i] = hexData[i*7 + 4];
       }
        qDebug()<<"纳宏探头id:  "<<arrayId.toHex();
        return arrayId;

    }
    return nullptr;
}

/**
 * @brief MyThread::setCalParameter 设置校准参数
 * @param real_id
 * @param k
 * @param b         true：校准成功   false：校准失败
 */
void MyThread::setCalParameter(unsigned char real_id,float k,float b,unsigned char addr,unsigned char len){
    union{
        float fk;
        uint8_t ck[4];
    }kk;
    union{
        float fb;
        uint8_t cb[4];
    }bb;
    kk.fk = k;
    bb.fb = b;
    uint8_t num= len*2;
    uint8_t data[17] = {real_id,0x10,addr,0x00,0x00,len,num,kk.ck[0],kk.ck[1],kk.ck[2],kk.ck[3],bb.cb[0],bb.cb[1],bb.cb[2],bb.cb[3]};

    if(MyData::isSondeCal){
        MyData::isSondeCal = false;
        data[2] = 0x11;
        if(MyData::calParam == "ldo"){
            data[3] = 0x01;
        }
        else if (MyData::calParam == "turb") {
            data[3] = 0x02;
        }
        else if (MyData::calParam == "cond") {
            data[3] = 0x03;
            qDebug()<<"多合一电导率校准";
        }
        else if (MyData::calParam == "ph") {
            data[3] = 0x04;
        }
        else if (MyData::calParam == "orp") {
            data[3] = 0x05;
        }
        else if (MyData::calParam == "chla") {
            data[3] = 0x06;
        }
        else if (MyData::calParam == "bga") {
            data[3] = 0x07;
        }
    }

    if(command(data,17,8,1,3,"")!= nullptr){
        qDebug()<<"校准K值："<<k<<"   b:"<<b;
        emit cal_param_ok(k,b,1);
    }
    else {
        emit cal_param_ok(1,0,-1);
    }
}

/**
 * @brief MyThread::setRotationPeriod   设置转动时间间隔
 * @param real_id   探头id
 * @param time      间隔时间
 * @param type      探头类型
 */
void MyThread::setRotationPeriod(unsigned char real_id,unsigned short time,QString type){
    uint8_t *data;
    uint8_t timeLow = time & 0xff;
    uint8_t timeHigh = (time>>8) & 0xff;
    QByteArray tmp;
    if(!QString::compare(MyData::oilSn,type)){
        if(MyData::oilType == "oil"){
            data = new uint8_t[11]{real_id,0x10,0x00,0x0a,0x00,0x01,0x02,timeHigh,timeLow};
            tmp = getNaHongName(real_id,1);
            if(tmp != nullptr){
                if(command(data,11,8,1,3,type) == nullptr){
                    emit time_ok(-1);
                }
                else {
                    emit time_ok(time);
                }
            }
            else {
                emit time_ok(0);
            }
            delete [] data;
        }
        else {
            data = new uint8_t[11]{real_id,0x10,0x32,0x00,0x00,0x01,0x02,timeLow,timeHigh};
            tmp = getSN(real_id,1,type);
            if(tmp != nullptr){
                if(command(data,11,8,1,3,type) == nullptr){
                    emit time_ok(-1);
                }
                else {
                    openBrush(real_id);
                    emit time_ok(time);
                }
            }
            else {
                emit time_ok(0);
            }
            delete [] data;
        }
    }
    else if (!QString::compare("38",type)) {
        data = new uint8_t[11]{real_id,0x10,0x0e,0x00,0x00,0x01,0x02,timeLow,timeHigh};
        tmp = getSondeSN(real_id,1);
        if(tmp != nullptr){
            if(command(data,11,8,1,3,type) == nullptr){
                emit time_ok(-1);
            }
            else {
                openBrush(real_id);
                emit time_ok(time);
            }
        }
        else {
            emit time_ok(0);
        }
        delete [] data;
    }
    else if (!QString::compare(MyData::condSn,type) || !QString::compare(MyData::orpSn,type) || !QString::compare(MyData::phSn,type) || !QString::compare(MyData::ldoSn,type)){
        return;
    }
    else {
        data = new uint8_t[11]{real_id,0x10,0x32,0x00,0x00,0x01,0x02,timeLow,timeHigh};
        tmp = getSN(real_id,1,type);
        if(tmp != nullptr){
            if(command(data,11,8,1,3,type) == nullptr){
                emit time_ok(-1);
            }
            else {
                openBrush(real_id);
                emit time_ok(time);
            }
        }
        else {
            emit time_ok(0);
        }
        delete [] data;
    }
}


/*开始测量*/
void MyThread::startMeasure(){
    uint8_t data[8] = {slaveId,3,0x25,0,0,1};
    command(data,8,7,1,3,"");
}

/*停止测量*/
void MyThread::stopMeasure(){
    uint8_t data[8] = {slaveId,3,0x2e,0,0,1};
    command(data,8,7,1,3,"");
}

void MyThread::setPHCal(float ph){    //开始PH校准，用于三点校准
    union{
        float fph;
        unsigned char cph[4];
    }pph;
    pph.fph = ph;
    uint8_t data[13] = {slaveId,0x10,0x23,0,0,2,4,pph.cph[0],pph.cph[1],pph.cph[2],pph.cph[3]};
    command(data,13,8,1,3,"");
}

/*获取PH校准状态*/
void MyThread:: getPHCalStatus(){
    uint8_t data[8] = {slaveId,3,0x0e,0,0,1};
    //emit receive_data(command(data,8,7,1,3),4);
}


void MyThread::setOrpCal(float k,float b){     //色设置orp参数
    union{
        float fk;
        unsigned char ck[4];
    }kk;
    union{
        float fb;
        unsigned char cb[4];
    }bb;
    kk.fk = k;
    bb.fb = b;
    uint8_t data[17] = {slaveId,0x10,0x34,0,0,4,8,kk.ck[0],kk.ck[1],kk.ck[2],kk.ck[3],bb.cb[0],bb.cb[1],bb.cb[2],bb.cb[3]};
    command(data,17,8,1,3,"");
}

void MyThread::getOrpCal(){
    uint8_t data[8] = {slaveId,3,0x34,0,04};
    //emit receive_data( command(data,8,13,1,3),5);
}

/*设置PH电极参数*/
void MyThread::setPHPole(float k1,float k2,float k3,float k4,float k5,float k6){  //设置PH电极参数
    union{
        float fk;
        unsigned char ck[4];
    }kk1;
    union{
        float fk;
        unsigned char ck[4];
    }kk2;
    union{
        float fk;
        unsigned char ck[4];
    }kk3;
    union{
        float fk;
        unsigned char ck[4];
    }kk4;
    union{
        float fk;
        unsigned char ck[4];
    }kk5;
    union{
        float fk;
        unsigned char ck[4];
    }kk6;

    kk1.fk = k1;
    kk2.fk = k2;
    kk3.fk = k3;
    kk4.fk = k4;
    kk5.fk = k5;
    kk6.fk = k6;

    uint8_t data[33] = {slaveId,0x10,0x29,0,0,0x0c,0x18,          kk1.ck[0],kk1.ck[1],kk1.ck[2],kk1.ck[3],  kk2.ck[0],kk2.ck[1],kk2.ck[2],kk2.ck[3],
                        kk3.ck[0],kk3.ck[1],kk3.ck[2],kk3.ck[3],  kk4.ck[0],kk4.ck[1],kk4.ck[2],kk4.ck[3],  kk5.ck[0],kk5.ck[1],kk5.ck[1],kk5.ck[3],
                        kk6.ck[0],kk6.ck[1],kk6.ck[1],kk6.ck[3]};
    command(data,33,8,1,3,"");
}

/*获取PH电极参数*/
void MyThread::getPHPole(){
    uint8_t data[8] = {slaveId,3,0x29,0,0,0x0c};
    //emit receive_data( command(data,8,29,1,3),6);
}

/*水中油一点定标*/
void MyThread::setOilCal(unsigned char real_id,float standare){
    union{
        float f;
        unsigned char c[4];
    }stand;
    QByteArray b1,b2;
    uint8_t light1,light2;
    uint8_t readLight[8] = {real_id,03,00,05,00,01};        //读原始光强
    QByteArray light = command(readLight,8,7,1,3,"BX-Oil-1N");
    if(light != nullptr){
        b1.resize(1);
        b2.resize(2);
        b1[0] = light[3];
        b2[0] = light[4];
        memcpy(&light1,b1,1);
        memcpy(&light2,b2,1);
        uint8_t writeLight[11] = {real_id,0x10,00,0x31,00,01,02,light1,light2};     //写入原始光强
        if(command(writeLight,11,8,1,3,"BX-Oil-1N") != nullptr){
            stand.f = standare;
            uint8_t writeStandare[13] = {real_id, 0x10, 00, 0x32, 00, 02, 04,stand.c[3],stand.c[2],stand.c[1],stand.c[0]};  //写入定标数据
            if(command(writeStandare,13,8,1,3,"BX-Oil-1N") != nullptr){
                uint8_t startCal[11] = {real_id,0x10,0x00,0x3a,0x00,0x01,0x02,0,01};    //启动一点定标
                if(command(startCal,11,8,1,3,"BX-Oil-1N") != nullptr){
                    emit oil_one_cal_ok("定标成功");
                }
                else {
                    emit oil_one_cal_ok("启动定标失败");
                }
            }
            else {
                emit oil_one_cal_ok("写入定标数据失败");
            }
        }
        else {
            emit oil_one_cal_ok("写入原始光强失败");
        }
    }
    else {
        emit oil_one_cal_ok("读取原始光强失败");
    }
}

/**
 * @brief setNaNhCal 纳宏氨氮探头参数校准
 * @param real_id
 * @param standare
 * @param calType
 */
void MyThread::setNaNhCal(unsigned char real_id,float standare,QString calType){
    union{
        float f;
        unsigned char c[4];
    }stand;
    QByteArray b1,b2,b3,b4;
    uint8_t light1,light2,light3,light4;

    uint8_t setType[11] = {real_id,0x10,0x00,0x30,0x00,0x01,0x02,0x00,0x01};
    qDebug()<<"纳宏探头开始"<<calType<<"校准";
    if(calType == "NH4_N"){
        setType[8] = 0x02;
    }
    if(command(setType,11,8,1,3,MyData::nhNaSn) != nullptr){
        uint8_t readLight[8] = {real_id,03,00,0x0b,00,02};        //读原始光强
        if(calType == "NH4_N"){
            setType[3] = 0x0d;
        }
        QByteArray light = command(readLight,8,9,1,3,MyData::nhNaSn);
        if(light != nullptr){
            b1.resize(1);
            b2.resize(1);
            b3.resize(1);
            b4.resize(1);
            b1[0] = light[6];
            b2[0] = light[5];
            b3[0] = light[4];
            b4[0] = light[3];
            memcpy(&light1,b1,1);
            memcpy(&light2,b2,1);
            memcpy(&light3,b3,1);
            memcpy(&light4,b4,1);
            uint8_t writeLight[13] = {real_id,0x10,00,0x31,00,01,02,light1,light2,light3,light4};     //写入原始光强
            if(command(writeLight,13,5,1,3,MyData::nhNaSn) != nullptr){
                stand.f = standare;
                uint8_t writeStandare[13] = {real_id, 0x10, 00, 0x33, 00, 02, 04,stand.c[3],stand.c[2],stand.c[1],stand.c[0]};  //写入定标数据
                if(command(writeStandare,13,8,1,3,MyData::nhNaSn) != nullptr){
                    uint8_t startCal[11] = {real_id,0x10,0x00,0x39,0x00,0x01,0x02,0,01};    //启动一点定标
                    if(command(startCal,11,8,1,3,MyData::nhNaSn) != nullptr){
                        emit oil_one_cal_ok("标定成功");
                    }
                    else {
                        emit oil_one_cal_ok("启动定标失败");
                    }
                }
                else {
                    emit oil_one_cal_ok("写入定标数据失败");
                }
            }
            else {
                emit oil_one_cal_ok("写入原始光强失败");
            }
        }
        else {
            emit oil_one_cal_ok("读取原始光强失败");
        }
    }
    else {
        emit oil_one_cal_ok("设置校准参数类型失败");
    }
}

QByteArray MyThread::command(uint8_t *data,uint8_t len,uint8_t recLen,uint8_t count,uint8_t end,QString type){
    QByteArray array;
    QString info;
    uint8_t recData[recLen];
    uint16_t tmp = GetCRC16(data,len-2);   //获取CRC校验码
    data[len-2] = tmp>>8;
    data[len-1] = tmp & 0xff;

    array.resize(len);
    memcpy(array.data(), data, len);  //将uint8类型数据转换为qByteArray
    info.prepend(array.toHex());
    port->readAll();
    sendInfo(info);
    sleep(300);
    if(recLen == 37){
        sleep(150);
    }
    if(condDelay){
        sleep(400);
    }
    handle_data();                            //读取下位机返回的数据
    memcpy(recData,hexData.data(),recLen);   //将QByteArray 转换成uint8类型的数组
    tmp = GetCRC16(recData,recLen-2);
    if(recData[recLen-2] == (tmp>>8) && recData[recLen-1] == (tmp & 0xff)){
        return hexData;
    }
    else {
        if(count < end){
            //出错的话则从数据库查询探头id,防止是由于修改id而导致收不到数据
            QSqlQuery query(DBUtil::getSqLite());
            query.prepare("select slave_id from probeInfo where sn = ? and status = 'online'");
            query.addBindValue(type);
            query.exec();
            while (query.next()) {
                data[0] = query.value(0).toInt();
            }

            qDebug()<<"第"<<QString::number(count)<<"次读取"+readType+"数据错误，发送数据:"<<info<<"  ,返回数据："<<hexData.toHex();
            count++;
            return command(data,len,recLen,count,end,type);       //读取次数是否到达
        }
        qDebug()<<"第2次读取"<<readType<<"数据错误，请确认探头存在或损坏，返回数据："<<hexData.toHex();
        return nullptr;
    }
}

void MyThread::sleep(int msec)//自定义Qt延时函数,单位毫秒
{
    QTime t;
    t.start();
    while(t.elapsed()<msec){
        QCoreApplication::processEvents();
    }
}

char MyThread::convertCharToHex(char ch){
    if(ch >= '0' && ch <= '9'){
        return ch-0x30;
    }else if(ch >= 'A' && ch <= 'F'){
        return ch - 'A' + 10;
    }else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

void MyThread::convertStringToHex(const QString &str, QByteArray &byteData){
    int hexdata,lowhexdata;
    int hexdatalen=0;
    int len = str.length();
    byteData.resize(len/2);
    char lstr,hstr;
    for (int i = 0;i < len;) {
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = convertCharToHex(hstr);
        lowhexdata = convertCharToHex(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        byteData[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    byteData.resize(hexdatalen);
}

//写两个函数 向单片机发送数据
 void MyThread::sendInfo(char* info,int len){

     for(int i=0; i<len; ++i)
     {
         printf("0x%x\n", info[i]);
     }
     port->write(info,len);//这句是真正的给单片机发数据 用到的是QIODevice::write 具体可以看文档
 }

 void MyThread::sendInfo(QString &info){

     QByteArray sendBuf;
     int res = 0;

     if (info.contains(" "))
     {
         info.replace(QString(" "),QString(""));//我这里是把空格去掉，根据你们定的协议来
     }
     //qDebug()<<"[sendInfo] Write to serial: "<<info;
     convertStringToHex(info, sendBuf); //把QString 转换 为 hex

     res = port->write(sendBuf); //这句是真正的给单片机发数据 用到的是QIODevice::write 具体可以看文档
     //qDebug()<<"[sendInfo] Write to res: "<<res;

 }


 /**
  * @brief MyThread::do_value_switch        溶解氧计算公式
  * @param TMEP
  * @param bdo
  * @return
  */
 float MyThread::do_value_switch(float TMEP,float bdo)//溶解氧百分比转mg/L
 {
     float pressure = 101.325;//气压值，单位kpa；
     float Phmg= 0.0;
     //float t = 0.0;
     float T = 0.0;
     float S = 0.0;//盐度，纯净水中，S=0；
     float ln_X1,X1,X2,log_u,u,Do;
     //qDebug()<<bdo<<endl;
     T = 273.15 + TMEP; //t为当前温度
     ln_X1 = -173.4292 // X1’ = ln X1根据以上的经典公式
             + 249.6339*(100/ T)
             + 143.3483*log(T /100)//log()函数即ln(x)
             + -21.8492*( T /100)
             +S*(-0.033096 + (0.014259* T)/100 //S=盐度，纯净水中，S=0；
                 -0.001700*( T /100)*( T /100));
     //qDebug()<<ln_X1<<endl;
     X1 = exp(ln_X1); //自然对数
     //qDebug()<<X1<<endl;
     // log u = 8.10765 - (1750.286/ (235+t))
     log_u = 8.10765 - (1750.286/ (235 + TMEP));// u’ = log u
     //()<<X1<<endl;
     u = pow(10, log_u);                              //u=10^u’
     //qDebug()<<u<<endl;
     Phmg = pressure*760/101.325;//pressure为气压值，单位kpa；
     //qDebug()<<Phmg<<endl;
     X2  =  ((Phmg - u)/(760 - u));
     //qDebug()<<X2<<endl;
     Do = bdo*X1*X2*1.4276;

     return Do;
 }



 /**
  * @brief Com::GetCRC16 获取CRC校验码
  * @param puchMsg
  * @param usDataLen
  * @return
  */
 uint16_t MyThread:: GetCRC16(unsigned char *puchMsg, unsigned short usDataLen)
 {
     const unsigned char auchCRCHi[256] = {
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
         0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
         0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
         0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
         };

     //CRC校验码低位
     const unsigned char auchCRCLo[256] = {
         0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
         0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
         0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
         0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
         0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
         0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
         0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
         0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
         0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
         0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
         0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
         0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
         0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
         0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
         0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
         0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
         0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
         0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
         0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
         0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
         0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
         0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
         0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
         0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
         0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
         0x43, 0x83, 0x41, 0x81, 0x80, 0x40
         };
     unsigned char uchCRCHi = 0xFF ;
     unsigned char uchCRCLo = 0xFF ;
     unsigned uIndex = 0;

     while (usDataLen--)
     {
         uIndex = uchCRCHi ^ *puchMsg++ ;
         uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
         uchCRCLo = auchCRCLo[uIndex] ;
     }
     return (unsigned short)((unsigned short)uchCRCHi << 8 | uchCRCLo) ;
 }
