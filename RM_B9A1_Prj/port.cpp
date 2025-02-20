#include "port.h"
#include <qdebug.h>

// Реализация "Non-blocking Asynchronous Way";

Port::Port(QObject *parent) :
    QObject(parent)
{
//    timer = new QTimer();
//    timer->start(2000);
//    connect(timer, SIGNAL(timeout()), this, SLOT(second()));

}

Port::~Port()
{
    qDebug("UKS By In Thread!");
    emit finished_Port();
}

void Port :: process_Port(){
    qDebug("Serial Port in Thread!");
    connect(&thisPort,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(&thisPort, SIGNAL(readyRead()),this,SLOT(ReadInPort()));
}

void Port :: Write_Settings_Port(QString name, int baudrate,int DataBits,
                         int Parity,int StopBits, int FlowControl){
    SettingsPort.name = name;
    SettingsPort.baudRate = (QSerialPort::BaudRate) baudrate;
    SettingsPort.dataBits = (QSerialPort::DataBits) DataBits;
    SettingsPort.parity = (QSerialPort::Parity) Parity;
    SettingsPort.stopBits = (QSerialPort::StopBits) StopBits;
    SettingsPort.flowControl = (QSerialPort::FlowControl) FlowControl;
}

void Port :: ConnectPort(){//
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &portInfo : serialPortInfos) {
            if(portInfo.hasProductIdentifier()){
                int q = QByteArray::number(portInfo.productIdentifier(), 16).toInt();
                    if(QByteArray::number(portInfo.productIdentifier(), 16).toInt() == 7523){
                        QString name = portInfo.portName();
                        thisPort.setPortName(name);
                    }
            }
        }
    if (thisPort.open(QIODevice::ReadWrite)) {
        if (thisPort.setBaudRate(QSerialPort::Baud115200)
                && thisPort.setDataBits(QSerialPort::Data8)//DataBits
                && thisPort.setParity(QSerialPort::NoParity)
                && thisPort.setStopBits(QSerialPort::OneStop)
                && thisPort.setFlowControl(QSerialPort::NoFlowControl))
        {
            if (thisPort.isOpen()){
                error_("ComPort Открыт!\r");
                QTimer::singleShot(1000,this, [this](){
                        initARD();
                });
            }
        } else {
            thisPort.close();

            error_(thisPort.errorString().toLocal8Bit());
          }
    } else {
        thisPort.close();
        //error_(thisPort.errorString().toLocal8Bit());
        error_("Ошибка подключения УКС к порту");
        qDebug("Ошибка подключения к COM порту");
    }
}

void Port::handleError(QSerialPort::SerialPortError error)//
{
    if ( (thisPort.isOpen()) && (error == QSerialPort::ResourceError)) {
        error_(thisPort.errorString().toLocal8Bit());
        DisconnectPort();
    }
}//


void  Port::DisconnectPort(){
    if(thisPort.isOpen()){
        thisPort.close();
        error_(SettingsPort.name.toLocal8Bit() + " >> Закрыт!\r");
    }
}

void Port :: WriteToPort(QByteArray data){
    if(thisPort.isOpen()){

        thisPort.write(data);
    }
}
//
void Port :: ReadInPort(){
    QByteArray data;

    data.append(thisPort.readAll());
    //QString st = data;

    if(data == "x\r\n"){
        //qDebug();
        chek_Second();
    }

    outPort(data);

    //((QString)(adr.toInt())).toLatin1().toHex()
}

void Port ::second(){
    chek_Second();
}
