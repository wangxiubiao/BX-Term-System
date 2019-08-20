#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mythread.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include "mydata.h"
#include "math.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /**  自定义函数 */
    void connInit();
    void mainUiInit();
    void readUserConfig();
    void saveUserConfig();

    void selectButton(QPushButton * button,int color[]);
    QString measure_value_swith(float data);

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event);


private slots:   

    void set_not_oil_show();

    void set_data_style();

    void on_pushButton_cod_clicked();

    void on_pushButton_nh_clicked();

    void on_pushButton_oxygen_clicked();

    void on_pushButton_turb_clicked();


    void on_pushButton_ph_clicked();

    void on_pushButton_orp_clicked();

    void on_pushButton_chla_clicked();


    void on_pushButton_oil_clicked();


    void on_pushButton_spectrum_clicked();

    void on_pushButton__chlorine_clicked();

    void on_pushButton_nitrate_nitrogen_clicked();

    void on_pushButton_port_conn_clicked();

    void on_pushButton_addr_update_clicked();

    void on_pushButton_port_update_clicked();

    void on_pushButton_20_clicked();

    //自定义槽函数，用于接收mythread返回的结果
    void id_ok(int addr);

    void time_ok(int time);

    void data_type(QByteArray data,QString type);

    void single_search_ok(QString snType,unsigned char id);

    void default_search_ok(QString *snTypes,unsigned char count);

    void test1_ok(float data);

    void oil_one_cal_ok(QString str);

    void com_open_ok(QString portName,bool ok);

    void cal_param_ok(float k,float b,int ok);

    void oil_clear_num_ok(unsigned char num,bool ok);

    void sql_conn_ok(bool ok,bool conn);

    void changeStatus(QString type,QString status);

    void save_data_ok();

    void get_sn_ok(QString sn);

    void ui_ok();

    void post_ok(QString msg);


    void on_pushButton_rotation_update_clicked();

    void on_comboBox_cal_type_currentIndexChanged(int index);

    void on_pushButton_stand1_clicked();

    void on_pushButton_read_data_clicked();


    void on_pushButton_dan_search_clicked();

    void on_pushButton_18_clicked();

    void on_pushButton_dault_search_clicked();

    void on_pushButton_start_read_clicked();

    void on_pushButton_clear_clicked();

    void on_pushButton_return_default_clicked();

    void sleep(int msec);

    void on_pushButton_5_clicked();

    void on_radioButton_auto_clicked();

    void on_radioButton_hand_clicked();

    void on_pushButton_clear_num_clicked();


    void on_pushButton_sql_conn_clicked();

    void on_pushButton_sql_close_clicked();

    void on_pushButton_sonde_clicked();

    void on_pushButton_cond_clicked();

    void on_pushButton_bga_clicked();

    void on_checkBox_stateChanged(int arg1);

    void on_pushButton_instructions_clicked();


    void on_pushButton_clicked();

    void on_pushButton_2_clicked();


    void on_checkBox_autoRead_stateChanged(int arg1);

signals:
    void conn_port(QString portName,qint32 baudRate);
    void setSlaveId(unsigned char id_update,unsigned char id_real,QString type);
    void setRotationPeriod(unsigned char real_id,unsigned short time,QString type);
    void close_port();
    void setCalParameter(unsigned char real_id,float k,float b,unsigned char addr,unsigned char len);
    void getChla(unsigned char real_id,QString type);
    void singleSearch();
    void defaultSearch();
    void run(bool);
    void clear(unsigned char real_id,QString type);
    void getCalParameter(unsigned char real_id,QString type,unsigned char addr);
    void getTestData(unsigned char real_id,QString type,bool one,QString param);
    void setOilCal(unsigned char real_id,float standare);
    void openNaHongBrush(unsigned char real_id ,int mode);
    void setOilClearNum(unsigned char real_id,unsigned char num);
    void sqlConn(QString addr,QString userName,QString pwd,bool conn,int sql_time);
    void getSNSignal(unsigned char real_id,QString type);

    void setNaNhCal(unsigned char real_id,float standare,QString calType);


