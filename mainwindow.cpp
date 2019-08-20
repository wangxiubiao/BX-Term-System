#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QHBoxLayout"
#include <QGridLayout>
#include <QMutex>
#include <QTextTable>
#include <qfile.h>
#include <qprocess.h>
#include "mythread.h"
#include <QtSerialPort/QSerialPortInfo>

QTimer *timer;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    this->setFixedSize(1024,768);
    ui->setupUi(this);
    ui->gridLayout_24->setMargin(0);

    //ui->gridLayout_24->setSpacing(0);
    setWindowFlags(Qt::FramelessWindowHint);
    installEventFilter(ui->customTitleBar);
    //DBUtil::getSqLite();
    query = QSqlQuery(DBUtil::getSqLite());
    mainUiInit();
    //readUserConfig();
    //查询可使用的串口
    ui->pushButton_port_update->emit clicked();

}

MainWindow::~MainWindow()
{
    saveUserConfig();
    DBUtil::closeDB();
    qDebug()<<"关闭程序";
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event){
    qDebug()<<"界面初始化结束：调用showEvent";
    readUserConfig();
    ui_ok();
}

void MainWindow::readUserConfig(){
    if(query.exec("select type ,value from userConfig")){
      int i = 0;
        while (query.next()) {
            qDebug()<<"查询用户配置成功"<<i++;
            if(query.value(0).toString() == "autoRead"){
                if(query.value(1).toString() == "0"){
                    //
                    //timer = new QTimer(this);
                    //connect(timer, SIGNAL(timeout()), this, SLOT(ui_ok()));
                    //timer->start(2000);
                    autoRead = true;
                    ui->checkBox_autoRead->setChecked(true);
                }
                else {
                    ui->checkBox_autoRead->setChecked(false);
                }
            }
            else if (query.value(0).toString() == "sqlName") {
                ui->LineEdit_user_name->setText(query.value(1).toString());
            }
            else if (query.value(0).toString() == "sqlPwd") {
                ui->LineEdit_pwd->setText(query.value(1).toString());
            }
            else if (query.value(0).toString() == "sqlIP") {
                ui->LineEdit_server_addr->setText(query.value(1).toString());
            }
            else if (query.value(0).toString() == "sqlTime") {
                ui->lineEdit_sql_time->setText(query.value(1).toString());
            }

        }
    }
}


void MainWindow::saveUserConfig(){
    QVariantList typeList,valueList;
    query.exec("update probeInfo set status = 'drop'");
    //保存用户信息
    typeList<<"deafaultPort"<<"sqlName"<<"sqlPwd"<<"sqlIP"<<"sqlTime";
    valueList<<ui->comboBox_portName->currentText()<<ui->LineEdit_user_name->text()
            <<ui->LineEdit_pwd->text()<<ui->LineEdit_server_addr->text()
            <<ui->lineEdit_sql_time->text();
    query.prepare("update userConfig set value = ? where type = ?");
    query.addBindValue(valueList);
    query.addBindValue(typeList);
    if(!query.execBatch()){
        query.lastError();
    }
}

void MainWindow::ui_ok(){
    qDebug()<<"界面初始化完成";
    //timer->stop();
    //delete timer;
//    autoRead = true;
//    ui->checkBox_autoRead->setChecked(true);
    if(autoRead)
        this->ui->pushButton_port_conn->emit clicked();
}

void MainWindow::post_ok(QString msg){
    ui->textEdit_sql->append("数据上传成功："+msg);
}


