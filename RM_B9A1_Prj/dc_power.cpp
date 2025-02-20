#include "dc_power.h"
#include <qdebug.h>

// Реализация "Blocking Synchronous Way";

DC_POWER::DC_POWER(QObject *parent) : QObject(parent)
{
    //ConnectPort();

}

DC_POWER::~DC_POWER()
{
    qDebug("DC power By In Thread!");
//    if (thisPort.isOpen()){
//        qDebug("Открыт");
//    }
    emit finished_DC_POWER();
}

void DC_POWER :: process_Port(){
    qDebug("DC Power in Thread!");
    connect(&thisPort,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    //
    //connect(&thisPort, SIGNAL(readyRead()),this,SLOT(ReadInPort()));

}

void DC_POWER :: Write_Settings_Port(QString name, int baudrate,int DataBits,
                         int Parity,int StopBits, int FlowControl){
    SettingsPort.name = name;
    SettingsPort.baudRate = (QSerialPort::BaudRate) baudrate;
    SettingsPort.dataBits = (QSerialPort::DataBits) DataBits;
    SettingsPort.parity = (QSerialPort::Parity) Parity;
    SettingsPort.stopBits = (QSerialPort::StopBits) StopBits;
    SettingsPort.flowControl = (QSerialPort::FlowControl) FlowControl;
}

void DC_POWER :: ConnectPort(){

    const auto serialPortInfos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &portInfo : serialPortInfos) {
            if(portInfo.hasProductIdentifier()){
                //int q = QByteArray::number(portInfo.productIdentifier(), 16).toInt();
                    if(QByteArray::number(portInfo.productIdentifier(), 16).toInt() == 51){
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
                //_L0(QString("L0\n").toUtf8());
                emit error_("Источник питания подключен!");
            }
        } else {
            thisPort.close();

            emit error_(thisPort.errorString().toLocal8Bit());
          }
    } else {
        thisPort.close();
        //error_(thisPort.errorString().toLocal8Bit());
        emit error_("Ошибка подключения источника питания к порту;");
        qDebug("Ошибка подключения источника питания к порту;");
    }
}

void DC_POWER::handleError(QSerialPort::SerialPortError error)//
{
    if ( (thisPort.isOpen()) && (error == QSerialPort::ResourceError)) {
        emit error_(thisPort.errorString().toLocal8Bit());
        DisconnectPort();
    }
}//


void  DC_POWER::DisconnectPort(){
    if(thisPort.isOpen()){
        thisPort.close();
        emit error_(SettingsPort.name.toLocal8Bit() + " >> Закрыт!\r");
    }
}

void DC_POWER :: WriteToPort(QByteArray data){
    if(thisPort.isOpen()){

        thisPort.write(data);
    }
}
//
void DC_POWER :: ReadInPort(){
    QByteArray data;

    data.append(thisPort.readAll());
    emit DC_outPort(data);
}

void DC_POWER :: init(){
    if(thisPort.isOpen()){
        QByteArray data;

        // Выключаем все каналы и пишем начальне установки

        thisPort.write(":OUTPut:STATe OFF\n");
        thisPort.waitForBytesWritten(50);

        thisPort.write(":VOLTage:PROTection 30\n");
        thisPort.waitForBytesWritten(50);

        thisPort.write(":CURRent:PROTection 1\n");
        thisPort.waitForBytesWritten(50);

        thisPort.write("APPL 27.00, 1.0\n");
        thisPort.waitForBytesWritten(50);

        thisPort.write(":DISPlay:MENU 3\n");
        thisPort.waitForBytesWritten(50);

        // Делаем запрос состояния и ждем пока данные запишутся.
        thisPort.write("APPLy?\n");
        thisPort.waitForBytesWritten(50);

        thisPort.waitForReadyRead(50);
        data.append(thisPort.readAll());

        QStringList data_s = QString(data).split(',');
        QString s1 = data_s[0].remove("+");
        QString s = data_s[1].remove(" +");
        QString s2 = s.remove("\n");
        if(s1.toFloat() == 27){
            emit CheckVset1(s1);
        }

        if(s2.toFloat() == 1.0){
            emit CheckIset1(s2);
        }

        if(s1.toFloat() == 27 && s2.toFloat() == 1.0){
            emit error_("Инициализация источника питания прошла успешно!");
            emit get_init_state_DC(true);
        }else{
            emit error_("Ошибка инициализация источника питания!");
            emit get_init_state_DC(false);
        }
        data.clear();

    }
}

