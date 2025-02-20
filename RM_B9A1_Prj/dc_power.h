#ifndef DC_POWER_H
#define DC_POWER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QThread>

struct Settings_DC_port {
    QString name;
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;
};


class DC_POWER : public QObject
{
    Q_OBJECT

public:

    explicit DC_POWER(QObject *parent = nullptr);

    ~DC_POWER();

    QSerialPort thisPort;

    Settings_DC_port SettingsPort;


signals:

    void finished_DC_POWER();

    void error_(QString err);

    void DC_outPort(QString data);

    void CheckVset1(QString data);

    void CheckVset2(QString data);

    void CheckIset1(QString data);

    void CheckIset2(QString data);

    void get_init_state_DC(bool init_state_DC);

public slots:


    void DisconnectPort();

    void process_Port();

    void ConnectPort();

    void Write_Settings_Port(QString name, int baudrate, int DataBits, int Parity, int StopBits, int FlowControl);

    void WriteToPort(QByteArray data);

    void ReadInPort();

    void setOCPlevel_OUTPUT1(QString data);

    void getOCPlevel_OUTPUT1();

    void setOCPlevel_OUTPUT2(QString data);

    void getOCPlevel_OUTPUT2();

    void turnON_OUTPUT1();

    void turnOFF_OUTPUT1();

    void getSTATe_OUTPUT1();

    void turnON_OUTPUT2();

    void turnOFF_OUTPUT2();

    void getSTATe_OUTPUT2();

    void setVoltage_OUTPUT1(QString data);

    void getCurrentVoltage_OUTPUT1();

    void getCurrentCurrent_OUTPUT1();

    void setVoltage_OUTPUT2(QString data);

    void getCurrentVoltage_OUTPUT2();

    void getCurrentCurrent_OUTPUT2();

    void init();

    void DC_OFF();


private slots:

    void handleError(QSerialPort::SerialPortError error);//
};

#endif // DC_POWER_H