//自定义标题栏实现鼠标移动
bool leftBtnClk = false;
QPoint m_Press,m_Move;
void MainWindow::mousePressEvent(QMouseEvent *event)
{

    if( event->button() == Qt::LeftButton &&
            ui->customTitleBar->frameRect().contains(event->globalPos() - this->frameGeometry().topLeft())){
        m_Press = event->globalPos();
        leftBtnClk = true;
    }
    event->ignore();//表示继续向下传递事件，其他的控件还可以去获取
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{

    if( event->button() == Qt::LeftButton ){
        leftBtnClk = false;
    }
    event->ignore();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if( leftBtnClk ){
        m_Move = event->globalPos();
        this->move( this->pos() + m_Move - m_Press );
        m_Press = m_Move;
    }
    event->ignore();
}


/**
 * @brief MainWindow::connInit
 * 初始化线程间通信的信号与槽
 */
void MainWindow::connInit(){
    /** ui进程主动与子线程(myThread)通信 */
    connect(this,SIGNAL(conn_port(QString,qint32)),myThread,SLOT(init_port(QString,qint32)));   //串口连接
    connect(this,SIGNAL(setSlaveId(unsigned char,unsigned char,QString)),myThread,SLOT(setSlaveId(unsigned char,unsigned char,QString)));
    connect(this,SIGNAL(setRotationPeriod(unsigned char,unsigned short,QString)),myThread,SLOT(setRotationPeriod(unsigned char,unsigned short,QString)));
    connect(this,SIGNAL(close_port()),myThread,SLOT(close_port()));
    connect(this,SIGNAL(getChla(unsigned char,QString)),myThread,SLOT(getChla(unsigned char,QString)));
    connect(this,SIGNAL(singleSearch()),myThread,SLOT(singleSearch()));
    connect(this,SIGNAL(defaultSearch()),myThread,SLOT(defaultSearch()));
    connect(this,SIGNAL(run(bool)),myThread,SLOT(run(bool)));
    connect(this,SIGNAL(clear(unsigned char,QString)),myThread,SLOT(clear(unsigned char,QString)));
    connect(this,SIGNAL(setCalParameter(unsigned char,float,float,unsigned char,unsigned char)),myThread,SLOT(setCalParameter(unsigned char,float,float,unsigned char,unsigned char)));
    connect(this,SIGNAL(getCalParameter(unsigned char,QString,unsigned char)),myThread,SLOT(getCalParameter(unsigned char,QString,unsigned char)));
    connect(this,SIGNAL(getTestData(unsigned char,QString,bool,QString)),myThread,SLOT(getTestData(unsigned char,QString,bool,QString)));
    connect(this,SIGNAL(setOilCal(unsigned char,float)),myThread,SLOT(setOilCal(unsigned char,float)));
    connect(this,SIGNAL(openNaHongBrush(unsigned char ,int)),myThread,SLOT(openNaHongBrush(unsigned char,int)));
    connect(this,SIGNAL(setOilClearNum(unsigned char,unsigned char)),myThread,SLOT(setOilClearNum(unsigned char,unsigned char)));
    connect(this,SIGNAL(sqlConn(QString,QString,QString,bool,int)),myThread,SLOT(sqlConn(QString,QString,QString,bool,int)));
    connect(this,SIGNAL(getSNSignal(unsigned char,QString)),myThread,SLOT(getSNSignal(unsigned char,QString)));
    connect(this,SIGNAL(setNaNhCal(unsigned char,float,QString)),myThread,SLOT(setNaNhCal(unsigned char,float,QString)));


    /** 子线程(myThread)主动与ui通信 */
    connect(myThread,SIGNAL(id_ok(int)),this,SLOT(id_ok(int)));
    connect(myThread,SIGNAL(time_ok(int)),this,SLOT(time_ok(int)));
    connect(myThread,SIGNAL(data_type(QByteArray,QString)),this,SLOT(data_type(QByteArray,QString)));
    connect(myThread,SIGNAL(single_search_ok(QString,unsigned char)),this,SLOT(single_search_ok(QString,unsigned char)));
    connect(myThread,SIGNAL(default_search_ok(QString *,unsigned char)),this,SLOT(default_search_ok(QString *,unsigned char)));
    connect(myThread,SIGNAL(test1_ok(float)),this,SLOT(test1_ok(float)));
    connect(myThread,SIGNAL(oil_one_cal_ok(QString)),this,SLOT(oil_one_cal_ok(QString)));
    connect(myThread,SIGNAL(com_open_ok(QString,bool)),this,SLOT(com_open_ok(QString,bool)));
    connect(myThread,SIGNAL(cal_param_ok(float,float,int)),this,SLOT(cal_param_ok(float,float,int)));
    connect(myThread,SIGNAL(oil_clear_num_ok(unsigned char,bool)),this,SLOT(oil_clear_num_ok(unsigned char,bool)));
    connect(myThread,SIGNAL(sql_conn_ok(bool,bool)),SLOT(sql_conn_ok(bool,bool)));
    connect(myThread,SIGNAL(changeStatus(QString,QString)),SLOT(changeStatus(QString,QString)));
    connect(myThread,SIGNAL(save_data_ok()),this,SLOT(save_data_ok()));
    connect(myThread,SIGNAL(get_sn_ok(QString)),this,SLOT(get_sn_ok(QString)));
    connect(myThread,SIGNAL(post_ok(QString)),this,SLOT(post_ok(QString)));

}


/**
 * @brief MainWindow::mainUiInit 初始化UI界面
 */
void MainWindow::mainUiInit(){

    ui->groupBox_sonde_bga->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_chla->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_cond->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_ldo->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_orp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_ph->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_turb->setMinimumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_cod_cod->setMinimumSize(groupBoxWidth,groupBoxHeight);

    ui->groupBox_sonde_bga->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_chla->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_cond->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_ldo->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_orp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_ph->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_sonde_turb->setMaximumSize(groupBoxWidth,groupBoxHeight);
    ui->groupBox_cod_cod->setMaximumSize(groupBoxWidth,groupBoxHeight);

    snType = MyData::codSn;
    ui->textEdit_setTantou_data->document()->setMaximumBlockCount(10000);
    ui->textEdit_all->document()->setMaximumBlockCount(10000);
    ui->textEdit_sql->document()->setMaximumBlockCount(10000);

    ui->textEdit_sql->setReadOnly(true);
    ui->textEdit_all->setReadOnly(true);
    ui->textEdit_setTantou_data->setReadOnly(true);

    ui->label_stand2->setHidden(true);
    ui->lineEdit_stand2->setHidden(true);
    ui->pushButton_stand2->setHidden(true);
    ui->label_test2_value->setHidden(true);
    ui->radioButton_auto->setChecked(true);
    ui->pushButton_clear_num->setEnabled(false);
    ui->label_clear_num->setEnabled(false);
    ui->lineEdit_clear_num->setEnabled(false);
    ui->radioButton_auto->setEnabled(false);
    ui->radioButton_hand->setEnabled(false);
    ui->LineEdit_pwd->setEchoMode(QLineEdit::Password);


    ui->label_current_tantou->setText("cod");
    ui->label_addr_default->setText("1");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Cod");
    ui->comboBox_cal_param->addItem("Turb");
    ui->groupBox_brush->setEnabled(true);
    query.prepare("select slave_id from probeInfo where sn = '47'");
    query.exec();
    while (query.next()) {
        ui->label_real_id->setNum(query.value(0).toInt());
    }
    snType = MyData::codSn;
    first = true;
    set_not_oil_show();


    box_cod_toc = new QGroupBox();
    box_cod_toc->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_cod_toc->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_cod_toc = new QGridLayout(box_cod_toc);
    label_cod_toc_title = new QLabel("TOC");
    label_cod_toc_unit = new QLabel("mg/L");
    label_cod_toc_data = new QLabel();
    label_cod_toc_logo = new QLabel("COD  BX-UV254-online");
    layout_cod_toc->addWidget(label_cod_toc_title,0,0,1,1,Qt::AlignLeft);
    layout_cod_toc->addWidget(label_cod_toc_unit,0,1,1,1,Qt::AlignRight);
    layout_cod_toc->addWidget(label_cod_toc_data,1,0,1,2,Qt::AlignHCenter);
    layout_cod_toc->addWidget(label_cod_toc_logo,2,0,1,2,Qt::AlignHCenter);

    box_cod_turb = new QGroupBox();
    box_cod_turb->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_cod_turb->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_cod_turb = new QGridLayout(box_cod_turb);
    label_cod_turb_title = new QLabel("浊度");
    label_cod_turb_unit = new QLabel("NTU");
    label_cod_turb_data = new QLabel();
    label_cod_turb_logo = new QLabel("COD  BX-UV254-online");
    layout_cod_turb->addWidget(label_cod_turb_title,0,0,1,1,Qt::AlignLeft);
    layout_cod_turb->addWidget(label_cod_turb_unit,0,1,1,1,Qt::AlignRight);
    layout_cod_turb->addWidget(label_cod_turb_data,1,0,1,2,Qt::AlignHCenter);
    layout_cod_turb->addWidget(label_cod_turb_logo,2,0,1,2,Qt::AlignHCenter);

    box_cod_temp = new QGroupBox();
    box_cod_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_cod_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_cod_temp = new QGridLayout(box_cod_temp);
    label_cod_temp_title = new QLabel("温度");
    label_cod_temp_unit = new QLabel("℃");
    label_cod_temp_data = new QLabel();
    label_cod_temp_logo = new QLabel("COD  BX-UV254-online");
    layout_cod_temp->addWidget(label_cod_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_cod_temp->addWidget(label_cod_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_cod_temp->addWidget(label_cod_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_cod_temp->addWidget(label_cod_temp_logo,2,0,1,2,Qt::AlignHCenter);



    box_nh_ph = new QGroupBox();
    box_nh_ph->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_ph->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_nh_ph = new QGridLayout(box_nh_ph);
    label_nh_ph_title = new QLabel("pH");
    label_nh_ph_unit = new QLabel("ph");
    label_nh_ph_data = new QLabel("0");
    label_nh_ph_logo = new QLabel("氨氮  BX-Ammo II");
    layout_nh_ph->addWidget(label_nh_ph_title,0,0,1,1,Qt::AlignLeft);
    layout_nh_ph->addWidget(label_nh_ph_unit,0,1,1,1,Qt::AlignRight);
    layout_nh_ph->addWidget(label_nh_ph_data,1,0,1,2,Qt::AlignHCenter);
    layout_nh_ph->addWidget(label_nh_ph_logo,2,0,1,2,Qt::AlignHCenter);

    box_nh_orp = new QGroupBox();
    box_nh_orp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_orp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_nh_orp = new QGridLayout(box_nh_orp);
    label_nh_orp_title = new QLabel("ORP");
    label_nh_orp_unit = new QLabel("mV");
    label_nh_orp_data = new QLabel();
    label_nh_orp_logo = new QLabel("氨氮  BX-Ammo II");
    layout_nh_orp->addWidget(label_nh_orp_title,0,0,1,1,Qt::AlignLeft);
    layout_nh_orp->addWidget(label_nh_orp_unit,0,1,1,1,Qt::AlignRight);
    layout_nh_orp->addWidget(label_nh_orp_data,1,0,1,2,Qt::AlignHCenter);
    layout_nh_orp->addWidget(label_nh_orp_logo,2,0,1,2,Qt::AlignHCenter);

    box_nh_nh4 = new QGroupBox();
    box_nh_nh4->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_nh4->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_nh_nh4 = new QGridLayout(box_nh_nh4);
    label_nh_nh4_title = new QLabel("NH4+");
    label_nh_nh4_unit = new QLabel("mV");
    label_nh_nh4_data = new QLabel();
    label_nh_nh4_logo = new QLabel("氨氮  BX-Ammo II");
    layout_nh_nh4->addWidget(label_nh_nh4_title,0,0,1,1,Qt::AlignLeft);
    layout_nh_nh4->addWidget(label_nh_nh4_unit,0,1,1,1,Qt::AlignRight);
    layout_nh_nh4->addWidget(label_nh_nh4_data,1,0,1,2,Qt::AlignHCenter);
    layout_nh_nh4->addWidget(label_nh_nh4_logo,2,0,1,2,Qt::AlignHCenter);

    box_nh_k = new QGroupBox();
    box_nh_k->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_k->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_nh_k = new QGridLayout(box_nh_k);
    label_nh_k_title = new QLabel("K+");
    label_nh_k_unit = new QLabel("mV");
    label_nh_k_data = new QLabel();
    label_nh_k_logo = new QLabel("氨氮  BX-Ammo II");
    layout_nh_k->addWidget(label_nh_k_title,0,0,1,1,Qt::AlignLeft);
    layout_nh_k->addWidget(label_nh_k_unit,0,1,1,1,Qt::AlignRight);
    layout_nh_k->addWidget(label_nh_k_data,1,0,1,2,Qt::AlignHCenter);
    layout_nh_k->addWidget(label_nh_k_logo,2,0,1,2,Qt::AlignHCenter);

    box_nh_nh4_n = new QGroupBox("");
    box_nh_nh4_n->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_nh4_n->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_nh_nh4_n = new QGridLayout(box_nh_nh4_n);
    label_nh_nh4_n_title = new QLabel("nh4_n");
    label_nh_nh4_n_unit = new QLabel("mg/L");
    label_nh_nh4_n_data = new QLabel();
    label_nh_nh4_n_logo = new QLabel("氨氮  BX-Ammo II");
    layout_nh_nh4_n->addWidget(label_nh_nh4_n_title,0,0,1,1,Qt::AlignLeft);
    layout_nh_nh4_n->addWidget(label_nh_nh4_n_unit,0,1,1,1,Qt::AlignRight);
    layout_nh_nh4_n->addWidget(label_nh_nh4_n_data,1,0,1,2,Qt::AlignHCenter);
    layout_nh_nh4_n->addWidget(label_nh_nh4_n_logo,2,0,1,2,Qt::AlignHCenter);

    box_nh_temp = new QGroupBox("");
    box_nh_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_nh_temp = new QGridLayout(box_nh_temp);
    label_nh_temp_title = new QLabel("温度");
    label_nh_temp_unit = new QLabel("℃");
    label_nh_temp_data = new QLabel();
    label_nh_temp_logo = new QLabel("氨氮  BX-Ammo II");
    layout_nh_temp->addWidget(label_nh_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_nh_temp->addWidget(label_nh_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_nh_temp->addWidget(label_nh_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_nh_temp->addWidget(label_nh_temp_logo,2,0,1,2,Qt::AlignHCenter);


    box_chla_chla = new QGroupBox("");
    box_chla_chla->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_chla_chla->setMaximumSize(groupBoxWidth,groupBoxHeight);
    //box_chla_chla->setStyleSheet("QGroupBox{padding-top: 4ex}");
    layout_chla_chla = new QGridLayout(box_chla_chla);
    label_chla_chla_title = new QLabel("chla");
    label_chla_chla_unit = new QLabel("μg/L");
    label_chla_chla_data = new QLabel("0");
    label_chla_chla_logo = new QLabel("叶绿素  BX-Chla");
    layout_chla_chla->addWidget(label_chla_chla_title,0,0,1,1,Qt::AlignLeft);
    layout_chla_chla->addWidget(label_chla_chla_unit,0,1,1,1,Qt::AlignRight);
    layout_chla_chla->addWidget(label_chla_chla_data,1,0,1,2,Qt::AlignHCenter);
    layout_chla_chla->addWidget(label_chla_chla_logo,2,0,1,2,Qt::AlignHCenter);

    box_chla_temp = new QGroupBox("");
    box_chla_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_chla_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_chla_temp = new QGridLayout(box_chla_temp);
    label_chla_temp_title = new QLabel("温度");
    label_chla_temp_unit = new QLabel("℃");
    label_chla_temp_data = new QLabel("0");
    label_chla_temp_logo = new QLabel("叶绿素  BX-Chla");
    layout_chla_temp->addWidget(label_chla_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_chla_temp->addWidget(label_chla_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_chla_temp->addWidget(label_chla_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_chla_temp->addWidget(label_chla_temp_logo,2,0,1,2,Qt::AlignHCenter);

    box_oxygen_do = new QGroupBox("");
    box_oxygen_do->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_oxygen_do->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_oxygen_do = new QGridLayout(box_oxygen_do);
    label_oxygen_do_title = new QLabel("溶解氧");
    label_oxygen_do_unit = new QLabel("mg/L");
    label_oxygen_do_data = new QLabel("0");
    label_oxygen_do_logo = new QLabel("溶解氧  BX-LDO");
    layout_oxygen_do->addWidget(label_oxygen_do_title,0,0,1,1,Qt::AlignLeft);
    layout_oxygen_do->addWidget(label_oxygen_do_unit,0,1,1,1,Qt::AlignRight);
    layout_oxygen_do->addWidget(label_oxygen_do_data,1,0,1,2,Qt::AlignHCenter);
    layout_oxygen_do->addWidget(label_oxygen_do_logo,2,0,1,2,Qt::AlignHCenter);


    box_oxygen_temp = new QGroupBox("");
    box_oxygen_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_oxygen_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_oxygen_temp = new QGridLayout(box_oxygen_temp);
    label_oxygen_temp_title = new QLabel("温度");
    label_oxygen_temp_unit = new QLabel("℃");
    label_oxygen_temp_data = new QLabel("0");
    label_oxygen_temp_logo = new QLabel("溶解氧  BX-LDO");
    layout_oxygen_temp->addWidget(label_oxygen_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_oxygen_temp->addWidget(label_oxygen_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_oxygen_temp->addWidget(label_oxygen_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_oxygen_temp->addWidget(label_oxygen_temp_logo,2,0,1,2,Qt::AlignHCenter);

    box_oxygen_saturation = new QGroupBox("");
    box_oxygen_saturation->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_oxygen_saturation->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_oxygen_saturation = new QGridLayout(box_oxygen_saturation);
    label_oxygen_saturation_title = new QLabel("饱和度");
    label_oxygen_saturation_unit = new QLabel("%");
    label_oxygen_saturation_data = new QLabel("0");
    label_oxygen_saturation_logo = new QLabel("溶解氧  BX-LDO");
    layout_oxygen_saturation->addWidget(label_oxygen_saturation_title,0,0,1,1,Qt::AlignLeft);
    layout_oxygen_saturation->addWidget(label_oxygen_saturation_unit,0,1,1,1,Qt::AlignRight);
    layout_oxygen_saturation->addWidget(label_oxygen_saturation_data,1,0,1,2,Qt::AlignHCenter);
    layout_oxygen_saturation->addWidget(label_oxygen_saturation_logo,2,0,1,2,Qt::AlignHCenter);

    box_turb_turb = new QGroupBox("");
    box_turb_turb->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_turb_turb->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_turb_turb = new QGridLayout(box_turb_turb);
    label_turb_turb_title = new QLabel("浊度");
    label_turb_turb_unit = new QLabel("NTU");
    label_turb_turb_data = new QLabel("0");
    label_turb_turb_logo = new QLabel("浊度  BX-Turb II");
    layout_turb_turb->addWidget(label_turb_turb_title,0,0,1,1,Qt::AlignLeft);
    layout_turb_turb->addWidget(label_turb_turb_unit,0,1,1,1,Qt::AlignRight);
    layout_turb_turb->addWidget(label_turb_turb_data,1,0,1,2,Qt::AlignHCenter);
    layout_turb_turb->addWidget(label_turb_turb_logo,2,0,1,2,Qt::AlignHCenter);


    box_turb_temp = new QGroupBox("");
    box_turb_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_turb_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_turb_temp = new QGridLayout(box_turb_temp);
    label_turb_temp_title = new QLabel("温度");
    label_turb_temp_unit = new QLabel("℃");
    label_turb_temp_data = new QLabel("0");
    label_turb_temp_logo = new QLabel("浊度  BX-Turb II");
    layout_turb_temp->addWidget(label_turb_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_turb_temp->addWidget(label_turb_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_turb_temp->addWidget(label_turb_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_turb_temp->addWidget(label_turb_temp_logo,2,0,1,2,Qt::AlignHCenter);


    box_cond_cond = new QGroupBox("");
    box_cond_cond->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_cond_cond->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_cond_cond = new QGridLayout(box_cond_cond);
    label_cond_cond_title = new QLabel("电导率");
    label_cond_cond_unit = new QLabel("μS/cm");
    label_cond_cond_data = new QLabel("0");
    label_cond_cond_logo = new QLabel("电导率  BX-Cond II");
    layout_cond_cond->addWidget(label_cond_cond_title,0,0,1,1,Qt::AlignLeft);
    layout_cond_cond->addWidget(label_cond_cond_unit,0,1,1,1,Qt::AlignRight);
    layout_cond_cond->addWidget(label_cond_cond_data,1,0,1,2,Qt::AlignHCenter);
    layout_cond_cond->addWidget(label_cond_cond_logo,2,0,1,2,Qt::AlignHCenter);

    box_cond_temp = new QGroupBox("");
    box_cond_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_cond_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_cond_temp = new QGridLayout(box_cond_temp);
    label_cond_temp_title = new QLabel("温度");
    label_cond_temp_unit = new QLabel("℃");
    label_cond_temp_data = new QLabel("0");
    label_cond_temp_logo = new QLabel("电导率  BX-Cond II");
    layout_cond_temp->addWidget(label_cond_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_cond_temp->addWidget(label_cond_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_cond_temp->addWidget(label_cond_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_cond_temp->addWidget(label_cond_temp_logo,2,0,1,2,Qt::AlignHCenter);


    box_ph_ph = new QGroupBox("");
    box_ph_ph->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_nh_ph->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_ph_ph = new QGridLayout(box_ph_ph);
    label_ph_ph_title = new QLabel("pH");
    label_ph_ph_unit = new QLabel("pH");
    label_ph_ph_data = new QLabel("0");
    label_ph_ph_logo = new QLabel("pH  BX-pH");
    layout_ph_ph->addWidget(label_ph_ph_title,0,0,1,1,Qt::AlignLeft);
    layout_ph_ph->addWidget(label_ph_ph_unit,0,1,1,1,Qt::AlignRight);
    layout_ph_ph->addWidget(label_ph_ph_data,1,0,1,2,Qt::AlignHCenter);
    layout_ph_ph->addWidget(label_ph_ph_logo,2,0,1,2,Qt::AlignHCenter);


    box_ph_orp = new QGroupBox("");
    box_ph_orp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_ph_orp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_ph_orp = new QGridLayout(box_ph_orp);
    label_ph_orp_title = new QLabel("orp");
    label_ph_orp_unit = new QLabel("mV");
    label_ph_orp_data = new QLabel("0");
    label_ph_orp_logo = new QLabel("pH  BX-pH");
    layout_ph_orp->addWidget(label_ph_orp_title,0,0,1,1,Qt::AlignLeft);
    layout_ph_orp->addWidget(label_ph_orp_unit,0,1,1,1,Qt::AlignRight);
    layout_ph_orp->addWidget(label_ph_orp_data,1,0,1,2,Qt::AlignHCenter);
    layout_ph_orp->addWidget(label_ph_orp_logo,2,0,1,2,Qt::AlignHCenter);

    box_ph_temp = new QGroupBox("");
    box_ph_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_ph_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_ph_temp = new QGridLayout(box_ph_temp);
    label_ph_temp_title = new QLabel("温度");
    label_ph_temp_unit = new QLabel("℃");
    label_ph_temp_data = new QLabel("0");
    label_ph_temp_logo = new QLabel("pH  BX-pH");
    layout_ph_temp->addWidget(label_ph_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_ph_temp->addWidget(label_ph_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_ph_temp->addWidget(label_ph_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_ph_temp->addWidget(label_ph_temp_logo,2,0,1,2,Qt::AlignHCenter);

    box_orp_orp = new QGroupBox("");
    box_orp_orp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_orp_orp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_orp_orp = new QGridLayout(box_orp_orp);
    label_orp_orp_title = new QLabel("orp");
    label_orp_orp_unit = new QLabel("mV");
    label_orp_orp_data = new QLabel("0");
    label_orp_orp_logo = new QLabel("ORP  BX-ORP");
    layout_orp_orp->addWidget(label_orp_orp_title,0,0,1,1,Qt::AlignLeft);
    layout_orp_orp->addWidget(label_orp_orp_unit,0,1,1,1,Qt::AlignRight);
    layout_orp_orp->addWidget(label_orp_orp_data,1,0,1,2,Qt::AlignHCenter);
    layout_orp_orp->addWidget(label_orp_orp_logo,2,0,1,2,Qt::AlignHCenter);

    box_orp_temp = new QGroupBox("");
    box_orp_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_orp_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_orp_temp = new QGridLayout(box_orp_temp);
    label_orp_temp_title = new QLabel("温度");
    label_orp_temp_unit = new QLabel("℃");
    label_orp_temp_data = new QLabel("0");
    label_orp_temp_logo = new QLabel("ORP  BX-ORP");
    layout_orp_temp->addWidget(label_orp_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_orp_temp->addWidget(label_orp_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_orp_temp->addWidget(label_orp_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_orp_temp->addWidget(label_orp_temp_logo,2,0,1,2,Qt::AlignHCenter);

    box_bga_bga = new QGroupBox("");
    box_bga_bga->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_bga_bga->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_bga_bga = new QGridLayout(box_bga_bga);
    label_bga_bga_title = new QLabel("bga");
    label_bga_bga_unit = new QLabel("cells/mL");
    label_bga_bga_data = new QLabel("0");
    label_bga_bga_logo = new QLabel("蓝绿藻  BX-Cyano");
    layout_bga_bga->addWidget(label_bga_bga_title,0,0,1,1,Qt::AlignLeft);
    layout_bga_bga->addWidget(label_bga_bga_unit,0,1,1,1,Qt::AlignRight);
    layout_bga_bga->addWidget(label_bga_bga_data,1,0,1,2,Qt::AlignHCenter);
    layout_bga_bga->addWidget(label_bga_bga_logo,2,0,1,2,Qt::AlignHCenter);

    box_bga_temp = new QGroupBox("");
    box_bga_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_bga_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_bga_temp = new QGridLayout(box_bga_temp);
    label_bga_temp_title = new QLabel("温度");
    label_bga_temp_unit = new QLabel("℃");
    label_bga_temp_data = new QLabel("0");
    label_bga_temp_logo = new QLabel("蓝绿藻  BX-Cyano");
    layout_bga_temp->addWidget(label_bga_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_bga_temp->addWidget(label_bga_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_bga_temp->addWidget(label_bga_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_bga_temp->addWidget(label_bga_temp_logo,2,0,1,2,Qt::AlignHCenter);


    box_oil_oil = new QGroupBox("");
    box_oil_oil->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_oil_oil->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_oil_oil = new QGridLayout(box_oil_oil);
    label_oil_oil_title = new QLabel("OIL");
    label_oil_oil_unit = new QLabel("mg/L");
    label_oil_oil_data = new QLabel("0");
    label_oil_oil_logo = new QLabel("水中油  BX-Oil");
    layout_oil_oil->addWidget(label_oil_oil_title,0,0,1,1,Qt::AlignLeft);
    layout_oil_oil->addWidget(label_oil_oil_unit,0,1,1,1,Qt::AlignRight);
    layout_oil_oil->addWidget(label_oil_oil_data,1,0,1,2,Qt::AlignHCenter);
    layout_oil_oil->addWidget(label_oil_oil_logo,2,0,1,2,Qt::AlignHCenter);

    box_oil_temp = new QGroupBox("");
    box_oil_temp->setMinimumSize(groupBoxWidth,groupBoxHeight);
    box_oil_temp->setMaximumSize(groupBoxWidth,groupBoxHeight);
    layout_oil_temp = new QGridLayout(box_oil_temp);
    label_oil_temp_title = new QLabel("温度");
    label_oil_temp_unit = new QLabel("℃");
    label_oil_temp_data = new QLabel("0");
    label_oil_temp_logo = new QLabel("水中油  BX-Oil");
    layout_oil_temp->addWidget(label_oil_temp_title,0,0,1,1,Qt::AlignLeft);
    layout_oil_temp->addWidget(label_oil_temp_unit,0,1,1,1,Qt::AlignRight);
    layout_oil_temp->addWidget(label_oil_temp_data,1,0,1,2,Qt::AlignHCenter);
    layout_oil_temp->addWidget(label_oil_temp_logo,2,0,1,2,Qt::AlignHCenter);

    label1 = new QLabel();
    set_data_style();
}


void MainWindow::set_data_style(){
    QString dataStyle = "QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}";

    ui->label_sonde_bga->setStyleSheet(dataStyle);
    ui->label_sonde_chla->setStyleSheet(dataStyle);
    ui->label_sonde_cond->setStyleSheet(dataStyle);
    ui->label_sonde_ldo->setStyleSheet(dataStyle);
    ui->label_sonde_orp->setStyleSheet(dataStyle);
    ui->label_sonde_ph->setStyleSheet(dataStyle);
    ui->label_sonde_temp->setStyleSheet(dataStyle);
    ui->label_sonde_turb->setStyleSheet(dataStyle);


    ui->label_cod_cod->setStyleSheet(dataStyle);
    label_cod_toc_data->setStyleSheet(dataStyle);
    label_cod_turb_data->setStyleSheet(dataStyle);
    label_cod_temp_data->setStyleSheet(dataStyle);

    label_nh_ph_data->setStyleSheet(dataStyle);
    label_nh_orp_data->setStyleSheet(dataStyle);
    label_nh_nh4_data->setStyleSheet(dataStyle);
    label_nh_k_data->setStyleSheet(dataStyle);
    label_nh_temp_data->setStyleSheet(dataStyle);
    label_nh_nh4_n_data->setStyleSheet(dataStyle);

    label_oxygen_do_data->setStyleSheet(dataStyle);
    label_oxygen_temp_data->setStyleSheet(dataStyle);
    label_oxygen_saturation_data->setStyleSheet(dataStyle);

    label_turb_turb_data->setStyleSheet(dataStyle);
    label_turb_temp_data->setStyleSheet(dataStyle);

    label_cond_cond_data->setStyleSheet(dataStyle);
    label_cond_temp_data->setStyleSheet(dataStyle);

    label_orp_orp_data->setStyleSheet(dataStyle);
    //label_orp_pH_data->setStyleSheet(dataStyle);
    label_orp_temp_data->setStyleSheet(dataStyle);

    label_ph_ph_data->setStyleSheet(dataStyle);
    label_ph_orp_data->setStyleSheet(dataStyle);
    label_ph_temp_data->setStyleSheet(dataStyle);

    label_chla_chla_data->setStyleSheet(dataStyle);
    label_chla_temp_data->setStyleSheet(dataStyle);

    label_bga_bga_data->setStyleSheet(dataStyle);
    label_bga_temp_data->setStyleSheet(dataStyle);

    label_oil_oil_data->setStyleSheet(dataStyle);
    label_oil_temp_data->setStyleSheet(dataStyle);


    ui->label_sonde_bga->setMinimumSize(196,45);
    ui->label_sonde_chla->setMinimumSize(196,45);
    ui->label_sonde_cond->setMinimumSize(196,45);
    ui->label_sonde_ldo->setMinimumSize(196,45);
    ui->label_sonde_orp->setMinimumSize(196,45);
    ui->label_sonde_ph->setMinimumSize(196,45);
    ui->label_sonde_temp->setMinimumSize(196,45);
    ui->label_sonde_turb->setMinimumSize(196,45);

    ui->label_sonde_bga->setMaximumSize(196,45);
    ui->label_sonde_chla->setMaximumSize(196,45);
    ui->label_sonde_cond->setMaximumSize(196,45);
    ui->label_sonde_ldo->setMaximumSize(196,45);
    ui->label_sonde_orp->setMaximumSize(196,45);
    ui->label_sonde_ph->setMaximumSize(196,45);
    ui->label_sonde_temp->setMaximumSize(196,45);
    ui->label_sonde_turb->setMaximumSize(196,45);

    label_cod_toc_data->setMinimumSize(196,45);
    label_cod_turb_data->setMinimumSize(196,45);
    label_cod_temp_data->setMinimumSize(196,45);


    label_nh_ph_data->setMinimumSize(196,45);
    label_nh_orp_data->setMinimumSize(196,45);
    label_nh_nh4_data->setMinimumSize(196,45);
    label_nh_k_data->setMinimumSize(196,45);
    label_nh_temp_data->setMinimumSize(196,45);
    label_nh_nh4_n_data->setMinimumSize(196,45);

    label_oxygen_do_data->setMinimumSize(196,45);
    label_oxygen_temp_data->setMinimumSize(196,45);
    label_oxygen_saturation_data->setMinimumSize(196,45);

    label_turb_turb_data->setMinimumSize(196,45);
    label_turb_temp_data->setMinimumSize(196,45);

    label_cond_cond_data->setMinimumSize(196,45);
    label_cond_temp_data->setMinimumSize(196,45);

    label_orp_orp_data->setMinimumSize(196,45);
    //label_orp_pH_data->setMinimumSize(196,45);
    label_orp_temp_data->setMinimumSize(196,45);

    label_ph_ph_data->setMinimumSize(196,45);
    label_ph_orp_data->setMinimumSize(196,45);
    label_ph_temp_data->setMinimumSize(196,45);

    label_chla_chla_data->setMinimumSize(196,45);
    label_chla_temp_data->setMinimumSize(196,45);

    label_bga_bga_data->setMinimumSize(196,45);
    label_bga_temp_data->setMinimumSize(196,45);

    label_oil_oil_data->setMinimumSize(196,45);
    label_oil_temp_data->setMinimumSize(196,45);


    label_cod_toc_data->setMaximumSize(196,45);
    label_cod_turb_data->setMaximumSize(196,45);
    label_cod_temp_data->setMaximumSize(196,45);

    label_nh_ph_data->setMaximumSize(196,45);
    label_nh_orp_data->setMaximumSize(196,45);
    label_nh_nh4_data->setMaximumSize(196,45);
    label_nh_k_data->setMaximumSize(196,45);
    label_nh_temp_data->setMaximumSize(196,45);
    label_nh_nh4_n_data->setMaximumSize(196,45);

    label_oxygen_do_data->setMaximumSize(196,45);
    label_oxygen_temp_data->setMaximumSize(196,45);
    label_oxygen_saturation_data->setMaximumSize(196,45);

    label_turb_turb_data->setMaximumSize(196,45);
    label_turb_temp_data->setMaximumSize(196,45);

    label_cond_cond_data->setMaximumSize(196,45);
    label_cond_temp_data->setMaximumSize(196,45);

    label_orp_orp_data->setMaximumSize(196,45);
    //label_orp_pH_data->setMaximumSize(196,45);
    label_orp_temp_data->setMaximumSize(196,45);

    label_ph_ph_data->setMaximumSize(196,45);
    label_ph_orp_data->setMaximumSize(196,45);
    label_ph_temp_data->setMaximumSize(196,45);

    label_chla_chla_data->setMaximumSize(196,45);
    label_chla_temp_data->setMaximumSize(196,45);

    label_bga_bga_data->setMaximumSize(196,45);
    label_bga_temp_data->setMaximumSize(196,45);

    label_oil_oil_data->setMaximumSize(196,45);
    label_oil_temp_data->setMaximumSize(196,45);


    label_cod_toc_data->setAlignment(Qt::AlignCenter);
    label_cod_turb_data->setAlignment(Qt::AlignCenter);
    label_cod_temp_data->setAlignment(Qt::AlignCenter);

    label_nh_ph_data->setAlignment(Qt::AlignCenter);
    label_nh_orp_data->setAlignment(Qt::AlignCenter);
    label_nh_nh4_data->setAlignment(Qt::AlignCenter);
    label_nh_k_data->setAlignment(Qt::AlignCenter);
    label_nh_temp_data->setAlignment(Qt::AlignCenter);
    label_nh_nh4_n_data->setAlignment(Qt::AlignCenter);

    label_oxygen_do_data->setAlignment(Qt::AlignCenter);
    label_oxygen_temp_data->setAlignment(Qt::AlignCenter);
    label_oxygen_saturation_data->setAlignment(Qt::AlignCenter);

    label_turb_turb_data->setAlignment(Qt::AlignCenter);
    label_turb_temp_data->setAlignment(Qt::AlignCenter);

    label_cond_cond_data->setAlignment(Qt::AlignCenter);
    label_cond_temp_data->setAlignment(Qt::AlignCenter);

    label_orp_orp_data->setAlignment(Qt::AlignCenter);
    //label_orp_pH_data->setAlignment(Qt::AlignCenter);
    label_orp_temp_data->setAlignment(Qt::AlignCenter);

    label_ph_ph_data->setAlignment(Qt::AlignCenter);
    label_ph_orp_data->setAlignment(Qt::AlignCenter);
    label_ph_temp_data->setAlignment(Qt::AlignCenter);

    label_chla_chla_data->setAlignment(Qt::AlignCenter);
    label_chla_temp_data->setAlignment(Qt::AlignCenter);

    label_bga_bga_data->setAlignment(Qt::AlignCenter);
    label_bga_temp_data->setAlignment(Qt::AlignCenter);

    label_oil_oil_data->setAlignment(Qt::AlignCenter);
    label_oil_temp_data->setAlignment(Qt::AlignCenter);    
}

void MainWindow::single_search_ok(QString snType,unsigned char id){
    QString style = "QPushButton{background-color:rgb(53, 171, 93);"
                    "border: 2px groove green;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    ui->label_real_id->setNum(id);
    if(!QString::compare(MyData::codSn,snType)){
        ui->textEdit_all->append("<font color='green'>搜索到COD探头，id为：</font>"+QString::number(id));       
        ui->pushButton_cod->setStyleSheet(style);
        color[0] = 1;
        query.prepare("select slave_id from probeInfo where sn = '47'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::nhSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到y氨氮探头，id为：</font>"+QString::number(id));
        ui->pushButton_nh->setStyleSheet(style);
        color[1] = 1;
        MyData::nhType = "nhy";
        query.prepare("select slave_id from probeInfo where sn = '68'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::nhNaSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到n氨氮探头，id为：</font>"+QString::number(id));
        ui->pushButton_nh->setStyleSheet(style);
        color[1] = 1;
        MyData::nhType = "nhn";
        query.prepare("select slave_id from probeInfo where sn = '768'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }


    else if (!QString::compare(MyData::ldoSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到溶解氧探头，id为：</font>"+QString::number(id));
        ui->pushButton_oxygen->setStyleSheet(style);
        color[2] = 1;
        query.prepare("select slave_id from probeInfo where sn = '01'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::turbSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到浊度探头，id为：</font>"+QString::number(id));
        ui->pushButton_turb->setStyleSheet(style);
        color[3] = 1;
        query.prepare("select slave_id from probeInfo where sn = '29'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::condSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到电导率探头，id为：</font>"+QString::number(id));
        ui->pushButton_cond->setStyleSheet(style);
        color[4] = 1;
        query.prepare("select slave_id from probeInfo where sn = '09'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::phSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到ph探头，id为：</font>"+QString::number(id));
        ui->pushButton_ph->setStyleSheet(style);
        color[5] = 1;
        query.prepare("select slave_id from probeInfo where sn = '43'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::orpSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到orp探头，id为：</font>"+QString::number(id));
        ui->pushButton_orp->setStyleSheet(style);
        color[6] = 1;
        query.prepare("select slave_id from probeInfo where sn = '44'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::chlaSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到叶绿素探头，id为：</font>"+QString::number(id));
        ui->pushButton_chla->setStyleSheet(style);
        color[7] = 1;
        query.prepare("select slave_id from probeInfo where sn = '36'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::bgaSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到蓝绿藻探头，id为：</font>"+QString::number(id));
        ui->pushButton_bga->setStyleSheet(style);
        color[8] = 1;
        query.prepare("select slave_id from probeInfo where sn = '62'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::oilSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到N水中油探头，id为：</font>"+QString::number(id));
        ui->pushButton_oil->setStyleSheet(style);
        color[9] = 1;
        MyData::oilType = "oil";
        query.prepare("select slave_id from probeInfo where sn = 'BX-Oil-1N'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::oiwSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到Y水中油探头，id为：</font>"+QString::number(id));
        ui->pushButton_oil->setStyleSheet(style);
        color[9] = 1;
        MyData::oilType = "oiw";
        query.prepare("select slave_id from probeInfo where sn = '63'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare(MyData::sondeSn,snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到多合一探头,id为：</font>"+QString::number(id));
        ui->pushButton_sonde->setStyleSheet(style);
        color[10] = 1;
        query.prepare("select slave_id from probeInfo where sn = '38'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare("",snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到全光谱探头，id为：</font>"+QString::number(id));
        ui->pushButton_cod->setStyleSheet(style);
        color[11] = 1;
        query.prepare("select slave_id from probeInfo where sn = '47'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare("",snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到余氯探头，id为：</font>"+QString::number(id));
        ui->pushButton_cod->setStyleSheet(style);
        color[12] = 1;
        query.prepare("select slave_id from probeInfo where sn = '47'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
    else if (!QString::compare("",snType)) {
        ui->textEdit_all->append("<font color='green'>搜索到硝酸盐氮探头，id为：</font>"+QString::number(id));
        ui->pushButton_cod->setStyleSheet(style);
        color[13] = 1;
        query.prepare("select slave_id from probeInfo where sn = '47'");
        query.exec();
        while (query.next()) {
            if(id != query.value(0).toInt()){
                ui->checkBox_repeat->setChecked(true);
            }
        }
    }
}


void MainWindow::default_search_ok(QString *snTypes,unsigned char count){
    //qDebug()<<"count :"<<count<<"sntype[0]:"<<snTypes[0];;
    delete ui->scrollAreaWidgetContents->layout();  //删除原有界面布局，然后进行新的布局
    int row = 0;
    int col = 0;
    ui->groupBox_sonde_ph->setHidden(true);
    ui->groupBox_sonde_bga->setHidden(true);
    ui->groupBox_sonde_ldo->setHidden(true);
    ui->groupBox_sonde_orp->setHidden(true);
    ui->groupBox_sonde_chla->setHidden(true);
    ui->groupBox_sonde_cond->setHidden(true);
    ui->groupBox_sonde_turb->setHidden(true);
    ui->groupBox_sonde_temp->setHidden(true);

    ui->groupBox_cod_cod->setHidden(true);
    box_cod_temp->setHidden(true);
    box_cod_toc->setHidden(true);
    box_cod_turb->setHidden(true);

    box_nh_ph->setHidden(true);
    box_nh_orp->setHidden(true);
    box_nh_nh4->setHidden(true);
    box_nh_k->setHidden(true);
    box_nh_nh4_n->setHidden(true);
    box_nh_temp->setHidden(true);

    box_oxygen_do->setHidden(true);
    box_oxygen_temp->setHidden(true);

    box_turb_turb->setHidden(true);
    box_turb_temp->setHidden(true);

    box_cond_cond->setHidden(true);
    box_cond_temp->setHidden(true);

    box_ph_ph->setHidden(true);
    box_ph_orp->setHidden(true);
    box_ph_temp->setHidden(true);

    box_orp_orp->setHidden(true);
    box_orp_temp->setHidden(true);

    box_chla_chla->setHidden(true);
    box_chla_temp->setHidden(true);

    box_bga_bga->setHidden(true);
    box_bga_temp->setHidden(true);

    box_oil_oil->setHidden(true);
    box_oil_temp->setHidden(true);


    ui->pushButton_dault_search->setEnabled(true);
    ui->pushButton_dan_search->setEnabled(true);
    ui->pushButton_start_read->setEnabled(true);

    if(count == 0){
        ui->textEdit_all->append("<font color='red'>未搜索到探头</font>");
        return;
    }
    else {
        ui->textEdit_all->append("<font color='green'>搜索结束</font>");       
    }
    qDebug()<<"默认搜索";


    QGridLayout *layout = new QGridLayout ();

    for (uint8_t i = 0; i < count; i++) {
        if(!QString::compare(snTypes[i],MyData::codSn)){
            color[0] = 1;
            layout->addWidget(ui->groupBox_cod_cod,row/3,col%3);
            ui->groupBox_cod_cod->setHidden(false);
            row++;
            col++;
            layout->addWidget(box_cod_temp,row/3,col%3);
            box_cod_temp->setHidden(false);
            row++;
            col++;
            layout->addWidget(box_cod_toc,row/3,col%3);
            box_cod_toc->setHidden(false);
            row++;
            col++;
            layout->addWidget(box_cod_turb,row/3,col%3);
            box_cod_turb->setHidden(false);
            row++;
            col++;
        }
        else if (!QString::compare(MyData::nhSn,snTypes[i])) {
            color[1] = 1;
            box_nh_ph->setHidden(false);
            layout->addWidget(box_nh_ph,row/3,col%3);
            row++;
            col++;
            box_nh_orp->setHidden(false);
            layout->addWidget(box_nh_orp,row/3,col%3);
            row++;
            col++;
            box_nh_nh4->setHidden(false);
            layout->addWidget(box_nh_nh4,row/3,col%3);
            row++;
            col++;
            box_nh_nh4_n->setHidden(false);
            layout->addWidget(box_nh_nh4_n,row/3,col%3);
            row++;
            col++;
            box_nh_temp->setHidden(false);
            layout->addWidget(box_nh_temp,row/3,col%3);
            row++;
            col++;
        }
        else if (!QString::compare(MyData::nhNaSn,snTypes[i])) {
            color[1] = 1;
            box_nh_ph->setHidden(false);
            layout->addWidget(box_nh_ph,row/3,col%3);
            row++;
            col++;
            box_nh_nh4_n->setHidden(false);
            layout->addWidget(box_nh_nh4_n,row/3,col%3);
            row++;
            col++;
            box_nh_temp->setHidden(false);
            layout->addWidget(box_nh_temp,row/3,col%3);
            row++;
            col++;
        }
//        else if (!QString::compare(MyData::nhNaSn,snTypes[i])) {
//            color[1] = 1;
//            box_nh_ph->setHidden(false);
//            layout->addWidget(box_nh_ph,row/3,col%3);
//            row++;
//            col++;
//            box_nh_orp->setHidden(false);
//            layout->addWidget(box_nh_orp,row/3,col%3);
//            row++;
//            col++;
//            box_nh_nh4->setHidden(false);
//            layout->addWidget(box_nh_nh4,row/3,col%3);
//            row++;
//            col++;
//            box_nh_nh4_n->setHidden(false);
//            layout->addWidget(box_nh_nh4_n,row/3,col%3);
//            row++;
//            col++;
//            box_nh_temp->setHidden(false);
//            layout->addWidget(box_nh_temp,row/3,col%3);
//            row++;
//            col++;
//        }
        else if (!QString::compare(MyData::ldoSn,snTypes[i])) {
            color[2] = 1;
            box_oxygen_do->setHidden(false);
            layout->addWidget(box_oxygen_do,row/3,col%3);
            row++;
            col++;
            box_oxygen_temp->setHidden(false);
            layout->addWidget(box_oxygen_temp,row/3,col%3);
            row++;
            col++;
            box_oxygen_saturation->setHidden(false);
            layout->addWidget(box_oxygen_saturation,row/3,col%3);
            row++;
            col++;
        }
        else if (!QString::compare(MyData::turbSn,snTypes[i])) {
            color[3] = 1;
            box_turb_turb->setHidden(false);
            layout->addWidget(box_turb_turb,row/3,col%3);
            row++;
            col++;
            box_turb_temp->setHidden(false);
            layout->addWidget(box_turb_temp,row/3,col%3);
            row++;
            col++;
        }
        else if (!QString::compare(MyData::condSn,snTypes[i])){
            color[4] = 1;
            box_cond_cond->setHidden(false);
            layout->addWidget(box_cond_cond,row/3,col%3);
            row++;
            col++;
            box_cond_temp->setHidden(false);
            layout->addWidget(box_cond_temp,row/3,col%3);
            row++;
            col++;
        }

        else if (!QString::compare(MyData::phSn,snTypes[i])){
            color[5] = 1;
            box_ph_ph->setHidden(false);
            layout->addWidget(box_ph_ph,row/3,col%3);
            row++;
            col++;
            box_ph_orp->setHidden(false);
            layout->addWidget(box_ph_orp,row/3,col%3);
            row++;
            col++;
            box_ph_temp->setHidden(false);
            layout->addWidget(box_ph_temp,row/3,col%3);
            row++;
            col++;
        }

        else if (!QString::compare(MyData::orpSn,snTypes[i])){
            color[6] = 1;
            box_orp_orp->setHidden(false);
            layout->addWidget(box_orp_orp,row/3,col%3);
            row++;
            col++;
            box_orp_temp->setHidden(false);
            layout->addWidget(box_orp_temp,row/3,col%3);
            row++;
            col++;
        }

        else if(!QString::compare(MyData::chlaSn,snTypes[i])){
            color[7] = 1;
            box_chla_chla->setHidden(false);
            layout->addWidget(box_chla_chla,row/3,col%3);
            row++;
            col++;
            box_chla_temp->setHidden(false);
            layout->addWidget(box_chla_temp,row/3,col%3);
            row++;
            col++;
        }

        else if(!QString::compare(MyData::bgaSn,snTypes[i])){
            color[8] = 1;
            box_bga_bga->setHidden(false);
            layout->addWidget(box_bga_bga,row/3,col%3);
            row++;
            col++;
            box_bga_temp->setHidden(false);
            layout->addWidget(box_bga_temp,row/3,col%3);
            row++;
            col++;
        }

        else if (!QString::compare("38",snTypes[i])) {
           color[10] = 1;
           ui->groupBox_sonde_ph->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_ph,row/3,col%3);
           row++;
           col++;
           ui->groupBox_sonde_bga->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_bga,row/3,col%3);

           row++;
           col++;
           ui->groupBox_sonde_ldo->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_ldo,row/3,col%3);

           row++;
           col++;
           ui->groupBox_sonde_orp->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_orp,row/3,col%3);

           row++;
           col++;
           ui->groupBox_sonde_chla->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_chla,row/3,col%3);

           row++;
           col++;
           ui->groupBox_sonde_cond->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_cond,row/3,col%3);

           row++;
           col++;
           ui->groupBox_sonde_turb->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_turb,row/3,col%3);

           row++;
           col++;
           ui->groupBox_sonde_temp->setHidden(false);
           layout->addWidget(ui->groupBox_sonde_temp,row/3,col%3);

           row++;
           col++;
        }
        //纳宏水中油
        else if (!QString::compare("BX-Oil-1N",snTypes[i])){
            qDebug()<<"搜索到纳宏水中油";
            color[9] = 1;
            box_oil_oil->setHidden(false);
            layout->addWidget(box_oil_oil,row/3,col%3);
            row++;
            col++;
            box_oil_temp->setHidden(false);
            layout->addWidget(box_oil_temp,row/3,col%3);
            row++;
            col++;
        }
        //虞山水中油
        else if (!QString::compare("63",snTypes[i])){
            color[9] = 1;
            box_oil_oil->setHidden(false);
            layout->addWidget(box_oil_oil,row/3,col%3);
            row++;
            col++;
            box_oil_temp->setHidden(false);
            layout->addWidget(box_oil_temp,row/3,col%3);
            row++;
            col++;
        }
    }
    while(row < 9){
        layout->addWidget(label1,row/3,col%3);
        row++;
        col++;
    }
    ui->scrollAreaWidgetContents->setLayout(layout);
    delete [] snTypes;


    if(autoRead){
        //自动读取数据和拉in接数据库
        ui->pushButton_sql_conn->emit clicked();
        sleep(500);
        qDebug()<<"开始调用开始读取数据函数";
        ui->pushButton_start_read->emit clicked();

    }



}

/**
 * @brief MainWindow::test1_ok  显示一点校准时的测试数据
 * @param type                  探头类型
 */
void MainWindow::test1_ok(float data){
    if(ui->pushButton_start_read->text() == "停止读取数据" || ui->pushButton_read_data->text() == " 停止测量 "){
        //emit run(true);
    }
    if(data == -1){
        ui->textEdit_setTantou_data->append("<font color='red'>获取探头测试数据失败</font>");
        return;
    }
    ui->textEdit_setTantou_data->append("<font color='green'>获取探头测试数据成功  </font>"+QString::number(data,'f'));
    ui->label_test1_value->setText(QString::number(data,'f'));
}

/**
 * @brief MainWindow::oil_one_cal_ok    水中油一点定标信息
 * @param str   探头返回信息
 */
void MainWindow::oil_one_cal_ok(QString str){
    ui->textEdit_setTantou_data->append(str);
//    emit run(true);
}


/**
 * @brief MainWindow::com_open_ok   判断串口是否打开
 * @param portName
 * @param ok        ok==true:打开  ok==false:未打开
 */
void MainWindow::com_open_ok(QString portName,bool ok){
    QString online = "QPushButton{background-color:rgb(53, 171, 93);"
                    "border: 2px groove green; padding-left:5px; padding-right:5px; border-style: outset;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString drop = "QPushButton{background-color:red;"
                    "border: 2px groove red; padding-left:5px; padding-right:5px; border-style: outset;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString nosearch = "QPushButton{background-color:rgb(225, 225, 225);"
                    "border: 2px groove gray; padding-left:5px; padding-right:5px; border-style: outset;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    if(ok){
        ui->label_com_open->setText(portName+" is opened");
        ui->pushButton_port_conn->setText("关闭连接");
        ui->textEdit_all->append(portName+"<font color='green'>端口打开成功</font>");
        ui->label_com_open->setStyleSheet("QLabel{color:green;font: 14px}");
        ui->pushButton_dan_search->setEnabled(true);
        ui->pushButton_dault_search->setEnabled(true);


        if(color[0] == 0){
            ui->pushButton_cod->setStyleSheet(nosearch);
        }
        else if (color[0] == 1) {
            ui->pushButton_cod->setStyleSheet(online);
        }


        if(color[1] == 0){
            ui->pushButton_nh->setStyleSheet(nosearch);
        }
        else if (color[1] == 1) {
            ui->pushButton_nh->setStyleSheet(online);
        }


        if(color[2] == 0){
            ui->pushButton_oxygen->setStyleSheet(nosearch);
        }
        else if (color[2] == 1) {
            ui->pushButton_oxygen->setStyleSheet(online);
        }



        if(color[3] == 0){
            ui->pushButton_turb->setStyleSheet(nosearch);
        }
        else if (color[3] == 1) {
            ui->pushButton_turb->setStyleSheet(online);
        }



        if(color[4] == 0){
            ui->pushButton_cond->setStyleSheet(nosearch);
        }
        else if (color[4] == 1) {
            ui->pushButton_cond->setStyleSheet(online);
        }



        if(color[5] == 0){
            ui->pushButton_ph->setStyleSheet(nosearch);
        }
        else if (color[5] == 1) {
            ui->pushButton_ph->setStyleSheet(online);
        }



        if(color[6] == 0){
            ui->pushButton_orp->setStyleSheet(nosearch);
        }
        else if (color[6] == 1) {
            ui->pushButton_orp->setStyleSheet(online);
        }



        if(color[7] == 0){
            ui->pushButton_chla->setStyleSheet(nosearch);
        }
        else if (color[7] == 1) {
            ui->pushButton_chla->setStyleSheet(online);
        }



        if(color[8] == 0){
            ui->pushButton_bga->setStyleSheet(nosearch);
        }
        else if (color[8] == 1) {
            ui->pushButton_bga->setStyleSheet(online);
        }


        if(color[9] == 0){
            ui->pushButton_oil->setStyleSheet(nosearch);
        }
        else if (color[9] == 1) {
            ui->pushButton_oil->setStyleSheet(online);
        }


        if(color[10] == 0){
            ui->pushButton_sonde->setStyleSheet(nosearch);
        }
        else if (color[10] == 1) {
            ui->pushButton_sonde->setStyleSheet(online);
        }

        if(color[11] == 0){
            ui->pushButton_spectrum->setStyleSheet(nosearch);
        }
        else if (color[11] == 1) {
            ui->pushButton_spectrum->setStyleSheet(online);
        }


        if(color[12] == 0){
            ui->pushButton__chlorine->setStyleSheet(nosearch);
        }
        else if (color[12] == 1) {
            ui->pushButton__chlorine->setStyleSheet(online);
        }


        if(color[13] == 0){
            ui->pushButton_nitrate_nitrogen->setStyleSheet(nosearch);
        }
        else if (color[13] == 1) {
            ui->pushButton_nitrate_nitrogen->setStyleSheet(online);
        }

        //软件启动后自动搜索
        if(autoRead)
            ui->pushButton_dault_search->emit clicked();
    }

    else {
        ui->label_com_open->setText(portName+" is closed");
        ui->pushButton_port_conn->setText("端口连接");
        ui->textEdit_all->append("<font color='red'>端口未打开</font>");
        ui->label_com_open->setStyleSheet("QLabel{color:red;font: 14px}");

        //ui->pushButton_start_read->setText("开始读取数据");
        //ui->pushButton_read_data->setText(" 启动测量 ");
        ui->pushButton_dan_search->setEnabled(false);
        ui->pushButton_dault_search->setEnabled(false);

        if(color[0] == 0){
            ui->pushButton_cod->setStyleSheet(nosearch);
        }
        else if (color[0] == 1) {
            ui->pushButton_cod->setStyleSheet(drop);
        }


        if(color[1] == 0){
            ui->pushButton_nh->setStyleSheet(nosearch);
        }
        else if (color[1] == 1) {
            ui->pushButton_nh->setStyleSheet(drop);
        }


        if(color[2] == 0){
            ui->pushButton_oxygen->setStyleSheet(nosearch);
        }
        else if (color[2] == 1) {
            ui->pushButton_oxygen->setStyleSheet(drop);
        }



        if(color[3] == 0){
            ui->pushButton_turb->setStyleSheet(nosearch);
        }
        else if (color[3] == 1) {
            ui->pushButton_turb->setStyleSheet(drop);
        }



        if(color[4] == 0){
            ui->pushButton_cond->setStyleSheet(nosearch);
        }
        else if (color[4] == 1) {
            ui->pushButton_cond->setStyleSheet(drop);
        }


        if(color[5] == 0){
            ui->pushButton_ph->setStyleSheet(nosearch);
        }
        else if (color[5] == 1) {
            ui->pushButton_ph->setStyleSheet(drop);
        }


        if(color[6] == 0){
            ui->pushButton_orp->setStyleSheet(nosearch);
        }
        else if (color[6] == 1) {
            ui->pushButton_orp->setStyleSheet(drop);
        }



        if(color[7] == 0){
            ui->pushButton_chla->setStyleSheet(nosearch);
        }
        else if (color[7] == 1) {
            ui->pushButton_chla->setStyleSheet(drop);
        }



        if(color[8] == 0){
            ui->pushButton_bga->setStyleSheet(nosearch);
        }
        else if (color[8] == 1) {
            ui->pushButton_bga->setStyleSheet(drop);
        }



        if(color[9] == 0){
            ui->pushButton_oil->setStyleSheet(nosearch);
        }
        else if (color[9] == 1) {
            ui->pushButton_oil->setStyleSheet(drop);
        }


        if(color[10] == 0){
            ui->pushButton_sonde->setStyleSheet(nosearch);
        }
        else if (color[10] == 1) {
            ui->pushButton_sonde->setStyleSheet(drop);
        }

        if(color[11] == 0){
            ui->pushButton_spectrum->setStyleSheet(nosearch);
        }
        else if (color[11] == 1) {
            ui->pushButton_spectrum->setStyleSheet(drop);
        }


        if(color[12] == 0){
            ui->pushButton__chlorine->setStyleSheet(nosearch);
        }
        else if (color[12] == 1) {
            ui->pushButton__chlorine->setStyleSheet(drop);
        }


        if(color[13] == 0){
            ui->pushButton_nitrate_nitrogen->setStyleSheet(nosearch);
        }
        else if (color[13] == 1) {
            ui->pushButton_nitrate_nitrogen->setStyleSheet(drop);
        }
    }
}


/**
 * @brief MainWindow::cal_param_ok  设置校准参数是否成功
 * @param k     校准后的k值
 * @param b     校准后的b值
 * @param ok    ok==1:校准成功  ok==-1:校准失败     ok==0:读取到当前K,B值
 */
void MainWindow::cal_param_ok(float k,float b, int ok){
    if(ok == 1){
        ui->textEdit_setTantou_data->append("<font color='green'>校准成功</font>");
        ui->label_k->setText(QString::number(k));
        ui->label_b->setText(QString::number(b));
    }
    else if(ok == -1){
        ui->textEdit_setTantou_data->append("<font color='red'>校准失败</font>");
    }
    else if (ok == 0) {
        ui->textEdit_setTantou_data->append("当前k值："+QString::number(k)+"  b值："+QString::number(b));
        ui->label_k->setText(QString::number(k));
        ui->label_b->setText(QString::number(b));
    }
}


/**
 * @brief MainWindow::oil_clear_num_ok  水中油自动清扫模式设置清扫次数是否成功
 * @param num
 * @param ok    ok==true:成功  ok==false:失败
 */
void MainWindow::oil_clear_num_ok(unsigned char num,bool ok){
    if(ok){
        ui->textEdit_setTantou_data->append("<font color='green'>水中油清扫次数设置成功</font>");
        ui->label_clear_num->setNum(num);
    }
    else {
        ui->textEdit_setTantou_data->append("<font color='green'>水中油清扫次数设置失败</font>");
    }
}

/**
 * @brief MainWindow::sql_conn_ok   判读数据库连接或断开是否成功
 * @param ok    true:成功  false:失败
 * @param conn  true:连接  false：断开
 */
void MainWindow:: sql_conn_ok(bool ok,bool conn){
    if(conn){
        if(ok){
            ui->textEdit_sql->append("<font color='green'>数据库连接成功</font>");
            ui->label_sql_conn->setText("已连接");
            ui->label_sql_conn->setStyleSheet("QLabel{background-color:green;}");
            MyData::firstSave = 0;

        }
        else {
            ui->textEdit_sql->append("<font color='red'>数据库连接失败</font>");
            ui->label_sql_conn->setText("断开");
            ui->label_sql_conn->setStyleSheet("QLabel{background-color:red;}");
        }
        ui->pushButton_sql_conn->setEnabled(true);
    }
    else {
        if(ok){
            ui->textEdit_sql->append("<font color='red'>数据库已断开</font>");
            ui->label_sql_conn->setText("断开");
            ui->label_sql_conn->setStyleSheet("QLabel{background-color:red;}");
        }
        else {
            ui->textEdit_sql->append("<font color='red'>数据库断开失败</font>");
            ui->label_sql_conn->setText("已连接");
            ui->label_sql_conn->setStyleSheet("QLabel{background-color:green;}");
        }
        ui->pushButton_sql_close->setEnabled(true);
    }
}


void MainWindow::changeStatus(QString type,QString status){
    QString online = "QPushButton{background-color:rgb(53, 171, 93);"
                    "border: 2px groove green;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString drop = "QPushButton{background-color:red;"
                    "border: 2px groove red;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString selectOnline = "QPushButton{background:qlineargradient(spread:reflect,"
                           " x1:0, y1:0, x2:1, y2:0, stop:0.0738636 rgba(0, 206, 0, 255), "
                           "stop:0.0965909 rgba(52, 151, 219, 255));"
                    "border: 2px groove rgb(67, 104, 226);}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString selectDrop = "QPushButton{background:qlineargradient(spread:reflect, "
                         "x1:0, y1:0, x2:1, y2:0, stop:0.0738636 rgba(255, 0, 0, 255),"
                         " stop:0.102273 rgba(52, 151, 219, 255));"
                    "border: 2px groove rgb(67, 104, 226);}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    if(status == "drop"){
       if(type == MyData::codSn){
           if(ui->label_current_tantou->text() == "COD"){
               ui->pushButton_cod->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_cod->setStyleSheet(drop);
           }
           color[0] = 2;
       }
       else if(type == MyData::nhSn){
           if(ui->label_current_tantou->text() == "氨氮"){
               ui->pushButton_nh->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_nh->setStyleSheet(drop);
           }

           color[1] = 2;
      }
       else if(type == MyData::ldoSn){
           if( ui->label_current_tantou->text() == "溶解氧"){
               ui->pushButton_oxygen->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_oxygen->setStyleSheet(drop);
           }

           color[2] = 2;
      }
       else if(type == MyData::turbSn){
           if(ui->label_current_tantou->text() == "浊度"){
               ui->pushButton_turb->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_turb->setStyleSheet(drop);
           }

           color[3] = 2;
      }
       else if(type == MyData::condSn ){
           if(ui->label_current_tantou->text() == "电导率"){
               ui->pushButton_cond->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_cond->setStyleSheet(drop);
           }

           color[4] = 2;
      }
       else if(type == MyData::phSn  ){
           if(ui->label_current_tantou->text() == "pH"){
               ui->pushButton_ph->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_ph->setStyleSheet(drop);
           }

           color[5] = 2;
      }
       else if(type == MyData::orpSn ){
           if(ui->label_current_tantou->text() == "ORP"){
               ui->pushButton_orp->setStyleSheet(selectDrop);
           }else{
               ui->pushButton_orp->setStyleSheet(drop);
           }

           color[6] = 2;
      }
       else if(type == MyData::chlaSn){
           if(ui->label_current_tantou->text() == "叶绿素a"){
                ui->pushButton_chla->setStyleSheet(selectDrop);
           }else{
                ui->pushButton_chla->setStyleSheet(drop);
           }

           color[7] = 2;
      }
       else if(type == MyData::bgaSn ){
           if(ui->label_current_tantou->text() == "蓝绿藻"){
                ui->pushButton_bga->setStyleSheet(selectDrop);
           }else{
                ui->pushButton_bga->setStyleSheet(drop);
           }

           color[8] = 2;
      }
       else if(type == MyData::oilSn){
           if(ui->label_current_tantou->text() == "水中油"){
                ui->pushButton_oil->setStyleSheet(selectDrop);
           }else{
                ui->pushButton_oil->setStyleSheet(drop);
           }

           color[9] = 2;
      }
       else if(type == MyData::sondeSn){
           if( ui->label_current_tantou->text() == "多合一"){
                ui->pushButton_sonde->setStyleSheet(selectDrop);
           }else{
                ui->pushButton_sonde->setStyleSheet(drop);
           }

           color[10] = 2;
      }
       else if(type == MyData::specSn){
           if(ui->label_current_tantou->text() == "全光谱"){
                ui->pushButton_spectrum->setStyleSheet(selectDrop);
           }else{
                ui->pushButton_spectrum->setStyleSheet(drop);
           }

           color[11] = 2;
      }
       else if(type == MyData::chloSn){
           if(ui->label_current_tantou->text() == "余氯"){
                ui->pushButton__chlorine->setStyleSheet(selectDrop);
           }else{
                ui->pushButton__chlorine->setStyleSheet(drop);
           }

           color[12] = 2;
      }
       else if(type == MyData::nitrSn ){
           if(ui->label_current_tantou->text() == "硝酸盐氮"){
                ui->pushButton_nitrate_nitrogen->setStyleSheet(selectDrop);
           }else{
                ui->pushButton_nitrate_nitrogen->setStyleSheet(drop);
           }

           color[13] = 2;
      }
    }

    else if (status == "online") {
        if(type == MyData::codSn){
            if(ui->label_current_tantou->text() == "cod"){
                ui->pushButton_cod->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_cod->setStyleSheet(online);
            }
            color[0] = 1;
        }
        else if(type == MyData::nhSn){
            if(ui->label_current_tantou->text() == "氨氮"){
                ui->pushButton_nh->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_nh->setStyleSheet(online);
            }

            color[1] = 1;
       }
        else if(type == MyData::ldoSn){
            if( ui->label_current_tantou->text() == "溶解氧"){
                ui->pushButton_oxygen->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_oxygen->setStyleSheet(online);
            }

            color[2] = 1;
       }
        else if(type == MyData::turbSn){
            if(ui->label_current_tantou->text() == "浊度"){
                ui->pushButton_turb->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_turb->setStyleSheet(online);
            }

            color[3] = 1;
       }
        else if(type == MyData::condSn ){
            if(ui->label_current_tantou->text() == "电导率"){
                ui->pushButton_cond->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_cond->setStyleSheet(online);
            }

            color[4] = 1;
       }
        else if(type == MyData::phSn  ){
            if(ui->label_current_tantou->text() == "pH"){
                ui->pushButton_ph->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_ph->setStyleSheet(online);
            }

            color[5] = 1;
       }
        else if(type == MyData::orpSn ){
            if(ui->label_current_tantou->text() == "ORP"){
                ui->pushButton_orp->setStyleSheet(selectOnline);
            }else{
                ui->pushButton_orp->setStyleSheet(online);
            }

            color[6] = 1;
       }
        else if(type == MyData::chlaSn){
            if(ui->label_current_tantou->text() == "叶绿素a"){
                 ui->pushButton_chla->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton_chla->setStyleSheet(online);
            }

            color[7] = 1;
       }
        else if(type == MyData::bgaSn ){
            if(ui->label_current_tantou->text() == "蓝绿藻"){
                 ui->pushButton_bga->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton_bga->setStyleSheet(online);
            }

            color[8] = 1;
       }
        else if(type == MyData::oilSn){
            if(ui->label_current_tantou->text() == "水中油"){
                 ui->pushButton_oil->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton_oil->setStyleSheet(online);
            }

            color[9] = 1;
       }
        else if(type == MyData::sondeSn){
            if( ui->label_current_tantou->text() == "多合一"){
                 ui->pushButton_sonde->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton_sonde->setStyleSheet(online);
            }

            color[10] = 1;
       }
        else if(type == MyData::specSn){
            if(ui->label_current_tantou->text() == "全光谱"){
                 ui->pushButton_spectrum->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton_spectrum->setStyleSheet(online);
            }

            color[11] = 1;
       }
        else if(type == MyData::chloSn){
            if(ui->label_current_tantou->text() == "余氯"){
                 ui->pushButton__chlorine->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton__chlorine->setStyleSheet(online);
            }

            color[12] = 1;
       }
        else if(type == MyData::nitrSn ){
            if(ui->label_current_tantou->text() == "硝酸盐氮"){
                 ui->pushButton_nitrate_nitrogen->setStyleSheet(selectOnline);
            }else{
                 ui->pushButton_nitrate_nitrogen->setStyleSheet(online);
            }

            color[13] = 1;
       }
    }    
}


void MainWindow::save_data_ok(){
    ui->textEdit_sql->append("<table cellpadding='0' cellspacing='5'><tr>"
                             "<td align='left' width='210'>时间</td>"
                             "<td align='left' width='100'>探头类型</td>"
                             "<td align='left' width='100'>参数</td>"
                             "<td align='left' width='100'>数据</td>"
                             "<td align='left' width='100'>单位</td></tr></table>");
    for (int i = 0; i < MyData::timeList.size(); i++) {
        ui->textEdit_sql->append("<table cellpadding='0' cellspacing='5'><tr>"
                                 "<td align='left' width='210'>"+MyData::timeList.value(i).toString()+"</td>"
                                 "<td align='left' width='100'>"+MyData::probeList.value(i).toString()+"</td>"
                                 "<td align='left' width='100'>"+MyData::paramList.value(i).toString()+"</td>"
                                 "<td align='left' width='100'>"+MyData::dataList.value(i).toString()+"</td>"
                                 "<td align='left' width='100'>"+MyData::unitList.value(i).toString()+"</td></tr></table>");
    }
    MyData::timeList.clear();
    MyData::probeList.clear();
    MyData::paramList.clear();
    MyData::dataList.clear();
    MyData::unitList.clear();
}

/**
 * @brief MainWindow::get_sn_ok 获取探头序列号
 * @param sn
 */
void MainWindow::get_sn_ok(QString sn){
    ui->label_sn->setText(sn);
}


/**
 * @brief MainWindow::id_ok 修改地址成功，更新界面显示
 * @param addr
 */
void MainWindow::id_ok(int addr){
    if(addr == -1){
        ui->textEdit_setTantou_data->append("<font color='red'>地址与其他探头地址冲突，，请重新设置</font>");
        return;
    }
    else if (addr == 0) {
       ui->textEdit_setTantou_data->append("<font color='red'>当前探头不存在,已掉线,已损坏或地址不正确</font>");
       return;
    }
    ui->label_real_id->setNum(addr);
    ui->checkBox_repeat->setChecked(false);
    ui->textEdit_setTantou_data->append("<font color='green'>地址修改成功！</font>");
}



void MainWindow::time_ok(int time){
    if(time == -1){
        ui->textEdit_setTantou_data->append("<font color='red'>转动间隔修改失败，请重新设置</font>");
        return;
    }
    else if (time == 0) {
       ui->textEdit_setTantou_data->append("<font color='red'>当前探头不存在,已掉线,已损坏或地址不正确</font>");
       return;
    }

    ui->label_rotation->setNum(time);
    ui->textEdit_setTantou_data->append("<font color='green'>转动间隔修改成功！</font>");
}

void MainWindow::on_pushButton_cod_clicked()
{
    ui->label_current_tantou->setText("COD");      
    ui->label_addr_default->setText("1");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Cod");
    ui->comboBox_cal_param->addItem("Turb");
    ui->groupBox_brush->setEnabled(true);
    query.prepare("select slave_id from probeInfo where sn = '47'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
            ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"cod");
    }
    snType = MyData::codSn;
    first = true;
    set_not_oil_show();
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }

    selectButton(ui->pushButton_cod,color);
}

void MainWindow::on_pushButton_nh_clicked()
{
    ui->label_current_tantou->setText("氨氮");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("2");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("NH4_N");
    ui->comboBox_cal_param->addItem("pH");
    ui->comboBox_cal_param->addItem("NH4+");


    if(MyData::nhType == "nhy"){
        query.prepare("select slave_id from probeInfo where sn = '68'");
    }
    else {
        query.prepare("select slave_id from probeInfo where sn = '768'");
        ui->comboBox_cal_param->removeItem(2);
    }

    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        if(MyData::nhType == "nhy"){
            emit getSNSignal(query.value(0).toInt(),"nhy");
        }
        else {
            emit getSNSignal(query.value(0).toInt(),"naHong");
        }
    }
    snType = MyData::nhSn;
    if(MyData::nhType == "nhn"){
        first = true;
        ui->pushButton_stand1->setText("写入");
        ui->pushButton_stand2->setText("写入");
        ui->pushButton_clear_num->setEnabled(true);
        ui->label_clear_num->setEnabled(true);
        ui->lineEdit_clear_num->setEnabled(true);
        ui->radioButton_auto->setEnabled(true);
        ui->radioButton_hand->setEnabled(true);
        ui->groupBox_brush->setEnabled(true);
    }
    else if(MyData::nhType == "nhy"){
        first = true;
        set_not_oil_show();
        ui->groupBox_brush->setEnabled(true);
    }

//    query.prepare("select slave_id from probeInfo where sn = '68'");
//    query.exec();
//    while (query.next()) {
//        if(!ui->checkBox_repeat->isChecked())
//        ui->label_real_id->setNum(query.value(0).toInt());
//        emit getSNSignal(query.value(0).toInt(),"nh");
//    }
//    snType = MyData::nhSn;
//    first = true;
//    set_not_oil_show();
//    ui->groupBox_brush->setEnabled(true);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_nh,color);
}



void MainWindow::on_pushButton_oxygen_clicked()
{
    ui->label_current_tantou->setText("溶解氧");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("3");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Ldo");
    query.prepare("select slave_id from probeInfo where sn = '01'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"ldo");
    }
    snType = MyData::ldoSn;
    first = true;
    set_not_oil_show();
    ui->groupBox_brush->setEnabled(false);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_oxygen,color);
}

void MainWindow::on_pushButton_turb_clicked()
{
    ui->label_current_tantou->setText("浊度");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("4");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Turb");
    query.prepare("select slave_id from probeInfo where sn = '29'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"turb");
    }
    snType = MyData::turbSn;
    first = true;
    set_not_oil_show();
    ui->groupBox_brush->setEnabled(true);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_turb,color);
}


void MainWindow::on_pushButton_ph_clicked()
{
    ui->label_current_tantou->setText("pH");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("6");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("pH");
    ui->comboBox_cal_param->addItem("orp");
    query.prepare("select slave_id from probeInfo where sn = '43'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"ph");
    }
    snType = MyData::phSn;
    first = true;
    set_not_oil_show();
    ui->groupBox_brush->setEnabled(false);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_ph,color);
}

void MainWindow::on_pushButton_orp_clicked()
{
    ui->label_current_tantou->setText("ORP");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("7");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("orp");
    //ui->comboBox_cal_param->addItem("pH");
    query.prepare("select slave_id from probeInfo where sn = '44'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"orp");
    }
    snType = MyData::orpSn;
    first = true;
    set_not_oil_show();
    ui->groupBox_brush->setEnabled(false);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_orp,color);
}

void MainWindow::on_pushButton_chla_clicked()
{
    ui->label_current_tantou->setText("叶绿素a");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("8");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Chla");
    query.prepare("select slave_id from probeInfo where sn = '36'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"chla");
    }
    snType = MyData::chlaSn;
    first = true;
    set_not_oil_show();
    ui->groupBox_brush->setEnabled(true);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_chla,color);
}


void MainWindow::on_pushButton_oil_clicked()
{
    ui->label_current_tantou->setText("水中油");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("10");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Oil");
    if(MyData::oilType == "oil"){
        query.prepare("select slave_id from probeInfo where sn = 'BX-Oil-1N'");
    }
    else {
        query.prepare("select slave_id from probeInfo where sn = '63'");
    }

    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        if(MyData::oilType == "oil"){
            emit getSNSignal(query.value(0).toInt(),"naHong");
        }
        else {
            emit getSNSignal(query.value(0).toInt(),"oiw");
        }
    }
    snType = MyData::oilSn;
    if(MyData::oilType == "oil"){
        first = true;
        ui->pushButton_stand1->setText("写入");
        ui->pushButton_stand2->setText("写入");
        ui->pushButton_clear_num->setEnabled(true);
        ui->label_clear_num->setEnabled(true);
        ui->lineEdit_clear_num->setEnabled(true);
        ui->radioButton_auto->setEnabled(true);
        ui->radioButton_hand->setEnabled(true);
        ui->groupBox_brush->setEnabled(true);
    }
    else if(MyData::oilType == "oiw"){
        first = true;
        set_not_oil_show();
        ui->groupBox_brush->setEnabled(true);
    }


    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_oil,color);
}


void MainWindow::on_pushButton_spectrum_clicked()
{
    ui->label_current_tantou->setText("全光谱");
   // ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("");
    query.prepare("select slave_id from probeInfo where sn = ''");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"spec");
    }
    first = true;
    set_not_oil_show();
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_spectrum,color);
}

void MainWindow::on_pushButton__chlorine_clicked()
{
    ui->label_current_tantou->setText("余氯");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("");
    query.prepare("select slave_id from probeInfo where sn = ''");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"chlo");
    }
    first = true;
    set_not_oil_show();
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton__chlorine,color);
}

void MainWindow::on_pushButton_nitrate_nitrogen_clicked()
{
    ui->label_current_tantou->setText("硝酸盐氮");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("");
    query.prepare("select slave_id from probeInfo where sn = ''");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"nitr");
    }
    first = true;
    set_not_oil_show();
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_nitrate_nitrogen,color);
}

/**
 * @brief MainWindow::on_pushButton_port_conn_clicked
 *  端口连接
 */
void MainWindow::on_pushButton_port_conn_clicked()
{
    QString portName = ui->comboBox_portName->currentText();
    qint32 baudRate = ui->comboBox_baud_rate->currentText().toInt();
    if(!QString::compare("端口连接",ui->pushButton_port_conn->text())){
        if(!threadExist){
            myThread = new MyThread();
            QThread *thread = new QThread();
            myThread->moveToThread(thread);
            thread->start();
            connInit();
            threadExist = true;
        }
        emit conn_port(portName,baudRate);
    }
    else {
        //MyData::clickedEvent = true;
        qDebug()<<"关闭端口";
        emit close_port();
    }
}

/**
 * @brief MainWindow::on_pushButton_addr_update_clicked
 *  修改子站地址
 */
void MainWindow::on_pushButton_addr_update_clicked()
{
    uint8_t addr_update = ui->lineEdit_addr_update->text().toInt() & 0xff;
    uint8_t real_id = ui->label_real_id->text().toInt() & 0xff;
    if(addr_update != real_id){                               //如果修改的地址与实际地址相同，则不操作
        //ui->label_real_id->setNum(addr_update);
        if(ui->checkBox_repeat->isChecked()){
            MyData::idRepeat = true;
        }
        else {
            MyData::idRepeat = false;
        }
        emit setSlaveId(addr_update,real_id,snType);
    }
}

/**
 * @brief MainWindow::on_pushButton_port_update_clicked
 * 更新串口列表
 */
void MainWindow::on_pushButton_port_update_clicked()
{
    ui->comboBox_portName->clear();
    QString portName;
    bool equel = false;
    query.exec("select value from userConfig where type = 'deafaultPort'");
    while (query.next()) {
        portName = query.value(0).toString();
    }
    foreach( const QSerialPortInfo &Info,QSerialPortInfo::availablePorts()){
        qDebug() << "portName    :"  << Info.portName();//调试时可以看的串口信息
        qDebug() << "Description   :" << Info.description();
        qDebug() << "Manufacturer:" << Info.manufacturer();

        QSerialPort serial;
        serial.setPort(Info);

        if( serial.open( QIODevice::ReadWrite) )//如果串口是可以用读写方式打开的
        {
            if(Info.portName() == portName){
                equel = true;
            }
            ui->comboBox_portName->addItem( Info.portName() );//在comboBox那添加串口号
            serial.close();//然后自动关闭等待人为开启（通过那个打开串口的PushButton）
        }       
    }
    if(equel){
        ui->comboBox_portName->setCurrentText(portName);
    }

}


/**
 * @brief MainWindow::on_pushButton_20_clicked
 * 清除探头设置界面的交互信息
 *
 */
void MainWindow::on_pushButton_20_clicked()
{
    ui->textEdit_setTantou_data->clear();
}

/**
 * @brief MainWindow::on_pushButton_rotation_update_clicked
 * 修改探头转动间隔
 */
void MainWindow::on_pushButton_rotation_update_clicked()
{
    uint8_t real_id = ui->label_real_id->text().toInt() &0xff;
    uint16_t time = ui->lineEdit_rotation_update->text().toInt() &0xff;
    if(time > 0){
        emit setRotationPeriod(real_id,time,snType);
    }
    else {
        ui->textEdit_setTantou_data->append("< font color = 'red'>间隔时间设置不合理 </font>");
    }

}



/**
 * @brief MainWindow::on_pushButton_stand1_clicked 校准方式测试一
 * 获取测试数据和当前K，B值 和测量数据
 */
void MainWindow::on_pushButton_stand1_clicked()
{
    MyData::testSn1 = snType;
    uint8_t real_id = ui->label_real_id->text().toInt() &0xff;
    float oil_stand = ui->lineEdit_stand1->text().toFloat();
    QString param = ui->comboBox_cal_param->currentText();
    //MyData::clickedEvent = true;
    //MyData::isRun = false;
    //emit run(false);


    if(snType == MyData::oilSn){
        if(MyData::oilType == "oil"){
            emit setOilCal(real_id,oil_stand);
            return;
        }
        else if (MyData::oilType == "oiw") {
            emit getCalParameter(real_id,snType,0x11);
        }

    }
    else if(snType == MyData::nhSn){
        qDebug()<<"氨氮类型："<<MyData::nhType;
        if(MyData::nhType == "nhy"){
            qDebug()<<"虞山氨氮标定";
            if(param == "NH4_N"){
                emit getCalParameter(real_id,snType,0x36);
            }
            else if (param == "pH") {
                emit getCalParameter(real_id,snType,0x11);
            }
            else if (param == "NH4+") {
                emit getCalParameter(real_id,snType,0x35);
            }
        }
        else {
            qDebug()<<"纳宏氨氮标定";
            if(abs(oil_stand - 0) < 0.0000001){
                ui->textEdit_setTantou_data->append("标液值不能为0:  ");
                qDebug()<<oil_stand;
                return;
            }
            emit setNaNhCal(real_id,oil_stand,param);
            return;
        }
        //
    }
    else if (snType == MyData::codSn) {
        if(param == "Cod"){
            emit getCalParameter(real_id,snType,0x11);
        }
        else if (param == "Turb") {
            emit getCalParameter(real_id,snType,0x34);
        }
    }
    else if (snType == MyData::phSn || snType == MyData::orpSn) {
        if(param == "pH"){
            emit getCalParameter(real_id,snType,0x11);
        }
        else if (param == "orp") {
            emit getCalParameter(real_id,snType,0x34);
        }
    }

    else if (snType == MyData::sondeSn) {
        if(param == "ldo"){
            emit getCalParameter(real_id,snType,0x01);
        }
        else if (param == "turb") {
            emit getCalParameter(real_id,snType,0x02);
        }
        if(param == "cond"){
            emit getCalParameter(real_id,snType,0x03);
        }
        else if (param == "ph") {
            emit getCalParameter(real_id,snType,0x04);
        }
        if(param == "orp"){
            emit getCalParameter(real_id,snType,0x05);
        }
        else if (param == "chla") {
            emit getCalParameter(real_id,snType,0x06);
        }
        else if (param == "bga") {
            emit getCalParameter(real_id,snType,0x07);
        }
    }

    else {
         emit getCalParameter(real_id,snType,0x11);
    }
    //MyData::clickedEvent = true;
    sleep(500);

    emit getTestData(real_id,snType,true,param);

}



/**
 * @brief MainWindow::on_pushButton_read_data_clicked
 * 开始读取数据或停止读取数据
 */
void MainWindow::on_pushButton_read_data_clicked()
{
    if(!QString::compare(ui->pushButton_port_conn->text(),"端口连接")){
        ui->textEdit_all->append("端口未打开");
        return;
    }
    if(ui->pushButton_read_data->text() == " 启动测量 "){
        ui->pushButton_read_data->setText(" 停止测量 ");
        if(ui->pushButton_start_read->text() == "开始读取数据"){
            emit run(true);
        }
        ui->pushButton_start_read->setText("停止读取数据");

    }
    else {
        ui->pushButton_read_data->setText(" 启动测量 ");
    }

}

void MainWindow::data_type(QByteArray data,QString type){

    QDateTime current_date_time;
    QString current_date;
    current_date_time =QDateTime::currentDateTime();
    current_date =current_date_time.toString("yyyy/MM/dd hh:mm:ss");
    ui->label_time->setText(current_date);

    /**将单探头的数据输出到探头设置页面**************************************/
    if(ui->pushButton_read_data->text() == " 停止测量 "){

        if(!QString::compare(MyData::codSn,type) && this->snType == MyData::codSn){   //cod
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>COD(mg/L)</td>"
                                                    "<td align='left' width='100'>TOC(mg/L)</td>"
                                                    "<td align='left' width='100'>浊度(NTU)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::codTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::codCod)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::codToc)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::codTurb)+"</td></tr></table>");
        }

        else if (!QString::compare(MyData::nhSn,type) && this->snType == MyData::nhSn) {        //氨氮
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>ORP(mg/L)</td>"
                                                    "<td align='left' width='100'>pH(pH)</td>"
                                                    "<td align='left' width='100'>NH4+(mV)</td>"
                                                    "<td align='left' width='120'>NH4_N(mg/L)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::nhTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::nhOrp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::nhPH)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::nhNh4Plus)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::nhNh4_N)+"</td></tr></table>");
        }

        else if (!QString::compare(MyData::nhNaSn,type) && this->snType == MyData::nhSn) {        //氨氮
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>pH(pH)</td>"
                                                    "<td align='left' width='120'>NH4_N(mg/L)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::nhTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::nhPH)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::nhNh4_N)+"</td></tr></table>");
        }

        else if (!QString::compare(MyData::ldoSn,type) && this->snType == MyData::ldoSn) {        //溶解氧
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='120'>溶解氧(mg/L)</td>"
                                                    "<td align='left' width='100'>饱和度(%)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::ldoTemp)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::ldoLdo)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::ldoSaturation)+"</td></tr></table>");
        }

        else if (!QString::compare(MyData::turbSn,type) && this->snType == MyData::turbSn) {        //浊度
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>浊度(NTU)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::turbTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::turbTurb)+"</td></tr></table>");
        }

        else if (!QString::compare(MyData::condSn,type) && this->snType == MyData::condSn) {        //电导率
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='120'>电导率(μS/cm)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::condTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::condCond)+"</td></tr></table>");

        }

        else if (!QString::compare(MyData::phSn,type) && this->snType == MyData::phSn) {        //ph
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>pH(pH)</td>"
                                                    "<td align='left' width='100'>orp(mV)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::phTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::phPH)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::phOrp)+"</td></tr></table>");



        }

        else if (!QString::compare(MyData::orpSn,type) && this->snType == MyData::orpSn) {        //orp
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>orp(mV)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::orpTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::orpOrp)+"</td></tr></table>");

           }

        else if (!QString::compare(MyData::chlaSn,type) && this->snType == MyData::chlaSn) {        //叶绿素a
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>叶绿素(μg/L)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::chlaTemp)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::chlaChla)+"</td></tr></table>");

        }
        else if (!QString::compare(MyData::bgaSn,type) && this->snType == MyData::bgaSn) {        //蓝绿藻
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='120'>蓝绿藻(cells/mL)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::bgaTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::bgaBga)+"</td></tr></table>");
           }
        else if (!QString::compare(MyData::oilSn,type) && this->snType == MyData::oilSn) {        //水中油
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='120'>水中油(mg/L)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::oilTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::oilOil)+"</td></tr></table>");
        }
        else if (!QString::compare(MyData::oiwSn,type) && this->snType == MyData::oilSn) {        //水中油
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='210'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='100'>水中油(ppm)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='210'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::oilTemp)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::oilOil)+"</td></tr></table>");
        }

        else if (!QString::compare("38",type) && this->snType == "38") {        //多合一
            if(first){
                ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                    "<td align='left' width='200'>测量时间</td>"
                                                    "<td align='left' width='100'>温度(℃)</td>"
                                                    "<td align='left' width='120'>溶解氧(mg/L)</td>"
                                                    "<td align='left' width='100'>pH(pH)</td>"
                                                    "<td align='left' width='140'>蓝绿藻(cells/cm)</td>"
                                                    "<td align='left' width='100'>ORP(mV)</td>"
                                                    "<td align='left' width='120'>叶绿素(μg/L)</td>"
                                                    "<td align='left' width='120'>电导率(μS/cm)</td>"
                                                    "<td align='left' width='100'>浊度(NTU)</td></tr></table>");
                first = false;
            }
            ui->textEdit_setTantou_data->append("<table cellpadding='0' cellspacing='5'><tr>"
                                                "<td align='left' width='200'>"+current_date+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::oilTemp)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::sondeLdo)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::sondePH)+"</td>"
                                                "<td align='left' width='140'>"+QString("%1").arg(MyData::sondeBga)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::sondeOrp)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::sondeChla)+"</td>"
                                                "<td align='left' width='120'>"+QString("%1").arg(MyData::sondeCond)+"</td>"
                                                "<td align='left' width='100'>"+QString("%1").arg(MyData::sondeTurb)+"</td></tr></table>");
        }
        //未实现的探头
        else if (!QString::compare(MyData::nhSn,type) && this->snType == "") {        //

        }
        else if (!QString::compare(MyData::nhSn,type) && this->snType == "") {        //

        }
    }



    /**将数据显示到数据总览页面***********************************************/
    if(!QString::compare(MyData::codSn,type)){   //cod
        ui->label_cod_cod->setNum(MyData::codCod);
        label_cod_temp_data->setNum(MyData::codTemp);
        label_cod_toc_data->setNum(MyData::codToc);
        label_cod_turb_data->setNum(MyData::codTurb);
        ui->textEdit_all->append(current_date+"<font color='green'> COD探头：Cod->：</font>"+QString::number(MyData::codCod)+
                                 "<font color='green'>    Toc->：</font>"+QString::number(MyData::codToc)+
                                 "<font color='green'>    Turb->：</font>"+QString::number(MyData::codTurb)+
                                 "<font color='green'>    温度->：</font>"+QString::number(MyData::codTemp));
    }
    else if (!QString::compare(MyData::nhSn,type)) {        //氨氮
        if(MyData::nhOrp <= -1000){
            label_nh_orp_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            label_nh_orp_data->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        if(MyData::nhNh4Plus <= -1000){
            label_nh_nh4_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            label_nh_nh4_data->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        label_nh_orp_data->setNum(MyData::nhOrp);
        label_nh_ph_data->setNum(MyData::nhPH);
        label_nh_nh4_data->setNum(MyData::nhNh4Plus);
        label_nh_nh4_n_data->setNum(MyData::nhNh4_N);
        label_nh_temp_data->setNum(MyData::nhTemp);
        ui->textEdit_all->append(current_date+"<font color='green'> 氨氮探头：orp>：</font>"+QString::number(MyData::nhOrp)+
                                 "<font color='green'>   ph->：</font>"+QString::number(MyData::nhPH)+
                                 "<font color='green'>   NH4+->：</font>"+QString::number(MyData::nhNh4Plus)+
                                 "<font color='green'>   NH4_N->：</font>"+QString::number(MyData::nhNh4_N)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::nhTemp));
    }
    //纳宏氨氮探头
    else if (!QString::compare(MyData::nhNaSn,type)) {        //氨氮
        label_nh_ph_data->setNum(MyData::nhPH);
        label_nh_nh4_n_data->setNum(MyData::nhNh4_N);
        label_nh_temp_data->setNum(MyData::nhTemp);
        ui->textEdit_all->append(current_date+"<font color='green'> 氨氮探头：ph->：</font>"+QString::number(MyData::nhPH)+
                                 "<font color='green'>   NH4_N->：</font>"+QString::number(MyData::nhNh4_N)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::nhTemp));
    }




    else if (!QString::compare(MyData::ldoSn,type)) {        //溶解氧
        label_oxygen_do_data->setNum(MyData::ldoLdo);
        label_oxygen_temp_data->setNum(MyData::ldoTemp);
        label_oxygen_saturation_data->setNum(MyData::ldoSaturation);
        ui->textEdit_all->append(current_date+"<font color='green'> 溶解氧探头：溶解氧->：</font>"+QString::number(MyData::ldoLdo)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::ldoTemp)+
                                 "<font color='green'>   饱和度->：</font>"+QString::number(MyData::ldoSaturation));
    }
    else if (!QString::compare(MyData::turbSn,type)) {        //浊度
        label_turb_temp_data->setNum(MyData::turbTemp);
        label_turb_turb_data->setNum(MyData::turbTurb);
        ui->textEdit_all->append(current_date+"<font color='green'> 浊度探头：浊度->：</font>"+QString::number(MyData::turbTurb)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::turbTemp));
    }
    else if (!QString::compare(MyData::condSn,type)) {        //电导率
        //数值位数判断，以不同大小显示
        if(MyData::condCond >= 100000){
            label_cond_cond_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
            MyData::condCond = QString::number(MyData::condCond,'f',0).toFloat();
        }
        else if(MyData::condCond >= 10000){
            label_cond_cond_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            label_cond_cond_data->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        label_cond_cond_data->setNum(QString::number(MyData::condCond,'f',0).toFloat());
        label_cond_temp_data->setNum(MyData::condTemp);
        ui->textEdit_all->append(current_date+"<font color='green'> 电导率探头：电导率->：</font>"+QString::number(MyData::condCond)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::condTemp));
    }
    else if (!QString::compare(MyData::phSn,type)) {        //ph
        if(MyData::phOrp <= -1000){
            label_ph_orp_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        label_ph_ph_data->setNum(MyData::phPH);
        label_ph_temp_data->setNum(MyData::phTemp);
        label_ph_orp_data->setNum(MyData::phOrp);
        ui->textEdit_all->append(current_date+"<font color='green'> pH探头：ph->：</font>"+QString::number(MyData::phPH,'f',3)+
                                 "<font color='green'>   orp->：</font>"+QString::number(MyData::phOrp)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::phTemp));
    }
    else if (!QString::compare(MyData::orpSn,type)) {        //orp
        if(MyData::orpOrp <= -1000){
            label_orp_orp_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            label_orp_orp_data->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        label_orp_orp_data->setNum(MyData::orpOrp);
        label_orp_temp_data->setNum(MyData::orpTemp);
        ui->textEdit_all->append(current_date+"<font color='green'> orp探头：orp->：</font>"+QString::number(MyData::orpOrp)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::orpTemp));
    }
    else if (!QString::compare(MyData::chlaSn,type)) {        //叶绿素a
        label_chla_chla_data->setNum(MyData::chlaChla);
        label_chla_temp_data->setNum(MyData::chlaTemp);
        ui->textEdit_all->append(current_date+"<font color='green'> 叶绿素a探头：叶绿素a->：</font>"+QString::number(MyData::chlaChla)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::chlaTemp));
    }
    else if (!QString::compare(MyData::bgaSn,type)) {        //蓝绿藻
        //数值位数判断
        if(MyData::bgaBga >= 1000000){
            label_bga_bga_data->setStyleSheet("QLabel{font: 35px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else if (MyData::bgaBga >= 100000) {
            label_bga_bga_data->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            label_bga_bga_data->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        label_bga_bga_data->setText(measure_value_swith(MyData::bgaBga));
        label_bga_temp_data->setNum(MyData::bgaTemp);
        ui->textEdit_all->append(current_date+"<font color='green'> 蓝绿藻探头：蓝绿藻->：</font>"+QString::number(MyData::bgaBga)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::bgaTemp));
    }
    else if (!QString::compare(MyData::oilSn,type)) {        //水中油        
        label_oil_oil_data->setNum(MyData::oilOil);
        label_oil_temp_data->setNum(MyData::oilTemp);
        label_oil_oil_unit->setText("mg/L");
        ui->textEdit_all->append(current_date+"<font color='green'> 水中油探头：水中油->：</font>"+QString::number(MyData::oilOil)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::oilTemp));
    }
    else if (!QString::compare(MyData::oiwSn,type)) {        //水中油
        label_oil_oil_data->setNum(MyData::oilOil);
        label_oil_temp_data->setNum(MyData::oilTemp);
        label_oil_oil_unit->setText("ppm");
        ui->textEdit_all->append(current_date+"<font color='green'> 水中油探头：水中油->：</font>"+QString::number(MyData::oilOil)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::oilTemp));
    }
    else if (!QString::compare(MyData::sondeSn,type)) {        //多合一
        ui->label_sonde_ldo->setNum(MyData::sondeLdo);
        ui->label_sonde_ph->setNum(MyData::sondePH);
        //数值位数判断
        if(MyData::sondeBga >= 1000000){
            ui->label_sonde_bga->setStyleSheet("QLabel{font: 35px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else if (MyData::sondeBga >= 100000) {
            ui->label_sonde_bga->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            ui->label_sonde_bga->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }

        if(MyData::sondeCond >= 100000){
            ui->label_sonde_cond->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
            MyData::sondeCond = QString::number(MyData::condCond,'f',0).toFloat();
        }
        else if(MyData::sondeCond >= 10000){
            ui->label_sonde_cond->setStyleSheet("QLabel{font: 40px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        else {
            ui->label_sonde_cond->setStyleSheet("QLabel{font: 45px 'Arial';background-color:rgb(190, 190, 190);color:blank;margin-left:25px;margin-right:25px;}");
        }
        ui->label_sonde_bga->setText(QString::number(MyData::sondeBga,'f',0));
        ui->label_sonde_orp->setNum(MyData::sondeOrp);
        ui->label_sonde_chla->setNum(MyData::sondeChla);
        ui->label_sonde_cond->setNum(QString::number(MyData::sondeCond,'f',0).toFloat());
        ui->label_sonde_temp->setNum(MyData::sondeTemp);
        ui->label_sonde_turb->setNum(MyData::sondeTurb);
        ui->textEdit_all->append(current_date+"<font color='green'> 多合一探头：溶解氧->：</font>"+QString::number(MyData::sondeLdo)+
                                 "<font color='green'>   pH->：</font>"+QString::number(MyData::sondePH)+
                                 "<font color='green'>   溶解氧->：</font>"+QString::number(MyData::sondeBga)+
                                 "<font color='green'>   orp->：</font>"+QString::number(MyData::sondeOrp)+
                                 "<font color='green'>   叶绿素a->：</font>"+QString::number(MyData::sondeChla)+
                                 "<font color='green'>   电导率->：</font>"+QString::number(MyData::sondeCond)+
                                 "<font color='green'>   浊度->：</font>"+QString::number(MyData::sondeTurb)+
                                 "<font color='green'>   温度->：</font>"+QString::number(MyData::sondeTemp));
    }
    else if (!QString::compare(MyData::nhSn,type)) {        //

    }
    else if (!QString::compare(MyData::nhSn,type)) {        //

    }

}


/**
 * @brief MainWindow::on_pushButton_dan_search_clicked
 * 单探头搜索
 */
void MainWindow::on_pushButton_dan_search_clicked()
{
    QString style = "QPushButton{background-color:rgb(225, 225, 225);"
                    "border: 2px groove gray;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    ui->pushButton_cod->setStyleSheet(style);
    ui->pushButton_nh->setStyleSheet(style);
    ui->pushButton_oxygen->setStyleSheet(style);
    ui->pushButton_turb->setStyleSheet(style);
    ui->pushButton_cond->setStyleSheet(style);
    ui->pushButton_ph->setStyleSheet(style);
    ui->pushButton_orp->setStyleSheet(style);
    ui->pushButton_chla->setStyleSheet(style);
    ui->pushButton_bga->setStyleSheet(style);
    ui->pushButton_oil->setStyleSheet(style);
    ui->pushButton_sonde->setStyleSheet(style);
    ui->pushButton_cod->setStyleSheet(style);
    ui->pushButton_cod->setStyleSheet(style);
    ui->pushButton_cod->setStyleSheet(style);
    if(ui->pushButton_port_conn->text() == "端口连接"){
        ui->textEdit_all->append("端口未打开");
        return;
    }
    ui->pushButton_dan_search->setEnabled(true);
    ui->textEdit_all->append("开始搜索探头，请等待搜索结束");
    ui->pushButton_dault_search->setEnabled(false);
    ui->pushButton_dan_search->setEnabled(false);
    ui->pushButton_start_read->setEnabled(false);
    emit singleSearch();
}

/**
 * @brief MainWindow::on_pushButton_18_clicked
 * 清空所有探头数据
 */
void MainWindow::on_pushButton_18_clicked()
{
    ui->textEdit_all->clear();
}

/**
 * @brief MainWindow::on_pushButton_dault_search_clicked
 * 默认地址搜索
 */
void MainWindow::on_pushButton_dault_search_clicked()
{
    QString style = "QPushButton{background-color:rgb(225, 225, 225);"
                    "border: 2px groove gray;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    ui->pushButton_cod->setStyleSheet(style);
    ui->pushButton_nh->setStyleSheet(style);
    ui->pushButton_oxygen->setStyleSheet(style);
    ui->pushButton_turb->setStyleSheet(style);
    ui->pushButton_cond->setStyleSheet(style);
    ui->pushButton_ph->setStyleSheet(style);
    ui->pushButton_orp->setStyleSheet(style);
    ui->pushButton_chla->setStyleSheet(style);
    ui->pushButton_bga->setStyleSheet(style);
    ui->pushButton_oil->setStyleSheet(style);
    ui->pushButton_sonde->setStyleSheet(style);
    ui->pushButton_cod->setStyleSheet(style);
    ui->pushButton_cod->setStyleSheet(style);
    ui->pushButton_cod->setStyleSheet(style);

    qDebug()<<"默认搜索";
    if(!QString::compare(ui->pushButton_port_conn->text(),"端口连接")){
        ui->textEdit_all->append("端口未打开");
        return;
    }
    ui->textEdit_all->append("开始搜索探头，请等待搜索结束");
    ui->pushButton_dault_search->setEnabled(false);
    ui->pushButton_start_read->setEnabled(false);
    ui->pushButton_dan_search->setEnabled(false);
    emit defaultSearch();
}

int i = 0;
void MainWindow::on_pushButton_start_read_clicked()
{
    qDebug()<<"自动运行变量"<<autoRead;
    qDebug()<<"调用开始读取数据函数";
    if(!QString::compare(ui->pushButton_port_conn->text(),"端口连接")){
        ui->textEdit_all->append("端口未打开");
        return;
    }

    if(ui->pushButton_start_read->text() == "开始读取数据"){
        qDebug()<<"调用开始读取数据函数1";
        ui->pushButton_start_read->setText("停止读取数据");
        MyData::isRun = true;
        if(i == 0){
            autoRead = false;
            emit run(true);
            i++;
        }

    }
    else {       
        ui->pushButton_start_read->setText("开始读取数据");
        ui->pushButton_read_data->setText(" 启动测量 ");
        MyData::isRun = false;
        //emit run(false);
    }
}

void MainWindow::on_comboBox_cal_type_currentIndexChanged(int index)
{
    if(index == 0){
        ui->label_stand2->setHidden(true);
        ui->lineEdit_stand2->setHidden(true);
        ui->pushButton_stand2->setHidden(true);
        ui->label_test2_value->setHidden(true);
    }
    else {
        ui->label_stand2->setHidden(false);
        ui->lineEdit_stand2->setHidden(false);
        ui->pushButton_stand2->setHidden(false);
        ui->label_test2_value->setHidden(false);
    }
}

void MainWindow::on_pushButton_clear_clicked()
{
    if(snType == MyData::oilSn){
        if(MyData::oilType == "oil"){
            if(ui->radioButton_hand->isChecked()){
                emit clear(ui->label_real_id->text().toInt() & 0xff,snType);
            }
        }
        else if (MyData::oilType == "oiw") {
            emit clear(ui->label_real_id->text().toInt() & 0xff,snType);
        }
    }
    else {
        emit clear(ui->label_real_id->text().toInt() & 0xff,snType);
    }
}


/**
 * @brief MainWindow::on_pushButton_return_default_clicked 恢复默认校准参数
 */
void MainWindow::on_pushButton_return_default_clicked()
{
    uint8_t real_id = ui->label_real_id->text().toInt() &0xff;
    QString param = ui->comboBox_cal_param->currentText();
    if(snType == MyData::codSn){
        if(param == "Turb"){
            emit setCalParameter(real_id,1,0,0x34,0x04);
            return;
        }
    }
    else if(snType == MyData::nhSn){
        if(param == "NH4_N"){
            emit setCalParameter(real_id,1,0,0x36,0x04);
            return;
        }
        else if (param == "pH") {
            emit setCalParameter(real_id,1,0,0x11,0x04);
            return;
        }
        else if (param == "NH4+") {
            emit setCalParameter(real_id,1,0,0x35,0x04);
            return;
        }
    }
    else if(snType == MyData::phSn || snType == MyData::orpSn){
        if(param == "pH"){
            emit setCalParameter(real_id,1,0,0x11,0x04);
            return;
        }
        else if (param == "orp") {
           emit setCalParameter(real_id,1,0,0x34,0x04);
            return;
        }
    }
    else if (snType == MyData::oilSn) {
        if(MyData::oilType == "oiw"){
            emit setCalParameter(real_id,1,0,0x11,0x04);
            return;
        }
    }
    else if (snType == MyData::sondeSn) {
        MyData::calParam = param;
        MyData::isSondeCal = true;
        if(param == "ldo"){
            emit setCalParameter(real_id,1,0,0x01,0x04);
        }
        else if (param == "turb") {
            emit setCalParameter(real_id,1,0,0x02,0x04);
        }
        if(param == "cond"){
            emit setCalParameter(real_id,1,0,0x03,0x04);
        }
        else if (param == "ph") {
            emit setCalParameter(real_id,1,0,0x04,0x04);
        }
        if(param == "orp"){
            emit setCalParameter(real_id,1,0,0x05,0x04);
        }
        else if (param == "chla") {
            emit setCalParameter(real_id,1,0,0x06,0x04);
        }
        else if (param == "bga") {
            emit setCalParameter(real_id,1,0,0x07,0x04);
        }
        return;
    }

    emit setCalParameter(real_id,1,0,0x11,0x04);
}


void MainWindow::sleep(int msec)    //自定义Qt延时函数,单位毫秒
{
    QTime t;
    t.start();
    while(t.elapsed()<msec){
        QCoreApplication::processEvents();
    }
}

/**
 * @brief MainWindow::on_pushButton_5_clicked
 * 设置校准参数
 */
void MainWindow::on_pushButton_5_clicked()
{
    float calK;
    uint8_t real_id = ui->label_real_id->text().toInt() & 0xff;
    float standValue1 = ui->lineEdit_stand1->text().toFloat();   //标液值
    float testValue1 = ui->label_test1_value->text().toFloat();  //实际测量值
    QString param = ui->comboBox_cal_param->currentText();
    if(ui->comboBox_cal_type->currentText() == "一点校准"){
        if(abs(standValue1 - 0) > 0.0000001 && abs(testValue1 -0) > 0.0000001){
            calK = standValue1*MyData::currentK/testValue1;        //计算校准后要保存的K值
            if(snType == MyData::condSn){
                calK = (standValue1*MyData::currentK/1000)/(testValue1/1000);
            }


            else if(snType == MyData::codSn){    //Cod探头有两个参数
                if (param == "Turb") {
                    emit setCalParameter(real_id,calK,0,0x34,0x04);
                    return;
                }
                else if (param == "Cod") {
                    emit setCalParameter(real_id,calK,0,0x11,0x04);
                    return;
                }
            }


            else if (snType == MyData::nhSn) {          //氨氮探头有三个需要校准的参数
                if(MyData::nhType == "nhy"){
                    if(param == "NH4_N"){
                        emit setCalParameter(real_id,calK,0,0x36,0x04);
                        return;
                    }
                    else if (param == "pH") {
                        emit setCalParameter(real_id,calK,0,0x11,0x04);
                        return;
                    }
                    else if (param == "NH4+") {
                        emit setCalParameter(real_id,calK,0,0x35,0x04);
                        return;
                    }
                }
            }

            else if(snType == MyData::phSn || snType == MyData::orpSn){
                if(param == "pH"){
                    emit setCalParameter(real_id,calK,0,0x11,0x04);
                    return;
                }
                else if (param == "orp") {
                   emit setCalParameter(real_id,calK,0,0x34,0x04);
                   return;
                }
            }
            if(snType == MyData::oilSn){
                if (MyData::oilType == "oiw") {
                    emit setCalParameter(real_id,calK,0,0x11,0x04);
                    return;
                }

            }
            else if(snType == MyData::sondeSn){
                MyData::isSondeCal = true;
                MyData::calParam = param;
                if(param == "ldo"){
                    emit setCalParameter(real_id,calK,0,0x01,0x04);
                    return;
                }
                else if (param == "turb") {
                   emit setCalParameter(real_id,calK,0,0x02,0x04);
                   return;
                }
                else if (param == "cond") {
                   calK = (standValue1*MyData::currentK/1000)/(testValue1/1000);
                   emit setCalParameter(real_id,calK,0,0x03,0x04);
                   return;
                }
                else if (param == "ph") {
                   emit setCalParameter(real_id,calK,0,0x04,0x04);
                   return;
                }
                else if (param == "orp") {
                   emit setCalParameter(real_id,calK,0,0x05,0x04);
                   return;
                }
                else if (param == "chla") {
                   emit setCalParameter(real_id,calK,0,0x06,0x04);
                   return;
                }
                else if (param == "bga") {
                   emit setCalParameter(real_id,calK,0,0x07,0x04);
                   return;
                }
            }

            emit setCalParameter(real_id,calK,0,0x11,0x04);
        }
    }
    else if (ui->comboBox_cal_type->currentText() == "两点校准") {

    }
}

void MainWindow::set_not_oil_show(){
    ui->pushButton_stand1->setText("测试");
    ui->pushButton_stand2->setText("测试");
    ui->pushButton_5->setHidden(false);
    ui->pushButton_clear_num->setEnabled(false);
    ui->label_clear_num->setEnabled(false);
    ui->lineEdit_clear_num->setEnabled(false);
    ui->radioButton_auto->setEnabled(false);
    ui->radioButton_hand->setEnabled(false);
}

/**
 * @brief MainWindow::on_radioButton_auto_clicked
 *  设置水中油探头自动清扫模式
 */
void MainWindow::on_radioButton_auto_clicked()
{
    uint8_t id = ui->label_real_id->text().toInt() & 0xff;
    openNaHongBrush(id ,0);
}

void MainWindow::on_radioButton_hand_clicked()
{
    uint8_t id = ui->label_real_id->text().toInt() & 0xff;
    openNaHongBrush(id ,1);
}

/**
 * @brief MainWindow::on_pushButton_clear_num_clicked
 * 设置水中油探头自动清扫次数
 */
void MainWindow::on_pushButton_clear_num_clicked()
{
    uint8_t num = ui->lineEdit_clear_num->text().toInt() & 0xff;
    uint8_t id = ui->label_real_id->text().toInt() & 0xff;
    emit setOilClearNum(id,num);
}


/**
 * @brief MainWindow::on_pushButton_sql_conn_clicked
 * 连接数据库
 */
void MainWindow::on_pushButton_sql_conn_clicked()
{

    if(ui->label_sql_conn->text().toStdString() == "断开"){
        QString serverAddr = ui->LineEdit_server_addr->text();
        QString userName = ui->LineEdit_user_name->text();
        QString pwd = ui->LineEdit_pwd->text();
        int sqlTime = ui->lineEdit_sql_time->text().toInt();

        if(ui->pushButton_port_conn->text() == "关闭连接"){          
            ui->pushButton_sql_conn->setEnabled(false);
            ui->textEdit_sql->append("<font color = 'green'>正在连接数据库，请等待</font>");
            emit sqlConn(serverAddr,userName,pwd,true,sqlTime);
        }
        else {
            ui->textEdit_sql->append("<font color = 'red'>端口未打开，请先打开端口</font>");
        }
    }
}

/**
 * @brief MainWindow::on_pushButton_sql_close_clicked
 * 断开数据库
 */
void MainWindow::on_pushButton_sql_close_clicked()
{
    if(ui->label_sql_conn->text() == "已连接"){
        QString serverAddr = ui->LineEdit_server_addr->text();
        QString userName = ui->LineEdit_user_name->text();
        QString pwd = ui->LineEdit_pwd->text();
        float time_delay = ui->lineEdit_sql_time->text().toFloat();


        if(ui->pushButton_port_conn->text() == "关闭连接"){            
            ui->textEdit_sql->append("<font color = 'green'>正在断开数据库，请等待</font>");
            ui->pushButton_sql_close->setEnabled(false);
            emit sqlConn(serverAddr,userName,pwd,false,time_delay);
        }
        else {
            ui->textEdit_sql->append("<font color = 'red'>端口未打开，请先打开端口</font>");
        }
    }
}

void MainWindow::on_pushButton_sonde_clicked()
{
    ui->label_current_tantou->setText("多合一");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("11");
    query.prepare("select slave_id from probeInfo where sn = '38'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"sonde");
    }
    snType = "38";
    first = true;

    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("ldo");
    ui->comboBox_cal_param->addItem("turb");
    ui->comboBox_cal_param->addItem("cond");
    ui->comboBox_cal_param->addItem("ph");
    ui->comboBox_cal_param->addItem("orp");
    ui->comboBox_cal_param->addItem("chla");
    ui->comboBox_cal_param->addItem("bga");

    set_not_oil_show();
    ui->groupBox_brush->setEnabled(true);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_sonde,color);
}