private:
    Ui::MainWindow *ui;
    MyThread *myThread;
    QString snType = "47";
    bool threadExist = false;
    bool first = true;
    bool test1 = false; //判断是否点击了测试1按钮
    bool test2 = false; //判断是否点击了测试2按钮
    QSqlQuery query;
    int color[14] = {0};    //0：未搜索到  1：在线  2：掉线或故障  3：选中
    //int chooseOld = 0;      //保存之前被选中的探头按钮
    QPushButton *chooseOld;
    int chooseStatus = 0;      //保存之前选中的探头按钮状态

    int groupBoxWidth = 250;
    int groupBoxHeight = 192;

    bool autoRead = false;     //用于判断自动读取是否完成


    //cod
    QGroupBox *box_cod_temp;
    QGridLayout *layout_cod_temp;
    QLabel *label_cod_temp_title;
    QLabel *label_cod_temp_unit;
    QLabel *label_cod_temp_data;
    QLabel *label_cod_temp_logo;

    QGroupBox *box_cod_toc;
    QGridLayout *layout_cod_toc;
    QLabel *label_cod_toc_title;
    QLabel *label_cod_toc_unit;
    QLabel *label_cod_toc_data;
    QLabel *label_cod_toc_logo;

    QGroupBox *box_cod_turb;
    QGridLayout *layout_cod_turb;
    QLabel *label_cod_turb_title;
    QLabel *label_cod_turb_unit;
    QLabel *label_cod_turb_data;
    QLabel *label_cod_turb_logo;

    /**氨氮探头*/
    QGroupBox *box_nh_ph;
    QGridLayout *layout_nh_ph;
    QLabel *label_nh_ph_title;
    QLabel *label_nh_ph_unit;
    QLabel *label_nh_ph_data;
    QLabel *label_nh_ph_logo;

    QGroupBox *box_nh_orp;
    QGridLayout *layout_nh_orp;
    QLabel *label_nh_orp_title;
    QLabel *label_nh_orp_unit;
    QLabel *label_nh_orp_data;
    QLabel *label_nh_orp_logo;

    QGroupBox *box_nh_nh4;
    QGridLayout *layout_nh_nh4;
    QLabel *label_nh_nh4_title;
    QLabel *label_nh_nh4_unit;
    QLabel *label_nh_nh4_data;
    QLabel *label_nh_nh4_logo;

    QGroupBox *box_nh_k;
    QGridLayout *layout_nh_k;
    QLabel *label_nh_k_title;
    QLabel *label_nh_k_unit;
    QLabel *label_nh_k_data;
    QLabel *label_nh_k_logo;

    QGroupBox *box_nh_nh4_n;
    QGridLayout *layout_nh_nh4_n;
    QLabel *label_nh_nh4_n_title;
    QLabel *label_nh_nh4_n_unit;
    QLabel *label_nh_nh4_n_data;
    QLabel *label_nh_nh4_n_logo;

    QGroupBox *box_nh_temp;
    QGridLayout *layout_nh_temp;
    QLabel *label_nh_temp_title;
    QLabel *label_nh_temp_unit;
    QLabel *label_nh_temp_data;
    QLabel *label_nh_temp_logo;

    /**溶解氧*/
    QGroupBox *box_oxygen_do;
    QGridLayout *layout_oxygen_do;
    QLabel *label_oxygen_do_title;
    QLabel *label_oxygen_do_unit;
    QLabel *label_oxygen_do_data;
    QLabel *label_oxygen_do_logo;

    QGroupBox *box_oxygen_temp;
    QGridLayout *layout_oxygen_temp;
    QLabel *label_oxygen_temp_title;
    QLabel *label_oxygen_temp_unit;
    QLabel *label_oxygen_temp_data;
    QLabel *label_oxygen_temp_logo;

    QGroupBox *box_oxygen_saturation;
    QGridLayout *layout_oxygen_saturation;
    QLabel *label_oxygen_saturation_title;
    QLabel *label_oxygen_saturation_unit;
    QLabel *label_oxygen_saturation_data;
    QLabel *label_oxygen_saturation_logo;

    //浊度
    QGroupBox *box_turb_turb;
    QGridLayout *layout_turb_turb;
    QLabel *label_turb_turb_title;
    QLabel *label_turb_turb_unit;
    QLabel *label_turb_turb_data;
    QLabel *label_turb_turb_logo;

    QGroupBox *box_turb_temp;
    QGridLayout *layout_turb_temp;
    QLabel *label_turb_temp_title;
    QLabel *label_turb_temp_unit;
    QLabel *label_turb_temp_data;
    QLabel *label_turb_temp_logo;

    //电导率
    QGroupBox *box_cond_cond;
    QGridLayout *layout_cond_cond;
    QLabel *label_cond_cond_title;
    QLabel *label_cond_cond_unit;
    QLabel *label_cond_cond_data;
    QLabel *label_cond_cond_logo;

    QGroupBox *box_cond_temp;
    QGridLayout *layout_cond_temp;
    QLabel *label_cond_temp_title;
    QLabel *label_cond_temp_unit;
    QLabel *label_cond_temp_data;
    QLabel *label_cond_temp_logo;

    //ORP
    QGroupBox *box_orp_orp;
    QGridLayout *layout_orp_orp;
    QLabel *label_orp_orp_title;
    QLabel *label_orp_orp_unit;
    QLabel *label_orp_orp_data;
    QLabel *label_orp_orp_logo;

    QGroupBox *box_orp_pH;
    QGridLayout *layout_orp_pH;
    QLabel *label_orp_pH_title;
    QLabel *label_orp_pH_unit;
    QLabel *label_orp_pH_data;
    QLabel *label_orp_pH_logo;

    QGroupBox *box_orp_temp;
    QGridLayout *layout_orp_temp;
    QLabel *label_orp_temp_title;
    QLabel *label_orp_temp_unit;
    QLabel *label_orp_temp_data;
    QLabel *label_orp_temp_logo;

    //PH
    QGroupBox *box_ph_ph;
    QGridLayout *layout_ph_ph;
    QLabel *label_ph_ph_title;
    QLabel *label_ph_ph_unit;
    QLabel *label_ph_ph_data;
    QLabel *label_ph_ph_logo;

    QGroupBox *box_ph_orp;
    QGridLayout *layout_ph_orp;
    QLabel *label_ph_orp_title;
    QLabel *label_ph_orp_unit;
    QLabel *label_ph_orp_data;
    QLabel *label_ph_orp_logo;

    QGroupBox *box_ph_temp;
    QGridLayout *layout_ph_temp;
    QLabel *label_ph_temp_title;
    QLabel *label_ph_temp_unit;
    QLabel *label_ph_temp_data;
    QLabel *label_ph_temp_logo;


    //叶绿素a
    QGroupBox *box_chla_chla;
    QGridLayout *layout_chla_chla;
    QLabel *label_chla_chla_title;
    QLabel *label_chla_chla_unit;
    QLabel *label_chla_chla_data;
    QLabel *label_chla_chla_logo;

    QGroupBox *box_chla_temp;
    QGridLayout *layout_chla_temp;
    QLabel *label_chla_temp_title;
    QLabel *label_chla_temp_unit;
    QLabel *label_chla_temp_data;
    QLabel *label_chla_temp_logo;

    //蓝绿藻
    QGroupBox *box_bga_bga;
    QGridLayout *layout_bga_bga;
    QLabel *label_bga_bga_title;
    QLabel *label_bga_bga_unit;
    QLabel *label_bga_bga_data;
    QLabel *label_bga_bga_logo;

    QGroupBox *box_bga_temp;
    QGridLayout *layout_bga_temp;
    QLabel *label_bga_temp_title;
    QLabel *label_bga_temp_unit;
    QLabel *label_bga_temp_data;
    QLabel *label_bga_temp_logo;

    //水中油
    QGroupBox *box_oil_oil;
    QGridLayout *layout_oil_oil;
    QLabel *label_oil_oil_title;
    QLabel *label_oil_oil_unit;
    QLabel *label_oil_oil_data;
    QLabel *label_oil_oil_logo;

    QGroupBox *box_oil_temp;
    QGridLayout *layout_oil_temp;
    QLabel *label_oil_temp_title;
    QLabel *label_oil_temp_unit;
    QLabel *label_oil_temp_data;
    QLabel *label_oil_temp_logo;


    //空的label，用来控制布局
    QLabel *label1;

};

#endif // MAINWINDOW_H
