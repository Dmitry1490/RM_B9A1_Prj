#ifndef BCTHREAD_H
#define BCTHREAD_H
//---------------------------------------------------------------------------
// TE1-PE2 (16K слов)
//---------------------------------------------------------------------------
// BUS_CONTROLLER
#define MAX_BASE              256
#define BASE_LEN              64
// DATA
#define VECTOR_WORD           0x5555
//---------------------------------------------------------------------------
#define MAX_COL               8
#define MAX_ROW               8
//---------------------------------------------------------------------------

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QtCore>
#include <QMessageBox>
#include "wdmtmkv2.h"
#include<array>
using namespace std;


//MIL_STD_1553

class MIL_STD_1553 : public QObject
{
    Q_OBJECT

public:
    explicit MIL_STD_1553(QObject *parent = nullptr);
    ~MIL_STD_1553();
    void setRTA(DWORD RTA);
    void setSA(DWORD SA);
    void setDWC(DWORD DWC);
    void setLPI(bool uBus);
    WORD* getPcBuf();
    void readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC);
    unsigned int getSizePcBuf();


signals:

    void msgWM_BC_INT_NORM();
    void msgWM_BC_INT_EXC();
    void messange_MKO(QString data);
    void error_MKO(QString data);
    void finished_Port();
    void started_thread();
    //void printToTXline(QString data);
    void printToRXline(QString data);

public slots:
    void        DoWork();
    void        closeDriver();
    void        ShowRAM(void);
    void        initMKO(void);
    void        writeToRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC,ushort *pData);
    unsigned short Calc_CRC16(uchar  *pBlk, unsigned int len,unsigned short *pTab16);

private:

    //---------------------------------------------------------------------------
    // for mil_std_1553

    UINT            uBus;
    int             res, count;
    TTmkConfigData  TmkConfigData;
    int             nTMK, nTmkNumber, nMaxBase;
    //static WORD            pcBuf[64];
    unsigned        CtrlCode, CtrlCmd, Result, Num;
    DWORD           RTA, TR, SA, DWC;
    DWORD           dwCW;
    class bcThread  *cObject;
    QThread         *cThread;
    uint8_t         MSGtoSEND;


    //---------------------------------------------------------------------------



    QString     InitMKODriver();
};

#endif // BCTHREAD_H