void MainWindow::on_pushButton_cond_clicked()
{
    ui->label_current_tantou->setText("电导率");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("5");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Cond");
    ui->groupBox_brush->setEnabled(false);
    query.prepare("select slave_id from probeInfo where sn = '09'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"cond");
    }
    snType = MyData::condSn;
    first = true;
    set_not_oil_show();
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_cond,color);
}

void MainWindow::on_pushButton_bga_clicked()
{
    ui->label_current_tantou->setText("蓝绿藻");
    //ui->tabWidget->setCurrentIndex(2);
    ui->label_addr_default->setText("9");
    ui->comboBox_cal_param->clear();
    ui->comboBox_cal_param->addItem("Bga");
    query.prepare("select slave_id from probeInfo where sn = '62'");
    query.exec();
    while (query.next()) {
        if(!ui->checkBox_repeat->isChecked())
        ui->label_real_id->setNum(query.value(0).toInt());
        emit getSNSignal(query.value(0).toInt(),"bga");
    }
    snType = MyData::bgaSn;
    first = true;
    set_not_oil_show();
    ui->groupBox_brush->setEnabled(true);
    if(ui->checkBox->isChecked()){
        ui->tabWidget->setCurrentIndex(2);
    }
    else{
        on_checkBox_stateChanged(0);
    }
    selectButton(ui->pushButton_bga,color);
}


