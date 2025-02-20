#ifndef PORT_H
#define PORT_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <qtimer.h>

struct Settings {
    QString name;
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;
};

struct Messenge_port {
    unsigned __int16 startMessange;
    bool typeMessenge;
    bool dataMessenge;
    bool finishMessange;
};

class Port : public QObject
{
    Q_OBJECT

public:

    explicit Port(QObject *parent = 0);

    ~Port();

    QSerialPort thisPort;

    Settings SettingsPort;

    QTimer *timer;

signals:

    void finished_Port(); //

    void error_(QString err);

    void outPort(QString data);

    void _L0(QByteArray data);

    void chek_Second();

    void initARD();

public slots:

    void DisconnectPort();

    void ConnectPort();

    void Write_Settings_Port(QString name, int baudrate, int DataBits, int Parity, int StopBits, int FlowControl);

    void process_Port();

    void WriteToPort(QByteArray data);

    void ReadInPort();

    void second();

private slots:

    void handleError(QSerialPort::SerialPortError error);//

public:

};

#endif // PORT_H
