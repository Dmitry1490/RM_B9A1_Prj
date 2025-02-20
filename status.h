#ifndef STATUS_H
#define STATUS_H

#include <QObject>
#include <QMutex>

class Status : public QObject
{
    Q_OBJECT
public:
    explicit Status(QObject *parent = nullptr);

    ~Status();



    unsigned char getRegUprPLIS() const;
    void setRegUprPLIS(unsigned char newRegUprPLIS);

    unsigned char getRegUprPwr() const;
    void setRegUprPwr(unsigned char newRegUprPwr);

    unsigned char* getRegUprPeriodOfSec();
    void setRegUprPeriodOfSecJoung(unsigned char a);
    void setRegUprPeriodOfSecOld(unsigned char b);

    unsigned char getRegUprLastingOfSec() const;
    void setRegUprLastingOfSec(unsigned char newRegUprLastingOfSec);

    unsigned char getRegUprLightDiod() const;
    void setRegUprLightDiod(unsigned char newRegUprLightDiod);

    unsigned char getRegUprLastingOfFC() const;
    void setRegUprLastingOfFC(unsigned char newRegUprLastingOfFC);

    unsigned char getRegUprPeriodOfFC() const;
    void setRegUprPeriodOfFC(unsigned char newRegUprPeriodOfFC);

    unsigned char* getRegUprQuantityFC();
    void setRegUprQuantityFCJoung(unsigned char a);
    void setRegUprQuantityFCOld(unsigned char b);

    unsigned char* getRegDelayFC();
    //биты [7:0];
    void setRegDelayFCJoung(unsigned char a);
    //биты [15:8];
    void setRegDelayFCMiddle(unsigned char a);
    // биты [21:16];
    void setRegDelayFCOld(unsigned char a);



private:

    unsigned char regUprPLIS;

signals:

};

#endif // STATUS_H