/**
 * @brief MainWindow::on_checkBox_stateChanged  用于选择查看全部数据还是单个探头数据
 * @param arg1
 */
void MainWindow::on_checkBox_stateChanged(int arg1)
{
    delete ui->scrollAreaWidgetContents->layout();  //删除原有界面布局，然后进行新的布局
    int row = 0;
    int col = 0;
    ui->groupBox_sonde_ph->setHidden(true);
    ui->groupBox_sonde_bga->setHidden(true);
    ui->groupBox_sonde_ldo->setHidden(true);
    ui->groupBox_sonde_orp->setHidden(true);
    ui->groupBox_sonde_chla->setHidden(true);
    ui->groupBox_sonde_cond->setHidden(true);
    ui->groupBox_sonde_turb->setHidden(true);
    ui->groupBox_sonde_temp->setHidden(true);

    ui->groupBox_cod_cod->setHidden(true);
    box_cod_temp->setHidden(true);
    box_cod_toc->setHidden(true);
    box_cod_turb->setHidden(true);

    box_nh_ph->setHidden(true);
    box_nh_orp->setHidden(true);
    box_nh_nh4->setHidden(true);
    box_nh_k->setHidden(true);
    box_nh_nh4_n->setHidden(true);
    box_nh_temp->setHidden(true);

    box_oxygen_do->setHidden(true);
    box_oxygen_temp->setHidden(true);
    box_oxygen_saturation->setHidden(true);

    box_turb_turb->setHidden(true);
    box_turb_temp->setHidden(true);

    box_cond_cond->setHidden(true);
    box_cond_temp->setHidden(true);

    box_ph_ph->setHidden(true);
    box_ph_orp->setHidden(true);
    box_ph_temp->setHidden(true);

    box_orp_orp->setHidden(true);
    box_orp_temp->setHidden(true);

    box_chla_chla->setHidden(true);
    box_chla_temp->setHidden(true);

    box_bga_bga->setHidden(true);
    box_bga_temp->setHidden(true);

    box_oil_oil->setHidden(true);
    box_oil_temp->setHidden(true);

    QGridLayout *layout = new QGridLayout ();
    if(ui->checkBox->isChecked()){  //被选中显示全部探头数据
        QSqlQuery sqlQuery(DBUtil::getSqLite());
        sqlQuery.prepare("select sn from probeInfo where status = ?");
        sqlQuery.addBindValue("online");
        sqlQuery.exec();
        QString *snTypes = new QString[14];
        int count = 0;
        while(sqlQuery.next()){
            snTypes[count++] = sqlQuery.value(0).toString();
            qDebug()<<"当前在线探头："<<sqlQuery.value(0).toString();
        }

        for (uint8_t i = 0; i < count; i++) {
            if(!QString::compare(snTypes[i],MyData::codSn)){
                layout->addWidget(ui->groupBox_cod_cod,row/3,col%3);
                ui->groupBox_cod_cod->setHidden(false);
                row++;
                col++;
                layout->addWidget(box_cod_temp,row/3,col%3);
                box_cod_temp->setHidden(false);
                row++;
                col++;
                layout->addWidget(box_cod_toc,row/3,col%3);
                box_cod_toc->setHidden(false);
                row++;
                col++;
                layout->addWidget(box_cod_turb,row/3,col%3);
                box_cod_turb->setHidden(false);
                row++;
                col++;
            }
            else if (!QString::compare(MyData::nhSn,snTypes[i])) {
                box_nh_ph->setHidden(false);
                layout->addWidget(box_nh_ph,row/3,col%3);
                row++;
                col++;
                box_nh_orp->setHidden(false);
                layout->addWidget(box_nh_orp,row/3,col%3);
                row++;
                col++;
                box_nh_nh4->setHidden(false);
                layout->addWidget(box_nh_nh4,row/3,col%3);
                row++;
                col++;
                box_nh_nh4_n->setHidden(false);
                layout->addWidget(box_nh_nh4_n,row/3,col%3);
                row++;
                col++;
                box_nh_temp->setHidden(false);
                layout->addWidget(box_nh_temp,row/3,col%3);
                row++;
                col++;
            }
            else if (!QString::compare(MyData::nhNaSn,snTypes[i])) {
                box_nh_ph->setHidden(false);
                layout->addWidget(box_nh_ph,row/3,col%3);
                row++;
                col++;
                box_nh_nh4_n->setHidden(false);
                layout->addWidget(box_nh_nh4_n,row/3,col%3);
                row++;
                col++;
                box_nh_temp->setHidden(false);
                layout->addWidget(box_nh_temp,row/3,col%3);
                row++;
                col++;
            }
            else if (!QString::compare(MyData::ldoSn,snTypes[i])) {
                box_oxygen_do->setHidden(false);
                layout->addWidget(box_oxygen_do,row/3,col%3);
                row++;
                col++;
                box_oxygen_temp->setHidden(false);
                layout->addWidget(box_oxygen_temp,row/3,col%3);
                row++;
                col++;
                box_oxygen_saturation->setHidden(false);
                layout->addWidget(box_oxygen_saturation,row/3,col%3);
                row++;
                col++;
            }
            else if (!QString::compare(MyData::turbSn,snTypes[i])) {
                box_turb_turb->setHidden(false);
                layout->addWidget(box_turb_turb,row/3,col%3);
                row++;
                col++;
                box_turb_temp->setHidden(false);
                layout->addWidget(box_turb_temp,row/3,col%3);
                row++;
                col++;
            }
            else if (!QString::compare(MyData::condSn,snTypes[i])){
                box_cond_cond->setHidden(false);
                layout->addWidget(box_cond_cond,row/3,col%3);
                row++;
                col++;
                box_cond_temp->setHidden(false);
                layout->addWidget(box_cond_temp,row/3,col%3);
                row++;
                col++;
            }

            else if (!QString::compare(MyData::phSn,snTypes[i])){
                box_ph_ph->setHidden(false);
                layout->addWidget(box_ph_ph,row/3,col%3);
                row++;
                col++;
                box_ph_orp->setHidden(false);
                layout->addWidget(box_ph_orp,row/3,col%3);
                row++;
                col++;
                box_ph_temp->setHidden(false);
                layout->addWidget(box_ph_temp,row/3,col%3);
                row++;
                col++;
            }

            else if (!QString::compare(MyData::orpSn,snTypes[i])){
                box_orp_orp->setHidden(false);
                layout->addWidget(box_orp_orp,row/3,col%3);
                row++;
                col++;
                box_orp_temp->setHidden(false);
                layout->addWidget(box_orp_temp,row/3,col%3);
                row++;
                col++;
            }

            else if(!QString::compare(MyData::chlaSn,snTypes[i])){
                box_chla_chla->setHidden(false);
                layout->addWidget(box_chla_chla,row/3,col%3);
                row++;
                col++;
                box_chla_temp->setHidden(false);
                layout->addWidget(box_chla_temp,row/3,col%3);
                row++;
                col++;
            }

            else if(!QString::compare(MyData::bgaSn,snTypes[i])){
                box_bga_bga->setHidden(false);
                layout->addWidget(box_bga_bga,row/3,col%3);
                row++;
                col++;
                box_bga_temp->setHidden(false);
                layout->addWidget(box_bga_temp,row/3,col%3);
                row++;
                col++;
            }

            else if (!QString::compare(MyData::sondeSn,snTypes[i])) {
               ui->groupBox_sonde_ph->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_ph,row/3,col%3);
               row++;
               col++;
               ui->groupBox_sonde_bga->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_bga,row/3,col%3);

               row++;
               col++;
               ui->groupBox_sonde_ldo->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_ldo,row/3,col%3);

               row++;
               col++;
               ui->groupBox_sonde_orp->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_orp,row/3,col%3);

               row++;
               col++;
               ui->groupBox_sonde_chla->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_chla,row/3,col%3);

               row++;
               col++;
               ui->groupBox_sonde_cond->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_cond,row/3,col%3);

               row++;
               col++;
               ui->groupBox_sonde_turb->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_turb,row/3,col%3);

               row++;
               col++;
               ui->groupBox_sonde_temp->setHidden(false);
               layout->addWidget(ui->groupBox_sonde_temp,row/3,col%3);

               row++;
               col++;
            }
            else if (!QString::compare(MyData::oilSn,snTypes[i]) || !QString::compare(MyData::oiwSn,snTypes[i])){
                box_oil_oil->setHidden(false);
                layout->addWidget(box_oil_oil,row/3,col%3);
                row++;
                col++;
                box_oil_temp->setHidden(false);
                layout->addWidget(box_oil_temp,row/3,col%3);
                row++;
                col++;
            }
        }
        while (row < 9) {
            layout->addWidget(label1,row/3,col%3);
            row++;
            col++;
        }
        ui->scrollAreaWidgetContents->setLayout(layout);
        delete [] snTypes;
    }
    else{
        if(snType == MyData::codSn){
            layout->addWidget(ui->groupBox_cod_cod,row/3,col%3);
            ui->groupBox_cod_cod->setHidden(false);
            row++;
            col++;
            layout->addWidget(box_cod_temp,row/3,col%3);
            box_cod_temp->setHidden(false);
            row++;
            col++;
            layout->addWidget(box_cod_toc,row/3,col%3);
            box_cod_toc->setHidden(false);
            row++;
            col++;
            layout->addWidget(box_cod_turb,row/3,col%3);
            box_cod_turb->setHidden(false);
            row++;
            col++;
        }
        else if(snType == MyData::nhSn){
            box_nh_ph->setHidden(false);
            layout->addWidget(box_nh_ph,row/3,col%3);
            row++;
            col++;           
            if(MyData::nhType == "nhy"){
                box_nh_orp->setHidden(false);
                layout->addWidget(box_nh_orp,row/3,col%3);
                row++;
                col++;
                box_nh_nh4->setHidden(false);
                layout->addWidget(box_nh_nh4,row/3,col%3);
                row++;
                col++;
            }
            box_nh_nh4_n->setHidden(false);
            layout->addWidget(box_nh_nh4_n,row/3,col%3);
            row++;
            col++;
            box_nh_temp->setHidden(false);
            layout->addWidget(box_nh_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::ldoSn){
            box_oxygen_do->setHidden(false);
            layout->addWidget(box_oxygen_do,row/3,col%3);
            row++;
            col++;
            box_oxygen_temp->setHidden(false);
            layout->addWidget(box_oxygen_temp,row/3,col%3);
            row++;
            col++;
            box_oxygen_saturation->setHidden(false);
            layout->addWidget(box_oxygen_saturation,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::turbSn){
            box_turb_turb->setHidden(false);
            layout->addWidget(box_turb_turb,row/3,col%3);
            row++;
            col++;
            box_turb_temp->setHidden(false);
            layout->addWidget(box_turb_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::condSn){
            box_cond_cond->setHidden(false);
            layout->addWidget(box_cond_cond,row/3,col%3);
            row++;
            col++;
            box_cond_temp->setHidden(false);
            layout->addWidget(box_cond_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::phSn){
            box_ph_ph->setHidden(false);
            layout->addWidget(box_ph_ph,row/3,col%3);
            row++;
            col++;
            box_ph_orp->setHidden(false);
            layout->addWidget(box_ph_orp,row/3,col%3);
            row++;
            col++;
            box_ph_temp->setHidden(false);
            layout->addWidget(box_ph_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::orpSn){
            box_orp_orp->setHidden(false);
            layout->addWidget(box_orp_orp,row/3,col%3);
            row++;
            col++;
            box_orp_temp->setHidden(false);
            layout->addWidget(box_orp_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::chlaSn){
            box_chla_chla->setHidden(false);
            layout->addWidget(box_chla_chla,row/3,col%3);
            row++;
            col++;
            box_chla_temp->setHidden(false);
            layout->addWidget(box_chla_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::bgaSn){
            box_bga_bga->setHidden(false);
            layout->addWidget(box_bga_bga,row/3,col%3);
            row++;
            col++;
            box_bga_temp->setHidden(false);
            layout->addWidget(box_bga_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::oilSn){
            box_oil_oil->setHidden(false);
            layout->addWidget(box_oil_oil,row/3,col%3);
            row++;
            col++;
            box_oil_temp->setHidden(false);
            layout->addWidget(box_oil_temp,row/3,col%3);
            row++;
            col++;
       }
        else if(snType == MyData::sondeSn){
            ui->groupBox_sonde_ph->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_ph,row/3,col%3);
            row++;
            col++;
            ui->groupBox_sonde_bga->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_bga,row/3,col%3);

            row++;
            col++;
            ui->groupBox_sonde_ldo->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_ldo,row/3,col%3);

            row++;
            col++;
            ui->groupBox_sonde_orp->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_orp,row/3,col%3);

            row++;
            col++;
            ui->groupBox_sonde_chla->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_chla,row/3,col%3);

            row++;
            col++;
            ui->groupBox_sonde_cond->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_cond,row/3,col%3);

            row++;
            col++;
            ui->groupBox_sonde_turb->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_turb,row/3,col%3);

            row++;
            col++;
            ui->groupBox_sonde_temp->setHidden(false);
            layout->addWidget(ui->groupBox_sonde_temp,row/3,col%3);

            row++;
            col++;
       }
        while (row < 9) {
            layout->addWidget(label1,row/3,col%3);
            row++;
            col++;
        }
        ui->scrollAreaWidgetContents->setLayout(layout);
    }
}

