#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "port.h"
#include "windows.h"
#include "stdint.h"
#include "QTimer"
#include "QDateTime"
#include "mil_std_1553.h"
#include "globaldef.h"
#include "dc_power.h"



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();


    // Шаблон реализует функцию, аргументом которой является сигнал;
        // Функция ожидает сигнал в течении 200 мсек;
        // Если сигнала не дождались, то функция возвращает:    true;
        // Если сигнал в течении 200 мсек появился то:          false;
        template<typename TSignal>
        bool waitingFor(TSignal&& signal){
            bool state = false;
            QTimer *waitingForTimer = new QTimer();
            QEventLoop evloop;
            (*waitingForTimer).setInterval(400);
            (*waitingForTimer).start();
            connect(this,signal,&evloop, &QEventLoop::quit);
            connect(waitingForTimer, &QTimer::timeout, [&evloop, &state](){
                state = true;
                evloop.quit();
            });
            evloop.exec();
            (*waitingForTimer).stop();
            delete waitingForTimer;
            return state;
        }

    QColor clButtonFace = QColor(240, 240, 240);
    QColor clGreen = QColor(0, 196, 0);
    QColor clRed = QColor(255, 111, 111);
    //QString stlPushButON = "background-color: #18d6ad; border: 1px solid #959c9b; border-radius: 10px; font: 150 10pt \"MS Shell Dlg 2\";"; //"QPushButton {background-color: rgb(0,196,0); border: 1px solid black; font: bold 12px; color: white;} ";
    //QString stlPushButOFF = "background-color: rgb(240, 240, 240); border: 1px solid #959c9b; border-radius: 10px;font: 150 10pt \"MS Shell Dlg 2\";"; //"QPushButton {background-color: rgb(240, 240, 240); border: 1px solid gray; font: 12px;} ";
    QColor clGreenGLN = QColor(192, 220, 192);


    #pragma pack(push, 1)
    union uKAS_g
    {
        struct sKAS_s
        {
            unsigned int cmd;
            unsigned int addr;
            unsigned short pn_c;
            unsigned short kas;
            unsigned short pn_r;
            unsigned short crc16;
        } r;
        unsigned char g[16];
    };
    #pragma pack(pop)
    uKAS_g ukas_g;

    // структура ULA_get и ULA_set
    #pragma pack(push, 1)
    union uULA_g_s
    {
       struct sULA_g_s
       {
           unsigned char R_UPR;             // регистр управления
           unsigned char R_KOM;             // регистр выбора светодиода поджига
           unsigned char T_COR;             // регистр длительности ФК
           unsigned char T_DAC_EN;          // регистр длительности сигнала DAC_EN

           unsigned char T_FC[3];           // регистр времени выдачи сигнала поджига
           unsigned char Length_FC;         // регистр длительность ФК

           unsigned char R_SUMM_HZ[3];      // регистр сумматора синхронизации секунды
           unsigned char empty;

           unsigned char REG_HZ[3];         // регистр длительности секунды
           unsigned char D_TERMO;           // значение температуры

           unsigned short DAC_CRC;          // DAC CRC16
           unsigned short DAC_CRC_C;        // DAC CRC16 calculated
       } r;
       unsigned char g[20];                 // 10 СД
    };
    #pragma pack(pop)
    uULA_g_s uula_g_s;

    // структура ULA_set
//    #pragma pack(push, 1)
//    union uULA_s
//    {
//       struct sULA_s
//       {
//           unsigned char reg_upr;           // регистр управления
//           unsigned char reg_kom;           // регистр выбора светодиода поджига
//           unsigned char reg_Lenght_FC;     // регистр длительности ФК
//           unsigned char rsvd_00C;
//           unsigned char reg_time_FC[3];    // регистр времени выдачи сигнала поджига
//           unsigned char rsvd_01C;
//           unsigned char reg_cor;           // регистр длительности корреляции
//           unsigned char reg_DAC_EN;        // регистр длительности сигнала DAC_EN
//           unsigned char reg_SUMM_HZ[3];    // регистр сумматора синхронизации секунды
//           unsigned char rsvd_034;