void DC_POWER :: DC_OFF(){
    if(thisPort.isOpen()){
        //QByteArray data;

        thisPort.write(":OUTPut1:STATe OFF\n");
        thisPort.waitForBytesWritten(50);

        thisPort.write(":OUTPut2:STATe OFF\n");
        thisPort.waitForBytesWritten(50);


        emit error_("Источник питания выключен;");

    }
}


























void DC_POWER :: turnON_OUTPUT1(){
    bool st = thisPort.isOpen();
    if(thisPort.isOpen()){
        QByteArray data;
        // Включаем первый канал источника.
        thisPort.write(":OUTPut1:STATe ON\n");

        // Ждём пока данные запишутся.
        thisPort.waitForBytesWritten(50);

        // Делаем запрос состояния и ждем пока данные запишутся.
        thisPort.write(":OUTPut1:STATe?\n");
        thisPort.waitForBytesWritten(50);

        thisPort.waitForReadyRead(150);
        data.append(thisPort.readAll());

        DC_outPort(data);
    }

}

void DC_POWER :: turnOFF_OUTPUT1(){
    // Включаем первый канал источника.
    WriteToPort(":OUTPut1:STATe OFF\n");

    // Ждём пока данные запишутся.
    thisPort.waitForBytesWritten(50);

    // Делаем запрос состояния и ждем пока данные запишутся.
    WriteToPort(":OUTPut1:STATe?\n");
    thisPort.waitForBytesWritten(50);

    // Ждём пока источник обработает данные и ответит.
    thisPort.waitForReadyRead(50);
    QByteArray data;
    data.append(thisPort.readAll());


}

void DC_POWER :: getSTATe_OUTPUT1(){
    WriteToPort(":OUTPut1:STATe?\n");
    // Ждём пока данные запишутся.
    thisPort.waitForBytesWritten(50);
    // Ждём пока источник обработает данные и ответит.
    thisPort.waitForReadyRead(50);
    QByteArray data;
    data.append(thisPort.readAll());
}

// set Voltage level
void DC_POWER :: setVoltage_OUTPUT1(QString data){
    WriteToPort(QString("VSET1:%1\n").arg(data).toUtf8());
}

void DC_POWER :: getCurrentVoltage_OUTPUT1(){
    WriteToPort("VOUT1?\n");
}

void DC_POWER :: getCurrentCurrent_OUTPUT1(){
    WriteToPort("IOUT1?\n");
}

// set OCP level - over current protection OUTput1
void DC_POWER :: setOCPlevel_OUTPUT1(QString data){
    WriteToPort(QString(":OUTPut1:OCP %1\n").arg(data).toUtf8());
}

void DC_POWER :: getOCPlevel_OUTPUT1(){
    WriteToPort(":OUTPut1:OCP?\n");
}

void DC_POWER :: turnON_OUTPUT2(){
    WriteToPort(":OUTPut2:STATe ON\n");
}

void DC_POWER :: turnOFF_OUTPUT2(){
    WriteToPort(":OUTPut2:STATe OFF\n");
}

void DC_POWER :: getSTATe_OUTPUT2(){
    WriteToPort(":OUTPut2:STATe?\n");
}

// set Voltage level
void DC_POWER :: setVoltage_OUTPUT2(QString data){
    WriteToPort(QString("VSET2:%1\n").arg(data).toUtf8());
}

void DC_POWER :: getCurrentVoltage_OUTPUT2(){
    WriteToPort("VOUT2?\n");
}

void DC_POWER :: getCurrentCurrent_OUTPUT2(){
    WriteToPort("IOUT2?\n");
}


// set OCP level - over current protection OUTput2
void DC_POWER :: setOCPlevel_OUTPUT2(QString data){
    WriteToPort(QString(":OUTPut2:OCP %1\n").arg(data).toUtf8());
}

void DC_POWER :: getOCPlevel_OUTPUT2(){
    WriteToPort(":OUTPut2:OCP?\n");
}