/**
 * @brief MainWindow::on_pushButton_instructions_clicked 使用说明
 */
void MainWindow::on_pushButton_instructions_clicked()
{
    QProcess* process = new QProcess();
    QString notepadPath = "notepad.exe .\\readme.txt";
    process->start(notepadPath);
}


/**
 * @brief selectButton  点击按钮选中后修改探头按钮颜色
 * @param button
 */
void MainWindow:: selectButton(QPushButton * button,int color[]){
    QString selectOnline = "QPushButton{background:qlineargradient(spread:reflect,"
                           " x1:0, y1:0, x2:1, y2:0, stop:0.0738636 rgba(0, 206, 0, 255), "
                           "stop:0.0965909 rgba(52, 151, 219, 255));"
                    "border: 2px groove rgb(67, 104, 226);}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString selectDrop = "QPushButton{background:qlineargradient(spread:reflect, "
                         "x1:0, y1:0, x2:1, y2:0, stop:0.0738636 rgba(255, 0, 0, 255),"
                         " stop:0.102273 rgba(52, 151, 219, 255));"
                    "border: 2px groove rgb(67, 104, 226);}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString selectNoSearch = "QPushButton{background-color:rgb(67, 104, 226);"
                    "border: 2px groove rgb(67, 104, 226);}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString online = "QPushButton{background-color:rgb(53, 171, 93);"
                    "border: 2px groove green;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString drop = "QPushButton{background-color:red;"
                    "border: 2px groove red;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";
    QString nosearch = "QPushButton{background-color:rgb(225, 225, 225);"
                    "border: 2px groove gray;}"
                    "QPushButton:hover{background-color:white; color: black;}"
                    "QPushButton:pressed{background-color:rgb(85, 170, 255); "
                    "border-style: outset; }";

    if(color[0] == 0){
        ui->pushButton_cod->setStyleSheet(nosearch);
    }
    else if (color[0] == 1) {
        ui->pushButton_cod->setStyleSheet(online);
    }
    else if (color[0] == 2) {
        ui->pushButton_cod->setStyleSheet(drop);
    }


    if(color[1] == 0){
        ui->pushButton_nh->setStyleSheet(nosearch);
    }
    else if (color[1] == 1) {
        ui->pushButton_nh->setStyleSheet(online);
    }
    else if (color[1] == 2) {
        ui->pushButton_nh->setStyleSheet(drop);
    }


    if(color[2] == 0){
        ui->pushButton_oxygen->setStyleSheet(nosearch);
    }
    else if (color[2] == 1) {
        ui->pushButton_oxygen->setStyleSheet(online);
    }
    else if (color[2] == 2) {
        ui->pushButton_oxygen->setStyleSheet(drop);
    }



    if(color[3] == 0){
        ui->pushButton_turb->setStyleSheet(nosearch);
    }
    else if (color[3] == 1) {
        ui->pushButton_turb->setStyleSheet(online);
    }
    else if (color[3] == 2) {
        ui->pushButton_turb->setStyleSheet(drop);
    }



    if(color[4] == 0){
        ui->pushButton_cond->setStyleSheet(nosearch);
    }
    else if (color[4] == 1) {
        ui->pushButton_cond->setStyleSheet(online);
    }
    else if (color[4] == 2) {
        ui->pushButton_cond->setStyleSheet(drop);
    }



    if(color[5] == 0){
        ui->pushButton_ph->setStyleSheet(nosearch);
    }
    else if (color[5] == 1) {
        ui->pushButton_ph->setStyleSheet(online);
    }
    else if (color[5] == 2) {
        ui->pushButton_ph->setStyleSheet(drop);
    }



    if(color[6] == 0){
        ui->pushButton_orp->setStyleSheet(nosearch);
    }
    else if (color[6] == 1) {
        ui->pushButton_orp->setStyleSheet(online);
    }
    else if (color[6] == 2) {
        ui->pushButton_orp->setStyleSheet(drop);
    }



    if(color[7] == 0){
        ui->pushButton_chla->setStyleSheet(nosearch);
    }
    else if (color[7] == 1) {
        ui->pushButton_chla->setStyleSheet(online);
    }
    else if (color[7] == 2) {
        ui->pushButton_chla->setStyleSheet(drop);
    }



    if(color[8] == 0){
        ui->pushButton_bga->setStyleSheet(nosearch);
    }
    else if (color[8] == 1) {
        ui->pushButton_bga->setStyleSheet(online);
    }
    else if (color[8] == 2) {
        ui->pushButton_bga->setStyleSheet(drop);
    }



    if(color[9] == 0){
        ui->pushButton_oil->setStyleSheet(nosearch);
    }
    else if (color[9] == 1) {
        ui->pushButton_oil->setStyleSheet(online);
    }
    else if (color[9] == 2) {
        ui->pushButton_oil->setStyleSheet(drop);
    }



    if(color[10] == 0){
        ui->pushButton_sonde->setStyleSheet(nosearch);
    }
    else if (color[10] == 1) {
        ui->pushButton_sonde->setStyleSheet(online);
    }
    else if (color[10] == 2) {
        ui->pushButton_sonde->setStyleSheet(drop);
    }

    if(color[11] == 0){
        ui->pushButton_spectrum->setStyleSheet(nosearch);
    }
    else if (color[11] == 1) {
        ui->pushButton_spectrum->setStyleSheet(online);
    }
    else if (color[11] == 2) {
        ui->pushButton_spectrum->setStyleSheet(drop);
    }


    if(color[12] == 0){
        ui->pushButton__chlorine->setStyleSheet(nosearch);
    }
    else if (color[12] == 1) {
        ui->pushButton__chlorine->setStyleSheet(online);
    }
    else if (color[12] == 2) {
        ui->pushButton__chlorine->setStyleSheet(drop);
    }


    if(color[13] == 0){
        ui->pushButton_nitrate_nitrogen->setStyleSheet(nosearch);
    }
    else if (color[13] == 1) {
        ui->pushButton_nitrate_nitrogen->setStyleSheet(online);
    }
    else if (color[13] == 2) {
        ui->pushButton_nitrate_nitrogen->setStyleSheet(drop);
    }

    //更改选中探头按钮的状态
    if(button == ui->pushButton_cod){
        if(color[0] == 0){
            ui->pushButton_cod->setStyleSheet(selectNoSearch);
        }
        else if (color[0] == 1) {
            ui->pushButton_cod->setStyleSheet(selectOnline);
        }
        else if (color[0] == 2) {
            ui->pushButton_cod->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_nh) {
        if(color[1] == 0){
            ui->pushButton_nh->setStyleSheet(selectNoSearch);
        }
        else if (color[1] == 1) {
            ui->pushButton_nh->setStyleSheet(selectOnline);
        }
        else if (color[1] == 2) {
            ui->pushButton_nh->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_oxygen) {
        if(color[2] == 0){
            ui->pushButton_oxygen->setStyleSheet(selectNoSearch);
        }
        else if (color[2] == 1) {
            ui->pushButton_oxygen->setStyleSheet(selectOnline);
        }
        else if (color[2] == 2) {
            ui->pushButton_oxygen->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_turb) {
        if(color[3] == 0){
            ui->pushButton_turb->setStyleSheet(selectNoSearch);
        }
        else if (color[3] == 1) {
            ui->pushButton_turb->setStyleSheet(selectOnline);
        }
        else if (color[3] == 2) {
            ui->pushButton_turb->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_cond) {
        if(color[4] == 0){
            ui->pushButton_cond->setStyleSheet(selectNoSearch);
        }
        else if (color[4] == 1) {
            ui->pushButton_cond->setStyleSheet(selectOnline);
        }
        else if (color[4] == 2) {
            ui->pushButton_cond->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_ph) {
        if(color[5] == 0){
            ui->pushButton_ph->setStyleSheet(selectNoSearch);
        }
        else if (color[5] == 1) {
            ui->pushButton_ph->setStyleSheet(selectOnline);
        }
        else if (color[5] == 2) {
            ui->pushButton_ph->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_orp) {
        if(color[6] == 0){
            ui->pushButton_orp->setStyleSheet(selectNoSearch);
        }
        else if (color[6] == 1) {
            ui->pushButton_orp->setStyleSheet(selectOnline);
        }
        else if (color[6] == 2) {
            ui->pushButton_orp->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_chla) {
        if(color[7] == 0){
            ui->pushButton_chla->setStyleSheet(selectNoSearch);
        }
        else if (color[7] == 1) {
            ui->pushButton_chla->setStyleSheet(selectOnline);
        }
        else if (color[7] == 2) {
            ui->pushButton_chla->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_bga) {
        if(color[8] == 0){
            ui->pushButton_bga->setStyleSheet(selectNoSearch);
        }
        else if (color[8] == 1) {
            ui->pushButton_bga->setStyleSheet(selectOnline);
        }
        else if (color[8] == 2) {
            ui->pushButton_bga->setStyleSheet(selectDrop);
        }
    }

    else if (button == ui->pushButton_oil) {
        if(color[9] == 0){
            ui->pushButton_oil->setStyleSheet(selectNoSearch);
        }
        else if (color[9] == 1) {
            ui->pushButton_oil->setStyleSheet(selectOnline);
        }
        else if (color[9] == 2) {
            ui->pushButton_oil->setStyleSheet(selectDrop);
        }
    }
    else if (button == ui->pushButton_sonde) {
        if(color[10] == 0){
            ui->pushButton_sonde->setStyleSheet(selectNoSearch);
        }
        else if (color[10] == 1) {
            ui->pushButton_sonde->setStyleSheet(selectOnline);
        }
        else if (color[10] == 2) {
            ui->pushButton_sonde->setStyleSheet(selectDrop);
        }
    }
    else if (button == ui->pushButton_spectrum) {
        if(color[11] == 0){
            ui->pushButton_spectrum->setStyleSheet(selectNoSearch);
        }
        else if (color[11] == 1) {
            ui->pushButton_spectrum->setStyleSheet(selectOnline);
        }
        else if (color[11] == 2) {
            ui->pushButton_spectrum->setStyleSheet(selectDrop);
        }
    }
    else if (button == ui->pushButton__chlorine) {
        if(color[12] == 0){
            ui->pushButton__chlorine->setStyleSheet(selectNoSearch);
        }
        else if (color[12] == 1) {
            ui->pushButton__chlorine->setStyleSheet(selectOnline);
        }
        else if (color[12] == 2) {
            ui->pushButton__chlorine->setStyleSheet(selectDrop);
        }
    }
    else if (button == ui->pushButton_nitrate_nitrogen) {
        if(color[13] == 0){
            ui->pushButton_nitrate_nitrogen->setStyleSheet(selectNoSearch);
        }
        else if (color[13] == 1) {
            ui->pushButton_nitrate_nitrogen->setStyleSheet(selectOnline);
        }
        else if (color[13] == 2) {
            ui->pushButton_nitrate_nitrogen->setStyleSheet(selectDrop);
        }
    }
}

/**
 * @brief MainWindow::on_pushButton_clicked 关闭程序
 */
void MainWindow::on_pushButton_clicked()
{
    this->close();
}

/**
 * @brief MainWindow::on_pushButton_2_clicked 最小化程序
 */
void MainWindow::on_pushButton_2_clicked()
{
    this->showMinimized();
}


/**
 * @brief measure_value_swith   数值位数转换
 * @param data
 * @return
 */
QString MainWindow::measure_value_swith(float data)
{
    QString a;
    long int b;


    if(data > 1000000)
    {
        b = (long int )data;
        a.sprintf("%7d",b);
    }
    else if(data > 100000)
    {
        b = (long int )data;
        a.sprintf("%d",b);
    }
    else if(data>10000)
    {
        a.sprintf("%0.1f",data);
    }
    else if(data >1000)
    {
        a.sprintf("%0.2f",data);
    }
    else if(data < -10000)
    {
        b = (long int )data;
        a.sprintf("%d",b);
    }
    else if(data < -1000)
    {
        a.sprintf("%0.1f",data);
    }
    else if(data < -100)
    {
        a.sprintf("%0.2f",data);
    }
    else
        a.sprintf("%0.3f",data);

    //qDebug()<<"\n\n measure_value_swith:"<<a;

    return a;
}



void MainWindow::on_checkBox_autoRead_stateChanged(int arg1)
{
    qDebug()<<"自动读取状态arg1:  "<<arg1;
    if(arg1 == 0){
        query.exec("update userConfig set value = '1' where type = 'autoRead'");
    }
    else {
        query.exec("update userConfig set value = '0' where type = 'autoRead'");
    }

}