//       } r;
//       unsigned char s[14];                // 7 СД
//    };
//    #pragma pack(pop)
//    uULA_s uula_s;

    // структура FC
    #pragma pack(push, 1)
    union uFC
    {
       struct sFC_s
       {
          unsigned char SVD;            // СД2 [7:0]
          unsigned char GATE;           // СД2 [15:8]
          unsigned char PWM[2];         // СД3 [15:0]
          unsigned char TPWM[2];        // СД4 [15:0]
          unsigned char fk_time[4];     // СД5 [15:0], СД6 [15:0]
       } r;
       unsigned char s[10];                // 5 СД
    };
    #pragma pack(pop)
    uFC uFC_s;

    // структура DAC1
    #pragma pack(push, 1)
    union uDAC1
    {
       struct sDAC1_s
       {
           unsigned short line1;
           unsigned short line2;
           unsigned short line3;
           unsigned short line4;
           unsigned short line5;
           unsigned short line6;
           unsigned short line7;
           unsigned short line8;
           unsigned short line9;
           unsigned short line10;
           unsigned short line11;
           unsigned short line12;
           unsigned short line13;
           unsigned short line14;
           unsigned short line15;
           unsigned short line16;

       } r;
       unsigned char s[32];
    };
    #pragma pack(pop)
    uDAC1 udac1;
    uDAC1 udac2;

    unsigned short DAC_R_20PA[2][16];




signals:

    void savesettings(QString name, int baudrate, int DataBits, int Parity, int StopBits, int FlowControl);
    void writeData(QByteArray data);
    void setBRGA1_A();
    //Обратная связь по состоянию адреса (для обрабочтчика квитанций):
    void signal_p();
    void signal_k();
    void signal_d();
    void signal_c();
    void signal_r();

    void DAC1_25PA_ready1();
    void DAC1_25PA_ready2();
    void DAC1_25PA_ready3();
    void DAC1_25PA_ready4();
    void DAC1_25PA_ready5();
    void DAC1_25PA_ready6();
    void DAC1_25PA_ready7();
    void DAC1_25PA_ready8();

    void DAC2_25PA_ready1();
    void DAC2_25PA_ready2();
    void DAC2_25PA_ready3();
    void DAC2_25PA_ready4();
    void DAC2_25PA_ready5();
    void DAC2_25PA_ready6();
    void DAC2_25PA_ready7();
    void DAC2_25PA_ready8();

    void DC_OUT1_ON();
    void DC_OFF();
    void IKPMO_sign();
    void OU_accepted();
    void KAS_ready();

private slots:
    void initTable();
    void PrintMiliSec(QString data);
    void Print(QString data);
    void PrintERR(QString data);
    void onMsgWM_BC_INT_NORM();
    unsigned short Calc_CRC16(uchar  *pBlk, unsigned int len,unsigned short *pTab16);
    void on_pBut_MKO_A1_toggled(bool checked);
    void on_pBut_MKO_B1_toggled(bool checked);
    void on_pBut_read_IKS_clicked();
    void transaction_handler(QString data);
    void on_cEnterText_returnPressed();   

    void CheckVset1(QString data);

    void CheckIset1(QString data);

    void read_EVT(int i);

    void on_pBut_COPY_clicked();

    void on_pBut_READ_clicked();

    void on_pBut_READ_clicked(unsigned int addr, ushort len);

    void on_pBut_WRITE_clicked();

    void on_pBut_WRITE_parametr_clicked(unsigned int addr, unsigned char value);

    void on_pBut_RUN_clicked();

    void on_pBut_STOP_clicked();

    void on_pBut_FC_2_clicked();

    void on_pBut_LOAD_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_6_clicked();

    void on_pBut_GET_ULA_clicked();

    void on_pBut_SET_ULA_clicked();

    void on_pBut_RESET_clicked();

    void on_pBut_GO_TO_clicked();

    void on_pBut_CORR_DIS_clicked();

    void on_pBut_CORR_EN_clicked();

    void on_pBut_UART_OFF_clicked();

    void on_pBut_UART_ON_clicked();

    void on_cBoxSort_activated(int index);

    void toTXline(WORD *data);
    void toRXline(WORD *data);

    void preparingArrForTransmit(int num);

    void read_Kvit();

    void on_pBut_read_Kvit_clicked();

    void preparingArrForTransmitWithData(ushort *pData);

    void on_open_File_IKPMO_clicked();

    void on_open_File_IKPMO_2_clicked();

    void preparingArrForTransmitWithDataWithoutCS(ushort *pData);

    void flashes1HZ();


    void on_circulating_clicked();

    void initARD();

    void on_pBut_BRGA1_A_clicked(bool checked);

    void on_pBut_BRGA1_B_clicked(bool checked);

    void on_pBut_BRGA1h_clicked(bool checked);

    void on_pBut_1Hz_A_clicked(bool checked);

    void on_pBut_1Hz_B_clicked(bool checked);

    void on_comboBox_2_activated(int index);

    void on_pBut_SELF_DIAG_clicked();

    void on_pBut_ERRASE_clicked();

    void on_pBut_FIRMWARE_clicked();

    void on_pBut_read_PA26_clicked();

    void on_cBtnSend_clicked();

    void on_pBut_read_IKS_2_clicked();

    bool check_IKPMO(int);

    void on_btn_lens1line1_clicked();

    void on_btn_read_lins1_clicked();

    void on_arbitary_KU_clicked();

    void on_clear_KK_OU_clicked();

    void on_clear_OU_KK_clicked();

    void on_btn_lens1line2_clicked();

    void on_btn_lens1line3_clicked();

    void on_btn_lens1line4_clicked();

    void on_btn_lens1line5_clicked();

    void on_btn_lens1line6_clicked();

    void on_btn_lens1line7_clicked();

    void on_btn_lens1line8_clicked();

    void on_btn_lens1line9_clicked();

    void on_btn_lens1line10_clicked();

    void on_btn_lens1line11_clicked();

    void on_btn_lens1line12_clicked();

    void on_btn_lens1line13_clicked();

    void on_btn_lens1line14_clicked();

    void on_btn_lens1line15_clicked();

    void on_btn_lens1line16_clicked();

    void on_btn_read_lins2_clicked();

    void on_btn_lens2line1_clicked();

    void on_btn_lens2line2_clicked();

    void on_btn_lens2line3_clicked();

    void on_btn_lens2line4_clicked();

    void on_btn_lens2line5_clicked();

    void on_btn_lens2line6_clicked();

    void on_btn_lens2line7_clicked();

    void on_btn_lens2line8_clicked();

    void on_btn_lens2line9_clicked();

    void on_btn_lens2line10_clicked();

    void on_btn_lens2line11_clicked();

    void on_btn_lens2line12_clicked();

    void on_btn_lens2line13_clicked();

    void on_btn_lens2line14_clicked();

    void on_btn_lens2line15_clicked();

    void on_btn_lens2line16_clicked();

    void set_init_state_DC(bool init_state_DC);

    void on_pushButton_clicked();


private:
    Ui::MainWindow  *ui;
    Port            PortNew;
    DC_POWER        PortDCpower;
    MIL_STD_1553    mko;
    uint8_t         numReadEVT = 0;
    uint8_t         MSGtoSEND;
    QByteArray      rxEVT = 0; // массив для приема данных
    uint8_t         sadrEVT = 2; // от 2 так как ПА от 2 до 14
    DWORD           dwCW;
    WORD            pcBuf[64];
    qint64          startTime;
    DWORD           pDWC = 32;
    QString         file_name_IKPMO;
    QByteArray      IKPMO;
    char            status_ARD = 0;
    QTimer          *timerDC;
    bool            state_1hz = 0;
    //bool            init_state_DC = 0;
    char            dump_PA25_DAC_1;
    char            dump_PA25_DAC_2;
    bool            init_state_DC = false;


};

#endif // MAINWINDOW_H
