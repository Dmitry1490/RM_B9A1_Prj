#include "mainwindow.h"
#include <QMessageBox>
#include <QString>
#include <QDesktopWidget>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <unistd.h>
#include <errno.h>
#include "ui_mainwindow.h"
#include <QThread>
#include <iostream>
#include <QFileDialog>
#include <sstream>

//      Инфо:
//      Сбрасываем бит
//      message &= ~(1 << 0);
//      Устонавливаем бит
//      message |= (1 << 0);
//

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    try {
        ui->setupUi(this);
        initTable();

        //ukas_g.r.kas = 0;
        //ukas_g.r.pn_c = 1;

        //Адреса БРГА1: 6, 14;


    //    //~~~~~~~~~~~~~~Thread of Serial Port~~~~~~~~~~~~~~~

        QThread *thread_New = new QThread;//Создаем поток для порта платы
        Port *PortNew = new Port();//Создаем обьект по классу
        PortNew->moveToThread(thread_New);//помешаем класс  в поток
        PortNew->thisPort.moveToThread(thread_New);//Помещаем сам порт в поток

        connect(PortNew, SIGNAL(error_(QString)), this, SLOT(Print(QString)));//Лог ошибок
        connect(thread_New, SIGNAL(started()), PortNew, SLOT(process_Port()));//Переназначения метода run
        connect(PortNew, SIGNAL(finished_Port()), thread_New, SLOT(quit()));//Переназначение метода выход
        connect(thread_New, SIGNAL(finished()), PortNew, SLOT(deleteLater()));//Удалить поток
        connect(PortNew, SIGNAL(finished_Port()), thread_New, SLOT(deleteLater()));//Удалить поток
        connect(ui->BtnConnect, SIGNAL(clicked()),PortNew,SLOT(ConnectPort()));
        connect(PortNew, SIGNAL(_L0(QByteArray)), this,SIGNAL(writeData(QByteArray)));
        connect(ui->BtnDisconect, SIGNAL(clicked()),PortNew,SLOT(DisconnectPort()));
        //connect(PortNew, SIGNAL(outPort(QString)), this, SLOT(Print(QString)));//Лог ошибок
        connect(PortNew, SIGNAL(outPort(QString)), this, SLOT(transaction_handler(QString)));//Обработчик квитанций
        connect(this,SIGNAL(writeData(QByteArray)),PortNew,SLOT(WriteToPort(QByteArray)));
        connect(PortNew, SIGNAL(chek_Second()), this,SLOT(flashes1HZ()));
        connect(PortNew, SIGNAL(initARD()), this,SLOT(initARD()));
        thread_New->start();

        //~~~~~~~~~~~~~~Thread of MIL_STD_1553~~~~~~~~~~~~~~~

        QThread *thread_MKO = new QThread;//Создаем поток для порта платы
        MIL_STD_1553 *mko = new MIL_STD_1553();//Создаем обьект по классу
        mko->moveToThread(thread_MKO);//помешаем класс  в поток

        connect(mko, SIGNAL(error_MKO(QString)), this, SLOT(PrintERR(QString)));//Лог ошибок
        connect(mko, SIGNAL(messange_MKO(QString)), this, SLOT(Print(QString)));//Лог сообщений
        connect(thread_MKO, SIGNAL(started()), mko, SLOT(DoWork()));//Переназначения метода run
        connect(mko, SIGNAL(msgWM_BC_INT_NORM()), this, SLOT(onMsgWM_BC_INT_NORM()));//Лог сообщений
        //connect(mko, SIGNAL(printToTXline(QString)), this, SLOT(toTXline(QString)));
        connect(mko, SIGNAL(finished_Port()), thread_MKO, SLOT(quit()));
        connect(thread_MKO, SIGNAL(finished()), mko, SLOT(deleteLater()));
        connect(mko, SIGNAL(finished_Port()), thread_MKO, SLOT(deleteLater()));
        MSGtoSEND = 0;
        thread_MKO->start();
        mko->initMKO();

        //~~~~~~~~~~~~~~Thread of serial port DC power~~~~~~~~~~~~~~~

        QThread *thread_DC_power = new QThread;//Создаем поток для порта платы/
        DC_POWER *PortDCpower = new DC_POWER();//Создаем обьект по классу
        PortDCpower->moveToThread(thread_DC_power);//помешаем класс  в поток
        //PortDCpower->thisPort.moveToThread(thread_DC_power);//Помещаем сам порт в поток
        connect(PortDCpower, SIGNAL(error_(QString)), this, SLOT(Print(QString)));//Лог ошибок
        connect(thread_DC_power, SIGNAL(finished()), PortDCpower, SLOT(deleteLater()));//Удалить поток
        connect(PortDCpower, SIGNAL(finished_DC_POWER()), thread_DC_power, SLOT(quit()));//
        connect(PortDCpower, SIGNAL(finished_DC_POWER()), thread_DC_power, SLOT(deleteLater()));//Удалить поток
        connect(thread_DC_power, SIGNAL(started()), PortDCpower, SLOT(process_Port()));//Переназначения метода run
        connect(this, SIGNAL(DC_OFF()), PortDCpower, SLOT(DC_OFF()));//Выключаю источник при закрытии программы
        // После инициализации источника питания обновляем UI;
        connect(PortDCpower, SIGNAL(CheckVset1(QString)), this, SLOT(CheckVset1(QString)));      
        connect(PortDCpower, SIGNAL(CheckIset1(QString)), this, SLOT(CheckIset1(QString)));
        connect(PortDCpower, SIGNAL(get_init_state_DC(bool)), this, SLOT(set_init_state_DC(bool)));

        thread_DC_power->start();
        PortDCpower->ConnectPort();
        PortDCpower->init();

        // Выполняю инициализацию ARD;
        //initARD();

        // Фукция опроса источника питания раз в 500 мсек;
        timerDC = new QTimer();
        connect(timerDC, &QTimer::timeout, this, [this, PortDCpower](){
            QByteArray data;
            if(PortDCpower->thisPort.isOpen()){

                PortDCpower->thisPort.write(":MEASure:VOLTage?\n");
                PortDCpower->thisPort.waitForBytesWritten(50);
                PortDCpower->thisPort.waitForReadyRead(100);
                data.append(PortDCpower->thisPort.readAll());
                ui->lineEdit_5->setText(data);
                data.clear();

                PortDCpower->thisPort.write(":MEASure:CURRent?\n");
                PortDCpower->thisPort.waitForBytesWritten(50);
                PortDCpower->thisPort.waitForReadyRead(100);
                data.append(PortDCpower->thisPort.readAll());
                ui->lineEdit_6->setText(data);
                data.clear();
            }

        });
        timerDC->start(500); // Запускаем таймер;


        // Функция ВКЛ/ВЫКЛ 1 канала источника питания (+27БС)
        connect(ui->BtnConnect_3, &QPushButton::toggled, this, [this, PortDCpower](){
            if(ui->BtnConnect_3->isChecked()){
                QByteArray data;
                if(PortDCpower->thisPort.isOpen()){

                    // Включаем первый канал источника.
                    PortDCpower->thisPort.write(":OUTPut:STATe ON\n");

                    // Ждём пока данные запишутся.
                    PortDCpower->thisPort.waitForBytesWritten(50);

                    // Делаем запрос состояния и ждем пока данные запишутся.
                    PortDCpower->thisPort.write(":OUTP?\n");
                    PortDCpower->thisPort.waitForBytesWritten(50);

                    PortDCpower->thisPort.waitForReadyRead(150);
                    data.append(PortDCpower->thisPort.readAll());

                }
                //QString st = data;
                if(data == "1\n"){
                    ui->BtnConnect_3->setText("Выключить");
                    Print("+27БС включен;");
                } else {
                    ui->BtnConnect_3->setDown(false);
                    //ui->BtnConnect_3->setChecked(false);
                    ui->BtnConnect_3->setText("Включить");
                    Print("Ошибка включения +27БС;");
                }
            } else {
                QByteArray data;
                if(PortDCpower->thisPort.isOpen()){
                    // Включаем первый канал источника.
                    PortDCpower->thisPort.write(":OUTPut:STATe OFF\n");

                    // Ждём пока данные запишутся.
                    PortDCpower->thisPort.waitForBytesWritten(50);

                    // Делаем запрос состояния и ждем пока данные запишутся.
                    PortDCpower->thisPort.write(":OUTP?\n");
                    PortDCpower->thisPort.waitForBytesWritten(50);

                    PortDCpower->thisPort.waitForReadyRead(150);
                    data.append(PortDCpower->thisPort.readAll());
                }
                if(data == "0\n"){
                    ui->BtnConnect_3->setText("Включить");
                    Print("+27БС выключен;");
                } else {
                    ui->BtnConnect_3->setChecked(true);
                    ui->BtnConnect_3->setText("Выключить");
                    Print("Ошибка выключения +27БС");
                }
            }
        });


        // Vset БС - изменение напряжения на источнике питания (Канал БС)
        connect(ui->doubleSpinBox_6, &QDoubleSpinBox::editingFinished, this, [this, PortDCpower](){
            if(PortDCpower->thisPort.isOpen()){
                QByteArray data;
                PortDCpower->thisPort.write(QString("APPL %1, %2\n").arg(ui->doubleSpinBox_6->text()).arg(ui->doubleSpinBox_12->text()).toUtf8());
                // Ждём пока данные запишутся.
                PortDCpower->thisPort.waitForBytesWritten(50);
                // Делаем запрос состояния и ждем пока данные запишутся.
                PortDCpower->thisPort.write(":APPLy?\n");
                PortDCpower->thisPort.waitForBytesWritten(50);

                PortDCpower->thisPort.waitForReadyRead(50);
                data.append(PortDCpower->thisPort.readAll());

                QStringList data_s = QString(data).split(',');
                QString s1 = data_s[0].remove("+");
                QString s = data_s[1].remove(" +");
                QString s2 = s.remove("\n");
                if(s1.toFloat() == 27){
                    CheckVset1(s1);
                }

                if(s2.toFloat() == 0.8){
                    CheckIset1(s2);
                }
            }

        });

        // Iset БС - изменение тока на источнике питания (Канал БС)
        connect(ui->doubleSpinBox_12, &QDoubleSpinBox::editingFinished, this, [this, PortDCpower](){
            if(PortDCpower->thisPort.isOpen()){
                QByteArray data;
                PortDCpower->thisPort.write(QString("APPL %1, %2\n").arg(ui->doubleSpinBox_6->text()).arg(ui->doubleSpinBox_12->text()).toUtf8());
                // Ждём пока данные запишутся.
                PortDCpower->thisPort.waitForBytesWritten(50);
                // Делаем запрос состояния и ждем пока данные запишутся.
                PortDCpower->thisPort.write(":APPLy?\n");
                PortDCpower->thisPort.waitForBytesWritten(50);

                PortDCpower->thisPort.waitForReadyRead(50);
                data.append(PortDCpower->thisPort.readAll());

                QStringList data_s = QString(data).split(',');
                QString s1 = data_s[0].remove("+");
                QString s = data_s[1].remove(" +");
                QString s2 = s.remove("\n");
                if(s1.toFloat() == 27){
                    CheckVset1(s1);
                }

                if(s2.toFloat() == ui->doubleSpinBox_12->text().toFloat()){
                    CheckIset1(s2);
                }
            }

        });

    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }


}

// инициализация Arduino
void MainWindow::initARD(){

    if(init_state_DC){

        emit writeData(QString("R\n").toUtf8());
        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };


        emit writeData(QString("P0\n").toUtf8());
        // Ожидаю квитанцию "p", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_p)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
            };


        emit writeData(QString("K0\n").toUtf8());
        // Ожидаю квитанцию "k", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_k)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции k;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции k;");
            };


        emit writeData(QString("C0\n").toUtf8());
        // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_c)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
            };


        emit writeData(QString("D0\n").toUtf8());
        // Ожидаю квитанцию "d", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_d)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции d;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции d;");
            };

        emit writeData(QString("R\n").toUtf8());
        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(status_ARD == 0x20)  {
            Print("Инициализация прошла успешно;");
            ui->pBut_BRGA1_A->setEnabled(true);
            ui->pBut_BRGA1_B->setEnabled(true);
            ui->pBut_BRGA1h->setEnabled(true);
        }
        else                    Print("Ошибка инициализации УКС;");

    } else Print("Ошибка инициализации УКС. Повторите инициализацию источника питания");


}

// инициализация таблицы EVT
void MainWindow::initTable(){

//    ui->lEdit_LEN->setInputMask("HHHHHHHH");
//    ui->lEdit_ADR->setInputMask("HH");

    //
    ui->pBut_BRGA1_A->setEnabled(false);
    ui->pBut_BRGA1_B->setEnabled(false);
    ui->pBut_BRGA1h->setEnabled(false);

    //------------------------------------------------
    // инициализация  ComboBox сортировки
    ui->comboBox->addItems(QStringList {"6","14"});
    //------------------------------------------------
    // инициализация BANK N
    ui->comboBox_3->addItems(QStringList {"0","1","2"});
    //------------------------------------------------
    // инициализация BANK SRC
    ui->comboBox_4->addItems(QStringList {"1","2"});
    //------------------------------------------------
    // инициализация BANK DST
    ui->comboBox_7->addItems(QStringList {"1","2"});

    // инициализация type
    ui->comboBox_type->addItems(QStringList {"во всех","в 1-ом", "во 2-ом"});

    // инициализация BANK
    ui->comboBox_BANK->addItems(QStringList {"1","2"});

    //------------------------------------------------
    // инициализация таблицы ComboBox сортировки
    ui->cBoxSort->addItems(QStringList {"Новые внизу","Новые вверху"});

    //------------------------------------------------
    // инициализация ComboBox2 (Длительность секунды)
    ui->comboBox_2->addItems(QStringList {"0,3","0,5","0,7"});

    //------------------------------------------------
    // инициализация таблицы DATA
    // запрет редактирования и выделения всей таблицы
    ui->tableDATAn->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableDATAn->setSelectionMode(QAbstractItemView::NoSelection);
    // добавление виджета в каждую ячейку, чтобы можно было ее редактировать
    for(int i = 0; i < 32; i++)
    {
        ui->tableDATAn->setItem(i,0,new QTableWidgetItem(QString("%1").arg(i)));
        ui->tableDATAn->item(i,0)->setTextAlignment(Qt::AlignCenter);
        ui->tableDATAn->item(i,0)->setText("");
        // красим весь столбец
        ui->tableDATAn->item(i,0)->setBackground(clButtonFace);
        // добавляем номер, кроме 0,0 ячейки
        if (i != 0) { ui->tableDATAn->item(i,0)->setText(QString::number(i)); }

        for(int j = 0; j < 4; j++)
        {
            ui->tableDATA->setItem(i,j,new QTableWidgetItem(QString("%1,%2").arg(i).arg(j)));
            ui->tableDATA->item(i,j)->setTextAlignment(Qt::AlignCenter);
            ui->tableDATA->item(i,j)->setText("");
            // красим 0 строку
            if (i == 0) { ui->tableDATA->item(i,j)->setBackground(clButtonFace); }
        }
    }
    ui->tableDATAn->horizontalHeader()->resizeSection(0,21);
    ui->tableDATA->setColumnWidth(0, 70);
    ui->tableDATA->item(0,0)->setText("DATA");
    ui->tableDATA->item(0,1)->setText("В ОУ");
    ui->tableDATA->item(0,2)->setText("ИЗ ОУ");
    ui->tableDATA->item(0,3)->setText("Dump");

    //------------------------------------------------

    //------------------------------------------------
    // инициализация таблицы EVT
    // запрет редактирования и выделения всей таблицы
    //ui->tableEvents->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableEvents->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableEvents->setSelectionMode(QAbstractItemView::NoSelection);
    // добавление виджета в каждую ячейку, чтобы можно было ее редактировать
    for(int i = 0; i < 32; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            ui->tableEvents->setItem(i,j,new QTableWidgetItem(QString("%1,%2").arg(i).arg(j)));
            ui->tableEvents->item(i,j)->setTextAlignment(Qt::AlignCenter);
            ui->tableEvents->item(i,j)->setText("");

            if (i == 0) // красим 0 строку
            {
                ui->tableEvents->item(i,j)->setBackground(clGreenGLN);
            }
            if (j == 0) // красим 0 столбец
            {
                ui->tableEvents->item(i,j)->setText(QString::number(i));
            }
        }
    }
    ui->tableEvents->horizontalHeader()->resizeSection(0,20);
    ui->tableEvents->horizontalHeader()->resizeSection(1,150);
    ui->tableEvents->horizontalHeader()->resizeSection(2,80);
    ui->tableEvents->horizontalHeader()->resizeSection(2,80);


    ui->tableEvents->item(0,1)->setText("t0");
    ui->tableEvents->item(0,2)->setText("Δt");
    ui->tableEvents->item(0,3)->setText("№ линейки");

    //------------------------------------------------
}

//++++++++[Процедура закрытия приложения]+++++++++++++++++++++++++++++++++++++++++++++
MainWindow::~MainWindow()
{
    emit DC_OFF();
    mko.closeDriver();
    delete timerDC;
    delete ui; //Удаление формы
}

//+++++++++++++[Процедура ввода данных из строки (COM_PORT) ]++++++++++++++++++++++++++++++++++++++++
void MainWindow::on_cEnterText_returnPressed()
{
    try {
        //QByteArray data; // Текстовая переменная
        //data.clear();
        QString data = ui->cEnterText->text() + '\n'; // Присвоение "data" значения из EnterText
        emit writeData(data.toUtf8()); // Отправка данных в порт
        Print(data); // Вывод данных в консоль
    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }

}


//+++++++++++++[Процедура вывода данных в консоль]++++++++++++++++++++++++++++++++++++++++
void MainWindow::Print(QString data)
{
    try {
        QString messange = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + " >> " + data;
        //qint64 time = QDateTime::currentMSecsSinceEpoch();
        //QString messange = QString::number(time) + " >> " + data;
        QTextCharFormat fmt = ui->consol->currentCharFormat();
        fmt.setForeground(QBrush(Qt::black));
        ui->consol->setCurrentCharFormat(fmt);
        ui->consol->textCursor().insertText(messange+'\r'); // Вывод текста в консоль
        ui->consol->moveCursor(QTextCursor::End);//Scroll
    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }

}

//+++++++++++++[Процедура вывода ошибки в консоль]++++++++++++++++++++++++++++++++++++++++
void MainWindow::PrintERR(QString data)
{
    try {
        QString messange = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + " >> " + data;
        //qint64 time = QDateTime::currentMSecsSinceEpoch();
        //QString messange = QString::number(time) + " >> " + data;
        QTextCharFormat fmt = ui->consol->currentCharFormat();
        fmt.setForeground(QBrush(Qt::red));
        ui->consol->setCurrentCharFormat(fmt);
        ui->consol->textCursor().insertText(messange+'\r'); // Вывод текста в консоль
        ui->consol->moveCursor(QTextCursor::End);//Scroll
    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }

}

//+++++++++++++[Процедура вывода данных в консоль "Передача (TX: КК -ОУ)"]++++++++++++++++++++++++++++++++++++++++
void MainWindow::toTXline(WORD *data)
{
    try {
        QString hex;
        for(int i = 0; i < 32; i++){
            hex += QString(" %1").arg(data[i], 4, 16, QLatin1Char( '0' ));
        }

        uint16_t time = QDateTime::currentMSecsSinceEpoch() - startTime;
        QString messange = QString("%1").arg(time).toUtf8() + " >> " + hex;
        QTextCharFormat fmt = ui->textEditTX->currentCharFormat();
        //QString messangeAll = " >> " + QString((QString(QString(messange[0]).toUInt()))) ;
        fmt.setForeground(QBrush(Qt::black));
        ui->textEditTX->setCurrentCharFormat(fmt);
        ui->textEditTX->textCursor().insertText(messange + ';' + '\r'); // Вывод текста в консоль
        ui->textEditTX->moveCursor(QTextCursor::End);
    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }

}

//+++++++++++++[Процедура вывода данных в консоль "Передача (TX: КК -ОУ)"]++++++++++++++++++++++++++++++++++++++++
void MainWindow::toRXline(WORD *data)
{
    try {
        QString hex;
        for(int i = 0; i < 34; i++){
            hex += QString(" %1").arg(data[i], 4, 16, QLatin1Char( '0' ));
        }

        uint16_t time = QDateTime::currentMSecsSinceEpoch() - startTime;
        QString messange = QString("%1").arg(time).toUtf8() + " >> " + hex;
        QTextCharFormat fmt = ui->textEditRX->currentCharFormat();
        //QString messangeAll = " >> " + QString((QString(QString(messange[0]).toUInt()))) ;
        fmt.setForeground(QBrush(Qt::black));
        ui->textEditRX->setCurrentCharFormat(fmt);
        ui->textEditRX->textCursor().insertText(messange + ';' + '\r'); // Вывод текста в консоль
        ui->textEditRX->moveCursor(QTextCursor::End);
    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }

}

//---------------------------------------------------------------------------


//////---------------------------------------------------------------------------
////// обработка прерывания - нормальное завершение обмена по МКО
void MainWindow::onMsgWM_BC_INT_NORM()
{
    try {
        int i;
        int ind,indEVT;
        uint8_t adrOU,sadrOU,dl;
        ushort rxdat[64] = {0}; // массив для приема данных
        unsigned short cCS;

        // Изммерительная информация:
        uint32_t t0 = 0; // время регистрации
        uint8_t number_line; // номер линейки
        uint8_t delta_t; // длительность

        //WORD* pcBuf = mko.getPcBuf();
        //unsigned int size_pcBuf = mko.getSizePcBuf();

        memset(pcBuf, 0, sizeof(pcBuf));

        // чтение принятых данных от ОУ
        // OУ-KK (ф.2)
        //  Смещение в базе:    00  01  02  03  …   …   32  33
        //      OУ-KK (ф.2)     KC  ОС  ИС  ИС  …   …   ИС  ИС

        bcgetblk(0, pcBuf, 64);

        ui->RTA2->setText(QString(" %1").arg(pcBuf[0], 4, 16, QLatin1Char( '0' )).toUpper());
        ui->RTA3->setText(QString(" %1").arg(pcBuf[1], 4, 16, QLatin1Char( '0' )).toUpper());

        // OУ-KK (ф.2)
        //
        if (pcBuf[0] & RT_DIR_MASK)
        {

           adrOU = (pcBuf[0] >> 11) & 0x1F; // адрес ОУ
           sadrOU = (pcBuf[0] >> 5) & 0x1F; ; // подадрес ОУ
           dl = (pcBuf[0] & 0x1F); ; // число СД
           if (dl == 0) dl = 32;
           // переписываем все сообщение в рабочий массив
           for(i = 0; i < dl; ++i) rxdat[i] = pcBuf[i+2];
           cCS = Calc_CRC16((uchar *)rxdat,(dl-1)*2,0); // расчет CS
           ui->RTA4->setText(QString(" %1").arg(cCS, 4, 16, QLatin1Char( '0' )).toUpper());

           toRXline(pcBuf);

           if(adrOU == 6 || adrOU == 14 )  // резервный комплект
           {
               if (sadrOU == 1) // чтение ИКС ПА1
               {
                   //cCS = Calc_CRC16((uchar *)rxdat,(12-1)*2,0); // перерасчет CS
                   if (cCS != rxdat[31]) ui->led_errCRC_IKS->setPalette(clGreen); else ui->led_errCRC_IKS->setPalette(clButtonFace);
                   ui->lEdit_RTA_5->setText(QString("%1").arg(rxdat[31], 4, 16, QLatin1Char( '0' )).toUpper()); // CSrx

                   // SW0 Пропадание бортовой секунды;
                   if (rxdat[0] & 0x0001) ui->led_lost_sec->setPalette(clGreen); else ui->led_lost_sec->setPalette(clButtonFace);
                   // SW1 Ошибка DSP;
                   if (rxdat[0] & 0x0002) ui->led__err_DSP->setPalette(clGreen); else ui->led__err_DSP->setPalette(clButtonFace);
                   // SW2 Неожиданное прерывание;
                   if (rxdat[0] & 0x0004) ui->led_except_CP->setPalette(clGreen); else ui->led_except_CP->setPalette(clButtonFace);
                   // SW3 Неожиданный перезапуск;
                   if (rxdat[0] & 0x0008) ui->led_restart->setPalette(clGreen); else ui->led_restart->setPalette(clButtonFace);
                   // SW4 ненорма ОЗУ;
                   if (rxdat[0] & 0x0010) ui->led_nonnorm_RAM->setPalette(clGreen); else ui->led_nonnorm_RAM->setPalette(clButtonFace);
                   // SW5 ненорма СОЗУ;
                   if (rxdat[0] & 0x0020) ui->led__nonnorm_sozu->setPalette(clGreen); else ui->led__nonnorm_sozu->setPalette(clButtonFace);
                   // SW6 ошибка CRC32, БАНК1;
                   if (rxdat[0] & 0x0040) ui->led_err_BANK1->setPalette(clGreen); else ui->led_err_BANK1->setPalette(clButtonFace);
                   // SW7 ошибка CRC32, БАНК2;
                   if (rxdat[0] & 0x0080) ui->led_err_BANK2->setPalette(clGreen); else ui->led_err_BANK2->setPalette(clButtonFace);
                   // SW8 в БРГА1 не используется 0x0100 fpga_err
                   // SW9 FPGA_load 0x0200
                   if (rxdat[0] & 0x0200) ui->led__fpga_load->setPalette(clGreen); else ui->led__fpga_load->setPalette(clButtonFace);
                   // SW10 Подключение ТК
                   if (rxdat[0] & 0x0400) ui->led_on_off_TK->setPalette(clGreen); else ui->led_on_off_TK->setPalette(clButtonFace);
                   // SW11 не используется 0x0800
                   // SW12 тип работающего ПО
                   if (rxdat[0] & 0x1000) ui->led_type_PO->setPalette(clGreen); else ui->led_type_PO->setPalette(clButtonFace);
                   // SW13 коррекция времени
                   if (rxdat[0] & 0x2000) ui->led_correction_on->setPalette(clGreen); else ui->led_correction_on->setPalette(clButtonFace);
                   // SW14 норма CRC32 ИКПМО
                   if (rxdat[0] & 0x4000) ui->led_norm_IKPMO->setPalette(clGreen); else ui->led_norm_IKPMO->setPalette(clButtonFace);
                   // SW15 Состояние регистрации
                   if (rxdat[0] & 0x8000) ui->led_registration_on->setPalette(clGreen); else ui->led_registration_on->setPalette(clButtonFace);


                   // СД1 SW
                   ui->lb_IKS_1->setText(QString("%1").arg(rxdat[0], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД2 EVQ
                   numReadEVT = rxdat[1];
                   ui->lb_IKS_2->setText(QString("%1").arg(rxdat[1], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД3 СД4 SEC
                   ui->lb_IKS_34->setText(QString::number((rxdat[3]<<16) | rxdat[2]));
                   // СД5 СД6 C_1HZ
                   ui->lb_IKS_56->setText(QString("%1").arg((((rxdat[5] & 0xFF)<<16) | rxdat[4]), 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД7 MODE Режим работы ПО
                   ui->lb_IKS_7->setText(QString("%1").arg(rxdat[6], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД8 TEMP
                   ui->lb_IKS_8->setText(QString("%1").arg(rxdat[7], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД9 Код последней команды
                   ui->lb_IKS_9->setText(QString("%1").arg(rxdat[8], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД10 KAS
                   ui->lb_IKS_10->setText(QString("%1").arg(rxdat[9], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД11 BUILD
                   ui->lb_IKS_11->setText(QString("%1").arg(rxdat[10], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД12 BLD0
                   ui->lb_IKS_12->setText(QString("%1").arg(rxdat[11], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД13 BLD1
                   ui->lb_IKS_13->setText(QString("%1").arg(rxdat[12], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД14 BLD2
                   ui->lb_IKS_14->setText(QString("%1").arg(rxdat[13], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД15 B_IKP
                   ui->lb_IKS_15->setText(QString("%1").arg(rxdat[14], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД16 C_IKP
                   ui->lb_IKS_16->setText(QString("%1").arg(rxdat[15], 4, 16, QLatin1Char( '0' )).toUpper());

                   // СД17 DMAPS DEBUG
                   ui->lb_IKS_17->setText(QString("%1").arg(rxdat[16], 4, 16, QLatin1Char( '0' )).toUpper());

                   //СД17 DMAPS СД18 (DBG1) + СД19 (DBG2) + СД20 (DBG3) + СД21 (DBG4) + СД22 (DBG5)
                   ui->lb_IKS_18_22->setText(QString("%1").arg(rxdat[16], 4, 16, QLatin1Char( '0' )).toUpper() +
                                             QString("%1").arg(rxdat[17], 4, 16, QLatin1Char( '0' )).toUpper() +
                                             QString("%1").arg(rxdat[18], 4, 16, QLatin1Char( '0' )).toUpper() +
                                             QString("%1").arg(rxdat[19], 4, 16, QLatin1Char( '0' )).toUpper() +
                                             QString("%1").arg(rxdat[20], 4, 16, QLatin1Char( '0' )).toUpper() +
                                             QString("%1").arg(rxdat[21], 4, 16, QLatin1Char( '0' )).toUpper()
                           );

                   // СД18 DBG1
                   ui->lb_DBG0->setText(QString("%1").arg(rxdat[17], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД19 DBG2
                   ui->lb_DBG1->setText(QString("%1").arg(rxdat[18], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД20 DBG3
                   ui->lb_DBG2->setText(QString("%1").arg(rxdat[19], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД21 DBG4
                   ui->lb_DBG3->setText(QString("%1").arg(rxdat[20], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД22 DBG5
                   ui->lb_DBG4->setText(QString("%1").arg(rxdat[21], 4, 16, QLatin1Char( '0' )).toUpper());

                   // СД23 SERR
                   ui->lb_IKS_23->setText(QString("%1").arg(rxdat[22], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД24 DERR
                   ui->lb_IKS_24->setText(QString("%1").arg(rxdat[23], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД25 СД26 STATUS
                   ui->lb_IKS_2526->setText(QString("%1").arg((((rxdat[25] & 0xFF)<<16) | rxdat[24]), 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД27 СД28 CAUSE
                   ui->lb_IKS_2728->setText(QString("%1").arg((((rxdat[27] & 0xFF)<<16) | rxdat[26]), 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД29 СД30 EPC
                   ui->lb_IKS_2930->setText(QString("%1").arg((((rxdat[29] & 0xFF)<<16) | rxdat[28]), 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД31 RTA
                   ui->lb_IKS_31->setText(QString("%1").arg(rxdat[30], 4, 16, QLatin1Char( '0' )).toUpper());
                   // СД32 CRC16
                   ui->lb_IKS_32->setText(QString("%1").arg(rxdat[31], 4, 16, QLatin1Char( '0' )).toUpper());


                   // Прочитать EVQ
                   // Проверяем numReadEVT - количество событий, если numReadEVT > 0,
                   // то читаем с ПА2-ПА13
                   if(numReadEVT > 0){
                       for(int i = 2; i < 14; i++){
                           read_EVT(i);
                       }
                   }

               }
               if ((sadrOU >= 2) && (sadrOU < 14)) // прием сообщения Измерительная информация (ПА 2 - 13)
               {

                  // дополняем глобальный массив rxEVT каждый раз на 32 слова
                  ind = 0;
                  while (ind < dl)
                  {
                      // Старшие пишутся первыми
                      rxEVT.append((rxdat[ind] >> 8) & 0xFF);
                      rxEVT.append(rxdat[ind] & 0xFF);
                      ind++;
                  }

                  indEVT = 1; ind = 0;
                  while (indEVT <= numReadEVT+1)
                  {
                  // СД1 t0[15:0] СД2 t0[3:0]
                  t0 = static_cast<uint32_t>((((rxEVT[ind+3] & 0xF) << 16) | ((rxEVT[ind] & 0xFF) << 8) | (rxEVT[ind+1] & 0xFF)) & 0xFFFFF);
                  // СД2 number_line[7:4]
                  number_line = static_cast<uint8_t>((rxEVT[ind+3] >> 4) & 0xF);
                  // СД2 delta_t[15:8]
                  delta_t = rxEVT[ind+2];

                  if (indEVT < 32)
                  {
                      ui->tableEvents->item(indEVT,1)->setText(QString::number(t0));
                      ui->tableEvents->item(indEVT,2)->setText(QString::number(delta_t));
                      ui->tableEvents->item(indEVT,3)->setText(QString::number(number_line));
                  }
                  ind += 12;
                  indEVT++;
                  }
               }
               if (sadrOU == 27) // прием сообщения на команду "GET_ULA" ПА27
               {
                   ui->lEdit_RTA_5->setText(QString(" %1").arg(rxdat[31], 4, 16, QLatin1Char( '0' )).toUpper()); // CSrx

                   memset(&uula_g_s,         0, sizeof(uula_g_s)); // Очистка пямяти
                   memcpy(&uula_g_s, &rxdat[0], sizeof(uula_g_s)); // Копирование данных

                   // СД2[7:0]  REG_UPR[7:0]
                   ui->lEdit_REG_UPR->setText(QString("%1").arg(uula_g_s.r.R_UPR, 2, 16, QLatin1Char( '0' )).toUpper());

                   // СД2[15:8] REG_KOM[7:0]
                   ui->lEdit_REG_KOM->setText(QString("%1").arg(uula_g_s.r.R_KOM, 2, 16, QLatin1Char( '0' )).toUpper());

                   // СД3[7:0]  REG_COR[7:0]
                   ui->lEdit_REG_COR->setText(QString("%1").arg(uula_g_s.r.T_COR, 2, 16, QLatin1Char( '0' )).toUpper());

                   // СД3[15:8] REG_DAC_EN[7:0]
                   ui->lEdit_REG_DAC_EN->setText(QString("%1").arg(uula_g_s.r.T_DAC_EN, 2, 16, QLatin1Char( '0' )).toUpper());

                   // СД4[15:0] REG_TIME_FC[15:0]
                   // СД5[7:0]  REG_TIME_FC [23:16]
                   uint32_t TIME_FC = uula_g_s.r.T_FC[2]<<16 |  uula_g_s.r.T_FC[1]<<8 | uula_g_s.r.T_FC[0];
                   ui->lEdit_REG_TIME_FC->setText(QString("%1").arg(TIME_FC, 6, 16, QLatin1Char( '0' )).toUpper());

                   // СД5[15:8] REG_Length_FС [7:0]
                   ui->lEdit_REG_LENGHT_FC->setText(QString("%1").arg(uula_g_s.r.Length_FC, 2, 16, QLatin1Char( '0' )).toUpper());

                   // СД6[15:0] REG_SUMM_HZ[15:0]
                   // СД7[7:0]  REG_SUMM_HZ [23:16]
                   // СД7[15:8] empty
                   uint32_t REG_SUMM_HZ = uula_g_s.r.R_SUMM_HZ[2]<<16 |  uula_g_s.r.R_SUMM_HZ[1]<<8 | uula_g_s.r.R_SUMM_HZ[0];
                   ui->lEdit_REG_SUMM_HZ->setText(QString("%1").arg(REG_SUMM_HZ, 6, 16, QLatin1Char( '0' )).toUpper());

                   // СД8[15:0]  REG_HZ[7:0]
                   // СД9[7:0]   REG_HZ[15:0]
                   uint32_t REG_HZ = uula_g_s.r.REG_HZ[2]<<16 |  uula_g_s.r.REG_HZ[1]<<8 | uula_g_s.r.REG_HZ[0];
                   ui->lb_REG_HZ->setText(QString("%1").arg(REG_HZ, 6, 16, QLatin1Char( '0' )).toUpper());

                   // СД9[15:8] D_TERMO[7:0]
                   ui->lb_REG_TERMO->setText(QString("%1").arg(uula_g_s.r.D_TERMO, 2, 16, QLatin1Char( '0' )).toUpper());

                   // СД10[15:0] DAC_CRC[15:0]
                   ui->lb_DAC_CRC->setText(QString("%1").arg(uula_g_s.r.DAC_CRC, 4, 16, QLatin1Char( '0' )).toUpper());

                   // СД11[15:0] DAC_CRC_C[15:0]
                   ui->lb_DAC_CRC_C->setText(QString("%1").arg(uula_g_s.r.DAC_CRC_C, 4, 16, QLatin1Char( '0' )).toUpper());

                   qDebug();

               }

               if(sadrOU == 25 && pcBuf[30]){
                   for(i = 0; i < 31; i++)
                   {
                       ui->tableDATA->item(i-1,3)->setText("");
                       ui->tableDATA->item(i-1,3)->setText(QString(" %1").arg(pcBuf[i], 4, 16, QLatin1Char( '0' )).toUpper());

//                       if(dump_PA25_DAC_1 > 0 ){
//                           switch (dump_PA25_DAC_1) {

//                           case 1: {
//                               udac1.r.line1    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line2    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready1();
//                               break;
//                           }

//                           case 2: {
//                               udac1.r.line3    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line4    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready2();
//                               break;
//                           }

//                           case 3: {
//                               udac1.r.line5    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line6    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready3();
//                               break;
//                           }

//                           case 4: {
//                               udac1.r.line7    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line8    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready4();
//                               break;
//                           }

//                           case 5: {
//                               udac1.r.line9    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line10   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready5();
//                               break;
//                           }

//                           case 6: {
//                               udac1.r.line11   = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line12   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready6();
//                               break;

//                           }

//                           case 7: {
//                               udac1.r.line13   = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line14   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready7();
//                               break;

//                           }

//                           case 8: {
//                               udac1.r.line15   = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac1.r.line16   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC1_25PA_ready8();
//                               break;
//                           }
//                           }

//                       }

//                       if(dump_PA25_DAC_2 > 0 ){
//                           switch (dump_PA25_DAC_2) {

//                           case 1: {
//                               udac2.r.line1    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line2    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready1();
//                               break;
//                           }

//                           case 2: {
//                               udac2.r.line3    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line4    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready2();
//                               break;
//                           }

//                           case 3: {
//                               udac2.r.line5    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line6    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready3();
//                               break;
//                           }

//                           case 4: {
//                               udac2.r.line7    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line8    = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready4();
//                               break;
//                           }

//                           case 5: {
//                               udac2.r.line9    = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line10   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready5();
//                               break;
//                           }

//                           case 6: {
//                               udac2.r.line11   = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line12   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready6();
//                               break;

//                           }

//                           case 7: {
//                               udac2.r.line13   = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line14   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready7();
//                               break;

//                           }

//                           case 8: {
//                               udac2.r.line15   = ((pcBuf[1]  & 0xFF) << 16 ) | (pcBuf[0] & 0xFF);
//                               udac2.r.line16   = ((pcBuf[3]  & 0xFF) << 16 ) | (pcBuf[2] & 0xFF);
//                               emit DAC2_25PA_ready8();
//                               break;
//                           }
//                           }

//                       }
                   }
               }

               if(sadrOU == 28){

                   ui->lEdit_CMD->setText(QString("%1").arg((rxdat[1] << 16) | rxdat[0], 4, 16, QLatin1Char( '0' )).toUpper());
                   ukas_g.r.cmd = (rxdat[1] << 16) | rxdat[0];

                   ui->lEdit_ADDR->setText(QString("%1").arg((rxdat[3] << 16) | rxdat[2], 4, 16, QLatin1Char( '0' )).toUpper());
                   ukas_g.r.addr = (rxdat[3] << 16) | rxdat[2];

                   ui->lEdit_KAS->setText(QString("%1").arg(rxdat[5], 4, 16, QLatin1Char( '0' )).toUpper());
                   ukas_g.r.kas = rxdat[5];

                   ui->lEdit_PN_C->setText(QString("%1").arg(rxdat[4], 4, 16, QLatin1Char( '0' )).toUpper());
                   ukas_g.r.pn_c = rxdat[4];

                   ui->lEdit_PN_R->setText(QString("%1").arg(rxdat[6], 4, 16, QLatin1Char( '0' )).toUpper());
                   ukas_g.r.pn_r = rxdat[6];

                   ui->lEdit_CRC->setText(QString("%1").arg(rxdat[7], 4, 16, QLatin1Char( '0' )).toUpper());
                   ukas_g.r.crc16 = rxdat[7];

                   emit KAS_ready();

               }
               if(sadrOU == 26){
                   // СД1 (valid)

                   // VF1_1
                   if (rxdat[0] & 0x0001) ui->led_VF1_0->setPalette(clGreen); else ui->led_VF1_0->setPalette(clButtonFace);
                   // VF2_1
                   if (rxdat[0] & 0x0004) ui->led_VF2_0->setPalette(clGreen); else ui->led_VF2_0->setPalette(clButtonFace);
                   // VF3_1
                   if (rxdat[0] & 0x0010) ui->led_VF3_0->setPalette(clGreen); else ui->led_VF3_0->setPalette(clButtonFace);
                   // VF4_1
                   if (rxdat[0] & 0x0040) ui->led_VF4_0->setPalette(clGreen); else ui->led_VF4_0->setPalette(clButtonFace);
                   // VF5_0

                   // VF1_2
                   if (rxdat[1] & 0x0002) ui->led_VF1_1->setPalette(clGreen); else ui->led_VF1_1->setPalette(clButtonFace);
                   // VF2_2
                   if (rxdat[1] & 0x0008) ui->led_VF2_1->setPalette(clGreen); else ui->led_VF2_1->setPalette(clButtonFace);
                   // VF3_2
                   if (rxdat[1] & 0x0020) ui->led_VF3_0->setPalette(clGreen); else ui->led_VF3_0->setPalette(clButtonFace);
                   // VF4_2
                   if (rxdat[1] & 0x0080) ui->led_VF4_0->setPalette(clGreen); else ui->led_VF4_0->setPalette(clButtonFace);
                   // VF5_1

                   // CD3_VF1_1
                   ui->lb_CD3_VF1_1->setText(QString("%1").arg(rxdat[2], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD4_VF1_1
                   ui->lb_CD4_VF1_2->setText(QString("%1").arg(rxdat[3], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD5_VF2_1
                   ui->lb_CD5_VF2_1->setText(QString("%1").arg(rxdat[4], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD6_VF2_2
                   ui->lb_CD6_VF2_2->setText(QString("%1").arg(rxdat[5], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD7_VF3_1
                   ui->lb_CD7_VF3_1->setText(QString("%1").arg(rxdat[6], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD8_VF3_2
                   ui->lb_CD8_VF3_2->setText(QString("%1").arg(rxdat[7], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD9_VF4_1
                   ui->lb_CD9_VF4_1->setText(QString("%1").arg(rxdat[8], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD10_VF4_2
                   ui->lb_CD10_VF4_2->setText(QString("%1").arg(rxdat[9], 4, 16, QLatin1Char( '0' )).toUpper());

                   // CD11_TF1_1
                   ui->lb_CD11_TF1_1->setText(QString("%1").arg(rxdat[10], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD12_TF1_1
                   ui->lb_CD12_TF1_2->setText(QString("%1").arg(rxdat[11], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD13_TF2_1
                   ui->lb_CD13_TF2_1->setText(QString("%1").arg(rxdat[12], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD14_TF2_2
                   ui->lb_CD14_TF2_2->setText(QString("%1").arg(rxdat[13], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD15_TF3_1
                   ui->lb_CD15_TF3_1->setText(QString("%1").arg(rxdat[14], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD16_TF3_2
                   ui->lb_CD16_TF3_2->setText(QString("%1").arg(rxdat[15], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD17_TF4_1
                   ui->lb_CD17_TF4_1->setText(QString("%1").arg(rxdat[16], 4, 16, QLatin1Char( '0' )).toUpper());
                   // CD18_TF4_2
                   ui->lb_CD18_TF4_2->setText(QString("%1").arg(rxdat[17], 4, 16, QLatin1Char( '0' )).toUpper());



               }
               if(sadrOU == 20){
                   memset(&DAC_R_20PA,         0, sizeof(DAC_R_20PA)); // Очистка пямяти
                   memcpy(&DAC_R_20PA, &rxdat[0], sizeof(DAC_R_20PA)); // Копирование данных

                   ui->label_lins_1_1->setText(QString::number((DAC_R_20PA[0][0] * 2500) / 4095));   // Канал 0 полоска 0 (0xBFD80200, 0xBFD80204)
                   ui->label_lins_1_2->setText(QString::number((DAC_R_20PA[0][1] * 2500) / 4095));   // Канал 0 полоска 1 (0xBFD80208, 0xBFD8020С)
                   ui->label_lins_1_3->setText(QString::number((DAC_R_20PA[0][2] * 2500) / 4095));   // Канал 0 полоска 2 (0xBFD80210, 0xBFD80214)
                   ui->label_lins_1_4->setText(QString::number((DAC_R_20PA[0][3] * 2500) / 4095));   // Канал 0 полоска 3 (0xBFD80218, 0xBFD8021С)
                   ui->label_lins_1_5->setText(QString::number((DAC_R_20PA[0][4] * 2500) / 4095));   // Канал 0 полоска 4 (0xBFD80220, 0xBFD80224)
                   ui->label_lins_1_6->setText(QString::number((DAC_R_20PA[0][5] * 2500) / 4095));   // Канал 0 полоска 5 (0xBFD80238, 0xBFD8023С)
                   ui->label_lins_1_7->setText(QString::number((DAC_R_20PA[0][6] * 2500) / 4095));   // Канал 0 полоска 6 (0xBFD80230, 0xBFD80234)
                   ui->label_lins_1_8->setText(QString::number((DAC_R_20PA[0][7] * 2500) / 4095));   // Канал 0 полоска 7 (0xBFD80238, 0xBFD8023С)
                   ui->label_lins_1_9->setText(QString::number((DAC_R_20PA[0][8] * 2500) / 4095));   // Канал 0 полоска 8 (0xBFD80240, 0xBFD80244)
                   ui->label_lins_1_10->setText(QString::number((DAC_R_20PA[0][9] * 2500) / 4095));  // Канал 0 полоска 9 (0xBFD80248, 0xBFD8024С)
                   ui->label_lins_1_11->setText(QString::number((DAC_R_20PA[0][10] * 2500) / 4095)); // Канал 0 полоска 10 (0xBFD80250, 0xBFD80254)
                   ui->label_lins_1_12->setText(QString::number((DAC_R_20PA[0][11] * 2500) / 4095)); // Канал 0 полоска 11 (0xBFD80258, 0xBFD8025С)
                   ui->label_lins_1_13->setText(QString::number((DAC_R_20PA[0][12] * 2500) / 4095)); // Канал 0 полоска 12 (0xBFD80260, 0xBFD80264)
                   ui->label_lins_1_14->setText(QString::number((DAC_R_20PA[0][13] * 2500) / 4095)); // Канал 0 полоска 13 (0xBFD80268, 0xBFD8026С)
                   ui->label_lins_1_15->setText(QString::number((DAC_R_20PA[0][14] * 2500) / 4095)); // Канал 0 полоска 14 (0xBFD80270, 0xBFD80274)
                   ui->label_lins_1_16->setText(QString::number((DAC_R_20PA[0][15] * 2500) / 4095)); // Канал 0 полоска 15 (0xBFD80278, 0xBFD8027С)

                   ui->label_lins_2_1->setText(QString::number((DAC_R_20PA[1][0] * 2500) / 4095));   // Канал 1 полоска 0 (0xBFD80280, 0xBFD80284)
                   ui->label_lins_2_2->setText(QString::number((DAC_R_20PA[1][1] * 2500) / 4095));   // Канал 1 полоска 1 (0xBFD80288, 0xBFD8028С)
                   ui->label_lins_2_3->setText(QString::number((DAC_R_20PA[1][2] * 2500) / 4095));   // Канал 1 полоска 2 (0xBFD80290, 0xBFD80294)
                   ui->label_lins_2_4->setText(QString::number((DAC_R_20PA[1][3] * 2500) / 4095));   // Канал 1 полоска 3 (0xBFD80298, 0xBFD8029С)
                   ui->label_lins_2_5->setText(QString::number((DAC_R_20PA[1][4] * 2500) / 4095));   // Канал 1 полоска 4 (0xBFD802A0, 0xBFD802A4)
                   ui->label_lins_2_6->setText(QString::number((DAC_R_20PA[1][5] * 2500) / 4095));   // Канал 1 полоска 5 (0xBFD802A8, 0xBFD802AС)
                   ui->label_lins_2_7->setText(QString::number((DAC_R_20PA[1][6] * 2500) / 4095));   // Канал 1 полоска 6 (0xBFD802B0, 0xBFD802B4)
                   ui->label_lins_2_8->setText(QString::number((DAC_R_20PA[1][7] * 2500) / 4095));   // Канал 1 полоска 7 (0xBFD802B8, 0xBFD802BС)
                   ui->label_lins_2_9->setText(QString::number((DAC_R_20PA[1][8] * 2500) / 4095));   // Канал 1 полоска 8 (0xBFD802C0, 0xBFD802C4)
                   ui->label_lins_2_10->setText(QString::number((DAC_R_20PA[1][9] * 2500) / 4095));  // Канал 1 полоска 9 (0xBFD802C8, 0xBFD802CС)
                   ui->label_lins_2_11->setText(QString::number((DAC_R_20PA[1][10] * 2500) / 4095)); // Канал 1 полоска 10 (0xBFD802D0, 0xBFD802D4)
                   ui->label_lins_2_12->setText(QString::number((DAC_R_20PA[1][11] * 2500) / 4095)); // Канал 1 полоска 11 (0xBFD802D8, 0xBFD802DС)
                   ui->label_lins_2_13->setText(QString::number((DAC_R_20PA[1][12] * 2500) / 4095)); // Канал 1 полоска 12 (0xBFD802E0, 0xBFD802E4)
                   ui->label_lins_2_14->setText(QString::number((DAC_R_20PA[1][13] * 2500) / 4095)); // Канал 1 полоска 13 (0xBFD802E8, 0xBFD802EС)
                   ui->label_lins_2_15->setText(QString::number((DAC_R_20PA[1][14] * 2500) / 4095)); // Канал 1 полоска 14 (0xBFD802F0, 0xBFD802F4)
                   ui->label_lins_2_16->setText(QString::number((DAC_R_20PA[1][15] * 2500) / 4095)); // Канал 1 полоска 15 (0xBFD802F8, 0xBFD802FС)

                   qDebug();

               }
               if(sadrOU == 30){
                   for(i = 2; i < 32; i++)
                   {
                       ui->tableDATA->item(i-1,2)->setText("");
                       ui->tableDATA->item(i-1,2)->setText(QString(" %1").arg(pcBuf[i], 4, 16, QLatin1Char( '0' )).toUpper());
                   }
               }

           }
        }
        else
        {
           Print("Принято ОС, ОУ приняло данные");
           emit OU_accepted();
        }
    }  catch (const std::exception &e) {
        qDebug() << "Exception:" << e.what();
    }
}



// CRC16 calculation function
unsigned short MainWindow::Calc_CRC16(uchar  *pBlk, unsigned int len,unsigned short *pTab16)
{
   unsigned int   n = len;
   unsigned short crc16 = 0xFFFF;

   if ( !len ) return 0;

   if ( pTab16 == 0 ) pTab16 = (unsigned short *)TableCRC16;

   do
   {
      crc16 = pTab16[(crc16 ^ *pBlk++) & 0xFF] ^ (crc16 >> 8);
   } while ( --n );

   return ( crc16 );
}


// кнопка вкл/откл линии Б МКО
void MainWindow::on_pBut_MKO_B1_toggled(bool checked)
{
    if (checked)
    {
        if (ui->pBut_MKO_A1->isChecked()) { ui->pBut_MKO_A1->setChecked(false);;}

        bcdefbus(BUS_2);

        Print("Выбран канал Б");
        //ui->pBut_MKO_B1->setDown(true);
    }
}

// кнопка вкл/откл линии А МКО
void MainWindow::on_pBut_MKO_A1_toggled(bool checked)
{
    if (checked)
    {
        if (ui->pBut_MKO_B1->isChecked()) {
            ui->pBut_MKO_B1->setChecked(false);
        }

        bcdefbus(BUS_1);

        Print("Выбран канал А");
    }
}

//// прочитать события
void MainWindow::read_EVT(int sadrEVT)
{
   // readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
   // База, Адрес, Подадрес, кол-во слов;
   // Адреса БРГА1: 6, 14;
   int num = ui->comboBox->currentText().toInt();;
   mko.readFromRT(1,num,sadrEVT,32);

}

// Кнопка "Прочитать ИКС"
void MainWindow::on_pBut_read_IKS_clicked()
{

    // readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
    // База, Адрес, Подадрес, кол-во слов;
    // Адреса БРГА1: 6, 14;
    int num = ui->comboBox->currentText().toInt();
    mko.readFromRT(1,num,1,32);
}

void MainWindow::on_pushButton_clicked()
{
    // readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
    // База, Адрес, Подадрес, кол-во слов;
    // Адреса БРГА1: 6, 14;
    int num = ui->comboBox->currentText().toInt();
    mko.readFromRT(1,num,20,32);

}


// Обработчик квитанций. Состояние регистров ПЛИС отображаются во вкладке настройки;
void MainWindow::transaction_handler(QString data){

    // Код принятой команды;
    unsigned char   cod      =   data.at(0).toLatin1();
    // Cодержание;
    unsigned char   value    =   data.mid(1,data.size()).toInt();

    switch (cod)
    {

    case 'p':
        {
            emit signal_p();

        } break;

    case 'k':
        {
            emit signal_k();

        } break;

    case 'd':
        {
            emit signal_d();

        } break;

    case 'c':
        {
            emit signal_c();

        } break;

    case 'r':
        {
            status_ARD = value;
            emit signal_r();
            (value & 0x01) ? ui->label_debug_37->setText("1") : ui->label_debug_37->setText("0");
            (value & 0x02) ? ui->label_debug_36->setText("1") : ui->label_debug_36->setText("0");
            (value & 0x04) ? ui->label_debug_35->setText("1") : ui->label_debug_35->setText("0");
            (value & 0x08) ? ui->label_debug_34->setText("1") : ui->label_debug_34->setText("0");
            (value & 0x10) ? ui->label_debug_33->setText("1") : ui->label_debug_33->setText("0");
            (value & 0x20) ? ui->label_debug_32->setText("1") : ui->label_debug_32->setText("0");
            (value & 0x40) ? ui->label_debug_31->setText("1") : ui->label_debug_31->setText("0");
            (value & 0x80) ? ui->label_debug_30->setText("1") : ui->label_debug_30->setText("0");

        } break;

//    case 'x':f

//       {
//           flashes1HZ();

//       } break;

     case 'w':
        {
            PrintERR("Ошибка 1Гц");

        } break;
    }

}

//+++++++++++++[Процедура вывода данных в консоль]++++++++++++++++++++++++++++++++++++++++
void MainWindow::PrintMiliSec(QString data)
{
    //QString messange = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + " >> " + data;
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    QString messange = QString::number(time) + " >> " + data;
    QTextCharFormat fmt = ui->consol->currentCharFormat();
    fmt.setForeground(QBrush(Qt::black));
    ui->consol->setCurrentCharFormat(fmt);
    ui->consol->textCursor().insertText(messange+'\r'); // Вывод текста в консоль
    ui->consol->moveCursor(QTextCursor::End);//Scroll
}



// При инициализации источника питания обновляет поле Vset БС
void MainWindow::CheckVset1(QString data){
    ui->doubleSpinBox_6->setValue(data.toFloat());
}

// При инициализации источника питания обновляет поле Iset БС
void MainWindow::CheckIset1(QString data){
    ui->doubleSpinBox_12->setValue(data.toFloat());
}

// Команда "LOAD". Код КУ 1.
void MainWindow::on_pBut_LOAD_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 1 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = ui->comboBox_3->currentText().toUInt(nullptr, 16) & 0xFFFF;

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();

    }

}

// Команда "COPY". Код КУ 2;
void MainWindow::on_pBut_COPY_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 2 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = ui->comboBox_4->currentText().toUInt(nullptr, 16) & 0xFFFF;
        pdat[3] = 0;
        pdat[4] = ui->comboBox_7->currentText().toUInt(nullptr, 16) & 0xFFFF;

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
}

// Команда "READ". Код КУ 3;
void MainWindow::on_pBut_READ_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 3 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = ui->lEdit_ADR->text().toUInt(nullptr,16) & 0xFFFF; // ADDR ст.
        pdat[3] = (ui->lEdit_ADR->text().toUInt(nullptr,16) >> 16) & 0xFFFF; // ADDR мл.
        pdat[4] = ui->lEdit_LEN->text().toUInt(nullptr,16) & 0xFFFF; // LEN мл.

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        // подаем команду на подготовку данных
        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

        // потом читаем данные по адресу 25
        QThread::msleep(100);
        mko.readFromRT(1,num,25,32);

    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Команда "READ" с параметром. Код КУ 3;
void MainWindow::on_pBut_READ_clicked(unsigned int addr, ushort len)
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 3 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = addr & 0xFFFF; // ADDR ст.
        pdat[3] = (addr >> 16) & 0xFFFF; // ADDR мл.
        pdat[4] = len & 0xFFFF; // LEN мл.

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        // подаем команду на подготовку данных
        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

        // потом читаем данные по адресу 25
        QThread::msleep(100);
        mko.readFromRT(1,num,25,32);

    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Команда "WRITE". Код КУ 4;
void MainWindow::on_pBut_WRITE_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 4 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = ui->lEdit_ADR->text().toUInt(nullptr,16) & 0xFFFF; // ADDR ст.
        pdat[3] =  (ui->lEdit_ADR->text().toUInt(nullptr,16) >> 16) & 0xFFFF; // ADDR мл.
        pdat[4] = ui->lEdit_LEN->text().toUInt(nullptr,16) & 0xFFFF; // LEN мл.

        int j = 1;
        if (pdat[4] > 12) pdat[4] = 12;
        for(int i=1; i <= pdat[4]; i ++)
        {
            ushort data1 =  (ui->tableDATA->item(i,0)->text().toUInt(nullptr,16)) & 0xFFFF; // DATA ст.
            ushort data2 =  (ui->tableDATA->item(i,0)->text().toUInt(nullptr,16) >> 16) & 0xFFFF;// DATA мл

            pdat[j+5] = data1;
            pdat[j+6] = data2;
            j += 2;

        }

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}


// Команда "Запуск измерений". Код КУ 5
void MainWindow::on_pBut_RUN_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 5 & 0xFFFF;
        pdat[1] = 0;

        // Текущая дата;
        QDateTime currentDateTime = QDateTime::currentDateTime();
        // Дата: 00:00:00 1 января 2000 года;
        QDateTime startDate(QDate(2000,1,1),QTime(00,00,00));
        // Количество секунд с 00:00:00 1 января 2000 года;
        uint32_t dataTimeSec = startDate.secsTo(currentDateTime);
        pdat[2] = dataTimeSec & 0xFFFF;// мл.
        pdat[3] = (dataTimeSec >> 16) & 0xFFFF;// ст.

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Команда "Остановка измерений". Код КУ 6.
void MainWindow::on_pBut_STOP_clicked()
{
    try {
        preparingArrForTransmit(6);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
}

// Кнопка ФК. Код КУ 7;
void MainWindow::on_pBut_FC_2_clicked()
{
    try {
        ushort pdat[32];

        memset(pdat,     0, sizeof(pdat)); // Clear memory
        memcpy(&pdat[1], &uFC_s, sizeof(uFC_s)); // Copy data to structure

        pdat[0] = 7 & 0xFFFF;
        pdat[1] = 0;

        uint32_t t0 = ui->lEdit_t0->text().toUInt(nullptr,16);
        pdat[2] = t0 & 0xFFFF;
        pdat[3] = (t0 >> 16) & 0xFFFF;

        char type = ui->comboBox_type->currentIndex();
        switch(type)
        {
            case 0:
                pdat[4] = 0x3;
                break;
            case 1:
                pdat[4] = 0x30;
                break;
            case 2:
                pdat[4] = 33;
                break;
        }
        pdat[5] = 0;

        uint32_t tau = ui->lEdit_tau->text().toUInt(nullptr,16);
        pdat[6] = tau & 0xFFFF;
        pdat[7] = (tau >> 16) & 0xFFFF;


        preparingArrForTransmitWithData(pdat);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);


        // Запрос квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
}

// Кнопка очистить. Очищает поля таблицы.
void MainWindow::on_pushButton_7_clicked()
{
    ui->tableEvents->clear();
}

// Кнопка сохранить. Сохраняет таблицу "Измерительная информация" в формате csv и txt
void MainWindow::on_pushButton_6_clicked()
{
    QString textData;
    int rows = ui->tableEvents->rowCount();
    int columns = ui->tableEvents->columnCount();

    for (int i = 0; i < rows - 1; i++) {
        for (int j = 0; j < columns - 1; j++) {
                textData += ui->tableEvents->item(i,j)->text();
                textData += ", ";      // для .csv
        }
        textData += "\n";             //
    }

    // .csv
    try {
        QFile csvFile("test.csv");
        if(csvFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {

            QTextStream out(&csvFile);
            out << textData;

            csvFile.close();
        }
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }


    // .txt
    try {
        QFile txtFile("test.txt");
        if(txtFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {

            QTextStream out(&txtFile);
            out << textData;

            txtFile.close();
        }
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
    Print("Сохранение прошло успешно");
}

// Кнопка GET_ULA. Код КУ 11
void MainWindow::on_pBut_GET_ULA_clicked()
{
    try {
        preparingArrForTransmit(11);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        // подаем команду на подготовку данных
        mko.writeToRT(1,num,1,32,pcBuf);

        // Запрос квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

        // читаем данные по адресу 20
//        QThread::msleep(100);
//        mko.readFromRT(1,num,20,32);

        // потом читаем данные по адресу 27
        QThread::msleep(100);
        mko.readFromRT(1,num,27,32);


    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Кнопка SET_ULA. Код КУ 10
void MainWindow::on_pBut_SET_ULA_clicked()
{
//    unsigned short DAC_S[2][16];
//    memset(DAC_S,     0, sizeof(DAC_S)); // Clear memory

//    DAC_S[0][0] = ui->lens1line1->text().toUShort();
//    DAC_S[0][1] = ui->lens1line2->text().toUShort();
//    DAC_S[0][2] = ui->lens1line3->text().toUShort();
//    DAC_S[0][3] = ui->lens1line4->text().toUShort();
//    DAC_S[0][4] = ui->lens1line5->text().toUShort();
//    DAC_S[0][5] = ui->lens1line6->text().toUShort();
//    DAC_S[0][6] = ui->lens1line7->text().toUShort();
//    DAC_S[0][7] = ui->lens1line8->text().toUShort();
//    DAC_S[0][8] = ui->lens1line9->text().toUShort();
//    DAC_S[0][9] = ui->lens1line10->text().toUShort();
//    DAC_S[0][10] = ui->lens1line11->text().toUShort();
//    DAC_S[0][11] = ui->lens1line12->text().toUShort();
//    DAC_S[0][12] = ui->lens1line13->text().toUShort();
//    DAC_S[0][13] = ui->lens1line14->text().toUShort();
//    DAC_S[0][14] = ui->lens1line15->text().toUShort();
//    DAC_S[0][15] = ui->lens1line16->text().toUShort();

//    DAC_S[1][0] = ui->lens2line1->text().toUShort();
//    DAC_S[1][1] = ui->lens2line2->text().toUShort();
//    DAC_S[1][2] = ui->lens2line3->text().toUShort();
//    DAC_S[1][3] = ui->lens2line4->text().toUShort();
//    DAC_S[1][4] = ui->lens2line5->text().toUShort();
//    DAC_S[1][5] = ui->lens2line6->text().toUShort();
//    DAC_S[1][6] = ui->lens2line7->text().toUShort();
//    DAC_S[1][7] = ui->lens2line8->text().toUShort();
//    DAC_S[1][8] = ui->lens2line9->text().toUShort();
//    DAC_S[1][9] = ui->lens2line10->text().toUShort();
//    DAC_S[1][10] = ui->lens2line11->text().toUShort();
//    DAC_S[1][11] = ui->lens2line12->text().toUShort();
//    DAC_S[1][12] = ui->lens2line13->text().toUShort();
//    DAC_S[1][13] = ui->lens2line14->text().toUShort();
//    DAC_S[1][14] = ui->lens2line15->text().toUShort();
//    DAC_S[1][15] = ui->lens2line16->text().toUShort();

//    WORD DAC_CRC = Calc_CRC16((unsigned char *)DAC_S,62,0);

    ushort pdat[32];

    // СД2[7:0] REG_UPR[7:0]
    uula_g_s.r.R_UPR = ui->lEdit_REG_UPR->text().toUInt(nullptr,16);

    // СД2[15:8] REG_KOM[7:0]
    uula_g_s.r.R_KOM = ui->lEdit_REG_KOM->text().toUInt(nullptr,16);

    // СД3[7:0] REG_LENGHT_FC[7:0]
    uula_g_s.r.T_COR = ui->lEdit_REG_COR->text().toUInt(nullptr,16);

    // СД3[15:8] REG_DAC_EN[7:0]
    uula_g_s.r.T_DAC_EN = ui->lEdit_REG_DAC_EN->text().toUInt(nullptr,16);

    // СД4[15:0] REG_TIME_FC[15:0]
    uint32_t time_FC = ui->lEdit_REG_TIME_FC->text().toUInt(nullptr,16);
    uula_g_s.r.T_FC[0] = time_FC & 0xFF;
    uula_g_s.r.T_FC[1] = (time_FC >> 8) & 0xFF;
    // СД5[7:0] REG_TIME_FC [23:16]
    uula_g_s.r.T_FC[2] = (time_FC >> 16) & 0xFF;

    // СД5[15:8] Length_FС[7:0]
    uula_g_s.r.Length_FC= ui->lEdit_REG_LENGHT_FC->text().toUInt(nullptr,16);

    // СД6[15:0] REG_SUMM_HZ[15:0]
    uint32_t summ_HZ = ui->lEdit_REG_SUMM_HZ->text().toUInt(nullptr,16);
    uula_g_s.r.R_SUMM_HZ[0] = summ_HZ & 0xFF;
    uula_g_s.r.R_SUMM_HZ[1] = (summ_HZ >> 8) & 0xFF;
    // СД7[7:0] REG_SUMM_HZ [23:16]
    uula_g_s.r.R_SUMM_HZ[2] = (summ_HZ >> 16) & 0xFF;

    // СД7[15:8] reserv 0
    uula_g_s.r.empty = 0;

    // СД8[15:0] REG_SUMM_HZ[15:0]
    uint32_t REG_HZ = ui->lb_REG_HZ->text().toUInt(nullptr,16);
    uula_g_s.r.REG_HZ[0] = REG_HZ & 0xFF;
    uula_g_s.r.REG_HZ[1] = (REG_HZ >> 8) & 0xFF;
    // СД9[7:0] REG_SUMM_HZ [23:16]
    uula_g_s.r.REG_HZ[2] = (REG_HZ >> 16) & 0xFF;

    // СД9[15:8] D_TERMO[7:0]
    uula_g_s.r.D_TERMO = ui->lb_REG_TERMO->text().toUInt(nullptr,16);

    // СД10[15:0] DAC_CRC
    uula_g_s.r.DAC_CRC = 0;

    // СД11[15:0] DAC_CRC_0
    uula_g_s.r.DAC_CRC_C = 0;


    memset(pdat,     0, sizeof(pdat)); // Clear memory
    memcpy(&pdat[2], &uula_g_s, sizeof(uula_g_s)); // Copy data to structure

//    try {
//        preparingArrForTransmitWithDataWithoutCS(&DAC_S[0][0]);
//        toTXline(pcBuf);

//        int num = ui->comboBox->currentText().toInt();
//        mko.writeToRT(1,num,20,32,pcBuf);

//        // Запрос квитанции
//        QTimer::singleShot(100,this, [this](){
//            ui->pBut_read_Kvit->click();
//        });

//    }  catch (const std::exception &e) {
//        qDebug()<< "Exception:" << e.what();
//    }

    pdat[0] = 10 & 0xFFFF;
    pdat[1] = 0;

    try {
        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        // Запрос квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
}

// Кнопка RESET. Код КУ 8
void MainWindow::on_pBut_RESET_clicked()
{
    try {
        preparingArrForTransmit(8);
        toTXline(pcBuf);

        // Забираю значение текущего Адреса и пишу в ОУ;
        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Кнопка GO_TO. Код КУ 9
void MainWindow::on_pBut_GO_TO_clicked()
{    
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 9 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = ui->lEdit_ADR->text().toUInt(nullptr,16) & 0xFFFF; // ADDR ст.
        pdat[3] = (ui->lEdit_ADR->text().toUInt(nullptr,16) >> 16) & 0xFFFF; // ADDR мл.

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        // подаем команду на подготовку данных
        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

        // потом читаем данные по адресу 25
        QThread::msleep(100);
        mko.readFromRT(1,num,25,32);
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
}

// Кнопка CORR_DIS. Код КУ 14
void MainWindow::on_pBut_CORR_DIS_clicked()
{   
    try {

        preparingArrForTransmit(14);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }


}

// Кнопка CORR_EN. Код КУ 15
void MainWindow::on_pBut_CORR_EN_clicked()
{
    try {
        preparingArrForTransmit(15);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }


}

// Кнопка UART_OFF. Код КУ 16
void MainWindow::on_pBut_UART_OFF_clicked()
{

    try {
        preparingArrForTransmit(16);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }


}

// Кнопка UART_ON. Код КУ 17
void MainWindow::on_pBut_UART_ON_clicked()
{
    try {
        preparingArrForTransmit(17);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }


}

// Кнопка DIAG. Код КУ 13
void MainWindow::on_pBut_SELF_DIAG_clicked()
{
    try {
        preparingArrForTransmit(0xD);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }
}

// Кнопка ERRASE. Код КУ 18
void MainWindow::on_pBut_ERRASE_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 12 & 0xFF;
        pdat[1] = 0;
        pdat[2] = ui->comboBox_BANK->currentText().toUInt(nullptr, 16) & 0xFFFF;
        pdat[3] = 0;

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Кнопка DIAG. Код КУ 12
void MainWindow::on_pBut_FIRMWARE_clicked()
{
    try {
        preparingArrForTransmit(0xC);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Сортировка событий по t0;
void MainWindow::on_cBoxSort_activated(int index)
{
    if(index == 0){
        ui->tableEvents->sortItems(2, Qt::AscendingOrder);
    } else {
        ui->tableEvents->sortItems(2, Qt::AscendingOrder);
    }
}


void MainWindow::preparingArrForTransmit(int num){

    ushort pdat[32];
    memset(pdat, 0, sizeof(pdat));
    pdat[0] = num & 0xFFFF;

    // подготовка массива на передачу и расчет CRC16
    memset(pcBuf, 0, sizeof(pcBuf));
    memcpy(pcBuf,pdat,pDWC);
    pcBuf[31] = Calc_CRC16((unsigned char *)pdat,62,0);

}


void MainWindow::preparingArrForTransmitWithData(ushort *pData){

    ushort pdat[32];
    memset(pdat, 0, sizeof(pdat));
    memcpy(pdat,pData,62);

    // подготовка массива на передачу и расчет CRC16
    memset(pcBuf, 0, sizeof(pcBuf));
    memcpy(pcBuf,pdat,62);
    pcBuf[31] = Calc_CRC16((unsigned char *)pdat,62,0);

}

void MainWindow::preparingArrForTransmitWithDataWithoutCS(ushort *pData){

    ushort dat[32];
    memset(dat, 0, sizeof(dat));
    memcpy(dat,pData,64);

    // подготовка массива на передачу
    memset(pcBuf, 0, sizeof(pcBuf));
    memcpy(pcBuf,dat,64);
    qDebug();
}

// Чтение квитанции
void MainWindow::read_Kvit()
{
    //QThread::sleep(100);
    // readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
    // База, Адрес, Подадрес, кол-во слов;
    // Адреса БРИИ: 1, 9, 2, 10, 3, 11, 4, 12;
    int num = ui->comboBox->currentText().toInt();
    mko.readFromRT(1,num,28,32);
}


// Кнопка "Чтение квитанции"
void MainWindow::on_pBut_read_Kvit_clicked()
{
    // readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
    // База, Адрес, Подадрес, кол-во слов;
    // Адреса БРИИ: 1, 9, 2, 10, 3, 11, 4, 12;
    int num = ui->comboBox->currentText().toInt();
    mko.readFromRT(1,num,28,32);
}


void MainWindow::on_open_File_IKPMO_clicked()
{
    file_name_IKPMO = QFileDialog::getOpenFileName();
    ui->fileNameIKPMO->setText(file_name_IKPMO);

    QFile file(file_name_IKPMO);
    ushort pdat[32];
    memset(pdat, 0, sizeof(pdat));
    int i = 0;
    int j = 0;

    if(!(QFile::exists(file_name_IKPMO)))
    {
        qDebug() << "Файла не существует";
    }

    try {
        if(file.open(QIODevice::ReadOnly))
            {
            while (!file.atEnd())
                {
                    IKPMO = file.readAll();
                }

            } file.close();
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
        file.close();
    }

    if(IKPMO.isNull()){
        PrintERR("Ошибка при загрузки файла ИКПМО");
    } else {
        while(j < IKPMO.size()){
            while(i < 32){

                ushort a = (IKPMO[j + 1] << 8) & 0xFF00;
                ushort b = IKPMO[j] & 0x00FF;
                ushort c = a | b;
                pdat[i] = ( c );
                i++;
                j += 2;
            }

            if(pdat[30] == 1){
                ui->PQ->setText(QString("%1").arg( (pdat[1] << 16) | pdat[0] , 4, 10, QLatin1Char( '0' )).toUpper());            }
            memset(pdat, 0, sizeof(pdat));
            break;
         }
    }
    ui->progressBar->setValue(0);
}

void MainWindow::on_open_File_IKPMO_2_clicked()
{
    Print("Старт загрузки ИКПМО;");
    ushort pdat[32];  
    memset(pdat, 0, sizeof(pdat));
    int i = 0;
    int j = 0;   
    int PQ = ui->PQ->text().toInt();
    bool verification_state = false;

    if(IKPMO.isNull()){
        PrintERR("Ошибка при загрузки файла ИКПМО");
    } else {
        float progressBar_devision = 0;
        while(j < IKPMO.size()){

            while(i < 32){

                ushort a = (IKPMO[j + 1] << 8) & 0xFF00;
                ushort b = IKPMO[j] & 0x00FF;
                ushort c = a | b;
                pdat[i] = ( c );
                i++;
                j += 2;
            }

            if(pdat[30] > 0){
                preparingArrForTransmitWithDataWithoutCS(pdat);
                toTXline(pcBuf);

                //QThread::msleep(100);
                // подаем команду на подготовку данных
                int num = ui->comboBox->currentText().toInt();
                mko.writeToRT(1,num,14,32,pcBuf);


                // OU_accepted()
                QTimer::singleShot(50,this, [this](){
                    ui->pBut_read_Kvit->click();
                });

                uint16_t pn_c = pcBuf[30];

                QEventLoop loop;
                QTimer qtimer;
                qtimer.setSingleShot(true);
                qtimer.start(200);

                // Проверить выполнение
//                connect(this, &MainWindow::KAS_ready, this, [&, pn_c](bool verification_state) mutable{
//                    verification_state = check_IKPMO(pn_c);
//                    emit IKPMO_sign();
//                });

                connect(this, &MainWindow::KAS_ready, this, [&](){
                                    verification_state = check_IKPMO(pn_c);
                                    emit IKPMO_sign();
                                });

                connect(&qtimer, &QTimer::timeout, this,  [&loop, this]() {
                            PrintERR("Ошибка таймаута: превышено время ожидания квитанции KAS;");
                            loop.quit();
                    });
                QObject::connect(this, SIGNAL (IKPMO_sign()), &loop, SLOT (quit()));
                loop.exec();
                qtimer.stop();

                if(verification_state){

                    ui->progressBar->setValue(progressBar_devision);
                    progressBar_devision += 0.05;
                    //ukas_g.r.pn_c += 1;
                    if(pn_c == PQ - 1) {
                        ui->progressBar->setValue(100);
                        Print("Загрузка ИКПМО завершена, всего загружено пакетов: " + QString::number(pn_c + 1));
                    }

                } else {
                    if(pn_c != PQ){
                        Print(QString("Ошибка при загрузки ИКПМО: пакет %1, КАС: %2").arg(QString::number(ukas_g.r.pn_c)).arg(ukas_g.r.kas));
                        ui->progressBar->setValue(0);
                    }
                    break;
                }

                i = 0;
                memset(pdat, 0, sizeof(pdat));

            }
         }

    }

}

bool MainWindow::check_IKPMO(int pn_c){

    if(ukas_g.r.kas == 0){
        if(ukas_g.r.pn_c == pn_c){
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }

}


void MainWindow::flashes1HZ(){

    state_1hz = !state_1hz;

    if(state_1hz)   ui->label_33->setStyleSheet("QLabel { background-color: #50C878; border: 1px solid black; border-radius: 12px;}");
    else            ui->label_33->setStyleSheet("QLabel { background-color: #DC143C; border: 1px solid black; border-radius: 12px;}");

    bool sec_IKS = ui->chBox_read_IKS->checkState();
    bool sec_ULA = ui->chBox_read_ULA->checkState();

    if(sec_IKS){

        QTimer::singleShot(100,this, [this](){
            QMetaObject::invokeMethod(ui->pBut_read_IKS, "clicked");
        });
        // QThread::msleep(100);
        // ui->pBut_read_IKS->clicked();
        Print("sec_IKS");
    }
    if(sec_ULA){
        QTimer::singleShot(200,this, [this](){
            QMetaObject::invokeMethod(ui->pBut_GET_ULA, "clicked");
        });
        //QThread::msleep(100);
        //ui->pBut_GET_ULA->clicked();
        Print("ula");
    }
}

// Кнопка "Циркуляционный обмен"
void MainWindow::on_circulating_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 30 & 0xFFFF;
        for(int i = 1; i < 32; i++){
            pdat[i] = i & 0xFFFF;
            ui->tableDATA->item(i,1)->setText("");
            ui->tableDATA->item(i,1)->setText(QString(" %1").arg(pdat[i], 4, 16, QLatin1Char( '0' )).toUpper());
        }

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,30,32,pcBuf);
        // Запрос Квитанции

        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });

        // Чтение 30 ПА
        mko.readFromRT(1,num,30,32);


    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}

// Кнопка включения БРГА1_А
void MainWindow::on_pBut_BRGA1_A_clicked(bool checked)
{
    if(checked){

        if(ui->pBut_BRGA1_B->isChecked()){
            ui->pBut_BRGA1_B->setChecked(false);
        }

        if(ui->pBut_BRGA1h->isChecked()){
            ui->pBut_BRGA1h->setChecked(false);
        }

        // Включаю первый полукомплект
        emit writeData(QString("K1\n").toUtf8());
        // Ожидаю квитанцию "k", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_k)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции k;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции k;");
            };

        // Запрашиваю квитанцию "R"
        emit writeData(QString("R\n").toUtf8());
        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(status_ARD & 0x02 && !(status_ARD & 0x04)){
            // Проверяем ON_OFF на ноль
            if(!(status_ARD & 0x01)) {
                // Включаю питание прибора;
                emit writeData(QString("P1\n").toUtf8());
                if(waitingFor(&MainWindow::signal_p)){
                        PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                        qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
                    };

                // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
                emit writeData(QString("R\n").toUtf8());
                if(waitingFor(&MainWindow::signal_r)){
                        PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                        qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                    };

                if(!(status_ARD & 0x01)) {
                    ui->pBut_BRGA1_A->setChecked(false);
                    PrintERR("Ошибка при включении БРГА1_А;");
                } else Print("БРГА1_А включено;");
            } else Print("БРГА1_А включено;");

        } else {
            ui->pBut_BRGA1_A->setChecked(false);
            PrintERR("Ошибка при включении БРГА1_А;");
        }
    } else {
        // Выключаю питание прибора;
        emit writeData(QString("P0\n").toUtf8());
        if(waitingFor(&MainWindow::signal_p)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
            };

        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        emit writeData(QString("R\n").toUtf8());
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(status_ARD & 0x01) {
            ui->pBut_BRGA1_A->setChecked(false);
            PrintERR("Ошибка при выключении БРГА1_А;");
        } else Print("БРГА1_А выключено;");

    }
}

// Кнопка включения БРГА1_Б
void MainWindow::on_pBut_BRGA1_B_clicked(bool checked)
{
    if(checked){

        if(ui->pBut_BRGA1_A->isChecked()){
            ui->pBut_BRGA1_A->setChecked(false);
        }

        if(ui->pBut_BRGA1h->isChecked()){
            ui->pBut_BRGA1h->setChecked(false);
        }

        // Включаю первый полукомплект
        emit writeData(QString("K2\n").toUtf8());
        // Ожидаю квитанцию "k", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_k)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции k;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции k;");
            };

        // Запрашиваю квитанцию "f"
        emit writeData(QString("R\n").toUtf8());

        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };


        if(!(status_ARD & 0x02) && status_ARD & 0x04){
            // Проверяем ON_OFF на ноль
            if(!(status_ARD & 0x01)) {
                // Включаю питание прибора;
                emit writeData(QString("P1\n").toUtf8());
                if(waitingFor(&MainWindow::signal_p)){
                        PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                        qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
                    };


                // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
                emit writeData(QString("R\n").toUtf8());
                if(waitingFor(&MainWindow::signal_r)){
                        PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                        qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                    };

                if(!(status_ARD & 0x01)) {
                    ui->pBut_BRGA1_B->setChecked(false);
                    PrintERR("Ошибка при включении БРГА1_B;");
                } else Print("БРГА1_B включено;");
            } else Print("БРГА1_B включено;");

        } else {
            ui->pBut_BRGA1_B->setChecked(false);
            PrintERR("Ошибка при включении БРГА1_B;");
        }
        } else {

            // Выключаю питание прибора;
            emit writeData(QString("P0\n").toUtf8());
            // Ожидаю квитанцию "p", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_p)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
                };

            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            emit writeData(QString("R\n").toUtf8());
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

            if(status_ARD & 0x01) {
                ui->pBut_BRGA1_B->setChecked(false);
                PrintERR("Ошибка при выключении БРГА1_B;");
            } else Print("БРГА1_А выключено;");

        }
}

// Кнопка включения обогрева
void MainWindow::on_pBut_BRGA1h_clicked(bool checked)
{
    if(checked){

        if(ui->pBut_BRGA1_A->isChecked()){
            ui->pBut_BRGA1_A->setChecked(false);
        }

        if(ui->pBut_BRGA1_B->isChecked()){
            ui->pBut_BRGA1_B->setChecked(false);
        }

        // Включаю первый полукомплект
        emit writeData(QString("K0\n").toUtf8());

        // Ожидаю квитанцию "k", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_k)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции k;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции k;");
            };

        emit writeData(QString("R\n").toUtf8());
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(!(status_ARD & 0x02) && !(status_ARD & 0x04)){
            // Проверяем ON_OFF на ноль
            if(!(status_ARD & 0x01)) {
                // Включаю питание прибора;
                emit writeData(QString("P1\n").toUtf8());
                if(waitingFor(&MainWindow::signal_p)){
                        PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                        qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
                    };


                // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
                emit writeData(QString("R\n").toUtf8());
                if(waitingFor(&MainWindow::signal_r)){
                        PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                        qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                    };

                if(!(status_ARD & 0x01)) {
                    ui->pBut_BRGA1h->setChecked(false);
                    PrintERR("Ошибка при включении обогрева;");
                } else Print("Обогрев включен;");
            } else Print("Обогрев включен;");

        } else {
            ui->pBut_BRGA1h->setChecked(false);
            PrintERR("Ошибка при включении обогрева;");
        }
        } else {
            // Выключаю питание прибора;
            emit writeData(QString("P0\n").toUtf8());
            // Ожидаю квитанцию "p", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_p)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции p;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции p;");
            };


            emit writeData(QString("R\n").toUtf8());
            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

            if(status_ARD & 0x01) {
                ui->pBut_BRGA1h->setChecked(false);
                PrintERR("Ошибка при включении обогрева;");
            } else Print("Обогрев выключен;");
        }

}

// Кнопка включения 1 Гц А
void MainWindow::on_pBut_1Hz_A_clicked(bool checked)
{

    if(checked){
        if(ui->pBut_1Hz_B->isChecked()){
            // Включаю 1Гц А и 1Гц Б
            emit writeData(QString("C3\n").toUtf8());

            // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_c)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
                };


            // Запрашиваю квитанцию "f"
            emit writeData(QString("R\n").toUtf8());

            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

            if(!(status_ARD & 0x10) || !(status_ARD & 0x08)) {
                ui->pBut_1Hz_A->setChecked(false);
                PrintERR("Ошибка при включении 1Гц А;");
            } else Print("1Гц-А и 1Гц-Б включены;");

        } else {

        // Включаю 1Гц А
        emit writeData(QString("C1\n").toUtf8());
        // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_c)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
            };


        // Запрашиваю квитанцию "f"
        emit writeData(QString("R\n").toUtf8());
        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(status_ARD & 0x08) Print("1Гц А включено;");
        else {
            ui->pBut_1Hz_A->setChecked(false);
            PrintERR("Ошибка при включении 1Гц А;");
        }
        }

    } else {
        if(ui->pBut_1Hz_B->isChecked()){
            // Включаю 1Гц Б
            emit writeData(QString("C2\n").toUtf8());

            // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_c)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
                };

            // Запрашиваю квитанцию "R"
            emit writeData(QString("R\n").toUtf8());

            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

            if(status_ARD & 0x10) Print("1Гц А выключено;");
            else {
                ui->pBut_1Hz_A->setChecked(false);
                PrintERR("Ошибка при выключении 1Гц А;");
            }


        } else {

        // Выключаю 1Гц А и 1Гц Б
        emit writeData(QString("C0\n").toUtf8());

        // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_c)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
            };

        // Запрашиваю квитанцию "R"
        emit writeData(QString("R\n").toUtf8());

        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(!(status_ARD & 0x10) || !(status_ARD & 0x08)) {
            ui->pBut_1Hz_A->setChecked(false);
            ui->pBut_1Hz_B->setChecked(false);
            Print("1Гц А выключено;");
        } else PrintERR("Ошибка при выключении 1Гц А;");
        }
    }

}

// Кнопка включения 1 Гц Б
void MainWindow::on_pBut_1Hz_B_clicked(bool checked)
{
    if(checked){
        if(ui->pBut_1Hz_A->isChecked()){
            // Включаю 1Гц А и 1Гц Б
            emit writeData(QString("C3\n").toUtf8());

            // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_c)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
                };

            // Запрашиваю квитанцию "f"
            emit writeData(QString("R\n").toUtf8());

            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

            if(!(status_ARD & 0x10) || !(status_ARD & 0x08)) {
                ui->pBut_1Hz_B->setChecked(false);
                PrintERR("Ошибка при включении 1Гц Б;");
            } else Print("1Гц-А и 1Гц-Б включены;");

        } else {

        // Включаю 1Гц Б
        emit writeData(QString("C2\n").toUtf8());
        // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_c)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
            };

        // Запрашиваю квитанцию "R"
        emit writeData(QString("R\n").toUtf8());
        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(status_ARD & 0x10) Print("1Гц Б включено;");
        else {
            ui->pBut_1Hz_B->setChecked(false);
            PrintERR("Ошибка при включении 1Гц Б;");
        }
        }

    } else {
        if(ui->pBut_1Hz_A->isChecked()){
            // Включаю 1Гц A
            emit writeData(QString("C1\n").toUtf8());
            // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_c)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
                };

            // Запрашиваю квитанцию "R"
            emit writeData(QString("R\n").toUtf8());
            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

            if(status_ARD & 0x08) Print("1Гц Б выключено;");
            else {
                ui->pBut_1Hz_B->setChecked(false);
                PrintERR("Ошибка при выключении 1Гц А;");
            }


        } else {

        // Выключаю 1Гц А и 1Гц Б
        emit writeData(QString("C0\n").toUtf8());
        // Ожидаю квитанцию "c", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_c)){
            PrintERR("Ошибка таймаута: превышено время ожидания квитанции c;");
            qWarning("Ошибка таймаута: превышено время ожидания квитанции c;");
        };

        // Запрашиваю квитанцию "R"
        emit writeData(QString("R\n").toUtf8());
        // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
        if(waitingFor(&MainWindow::signal_r)){
                PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
            };

        if(!(status_ARD & 0x10) || !(status_ARD & 0x08)) {
            ui->pBut_1Hz_A->setChecked(false);
            ui->pBut_1Hz_B->setChecked(false);
            Print("1Гц А выключено;");
        } else PrintERR("Ошибка при выключении 1Гц Б;");
    }
    }
}

// Выбор длительности секундной метки
void MainWindow::on_comboBox_2_activated(int index)
{
    switch (index)
    {

    case 0:
        {
            emit writeData(QString("D0\n").toUtf8());

            // Ожидаю квитанцию "d", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_d)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции d;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции d;");
                };

            emit writeData(QString("R\n").toUtf8());

            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

        } break;

    case 1:
        {
            emit writeData(QString("D1\n").toUtf8());

            // Ожидаю квитанцию "d", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_d)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции d;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции d;");
                };

            emit writeData(QString("R\n").toUtf8());

            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

        } break;

    case 2:
        {
            emit writeData(QString("D3\n").toUtf8());
            // Ожидаю квитанцию "d", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_d)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции d;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции d;");
                };

            emit writeData(QString("R\n").toUtf8());
            // Ожидаю квитанцию "r", если квитанция не приходит loop закрывается с ошибкой;
            if(waitingFor(&MainWindow::signal_r)){
                    PrintERR("Ошибка таймаута: превышено время ожидания квитанции r;");
                    qWarning("Ошибка таймаута: превышено время ожидания квитанции r;");
                };

        } break;


    }
}

void MainWindow::on_pBut_read_PA26_clicked()
{
    // readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
    // База, Адрес, Подадрес, кол-во слов;
    // Адреса БРГА1: 6, 14;
    int num = ui->comboBox->currentText().toInt();
    mko.readFromRT(1,num,26,32);
}


void MainWindow::on_cBtnSend_clicked()
{

}


void MainWindow::on_pBut_read_IKS_2_clicked()
{
    // SW0 Пропадание бортовой секунды;
    ui->led_lost_sec->setPalette(clButtonFace);
    // SW1 Ошибка DSP;
    ui->led__err_DSP->setPalette(clButtonFace);
    // SW2 Неожиданное прерывание;
    ui->led_except_CP->setPalette(clButtonFace);
    // SW3 Неожиданный перезапуск;
    ui->led_restart->setPalette(clButtonFace);
    // SW4 ненорма ОЗУ;
    ui->led_nonnorm_RAM->setPalette(clButtonFace);
    // SW5 ненорма СОЗУ;
    ui->led__nonnorm_sozu->setPalette(clButtonFace);
    // SW6 ошибка CRC32, БАНК1;
    ui->led_err_BANK1->setPalette(clButtonFace);
    // SW7 ошибка CRC32, БАНК2;
    ui->led_err_BANK2->setPalette(clButtonFace);
    // SW8 в БРГА1 не используется 0x0100 fpga_err
    // SW9 FPGA_load 0x0200
    ui->led__fpga_load->setPalette(clButtonFace);
    // SW10 Подключение ТК
    ui->led_on_off_TK->setPalette(clButtonFace);
    // SW11 не используется 0x0800
    // SW12 тип работающего ПО
    ui->led_type_PO->setPalette(clButtonFace);
    // SW13 коррекция времени
    ui->led_correction_on->setPalette(clButtonFace);
    // SW14 норма CRC32 ИКПМО
    ui->led_norm_IKPMO->setPalette(clButtonFace);
    // SW15 Состояние регистрации
    ui->led_registration_on->setPalette(clButtonFace);

}

// Команда "WRITE" c параметрами.
void MainWindow::on_pBut_WRITE_parametr_clicked(unsigned int addr, unsigned char value)
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] = 4 & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = addr & 0xFFFF; // ADDR ст.
        pdat[3] =  (addr >> 16) & 0xFFFF; // ADDR мл.
        pdat[4] = 1 & 0xFFFF; // LEN мл.
        pdat[5] = 0;
        pdat[6] = value;// данные

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}





void MainWindow::on_btn_read_lins1_clicked()
{
    uint16_t lbl;
    dump_PA25_DAC_1 = 1;
    // 8 СД (16 разряд)
    // Канал 0 полоска 0 (0xBFD80200, 0xBFD80204)
    // Канал 0 полоска 1 (0xBFD80208, 0xBFD8020С)
    on_pBut_READ_clicked(0xBFD80200, 8);

    // 8 СД (16 разряд)
    // Канал 0 полоска 2 (0xBFD80210, 0xBFD80214)
    // Канал 0 полоска 3 (0xBFD80218, 0xBFD8021С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready1)){

        ui->lens1line1->setText(QString::number(udac1.r.line1));
        lbl = (udac1.r.line1 * 2500) / 4095;
        ui->label_lins_1_1->setText(QString::number(lbl)); // Канал 0 полоска 0 (0xBFD80200, 0xBFD80204)

        ui->lens1line2->setText(QString::number(udac1.r.line2));
        lbl = (udac1.r.line2 * 2500) / 4095;
        ui->label_lins_1_2->setText(QString::number(lbl)); // Канал 0 полоска 1 (0xBFD80208, 0xBFD8020С)

        dump_PA25_DAC_1 = 2;
        on_pBut_READ_clicked(0xBFD80210, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80200: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 0 полоска 4 (0xBFD80220, 0xBFD80224)
    // Канал 0 полоска 5 (0xBFD80228, 0xBFD8022С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready2)){

        ui->lens1line3->setText(QString::number(udac1.r.line3));
        lbl = (udac1.r.line3 * 2500) / 4095;
        ui->label_lins_1_3->setText(QString::number(lbl)); // Канал 0 полоска 2 (0xBFD80210, 0xBFD80214)

        ui->lens1line4->setText(QString::number(udac1.r.line4));
        lbl = (udac1.r.line4 * 2500) / 4095;
        ui->label_lins_1_4->setText(QString::number(lbl)); // Канал 0 полоска 3 (0xBFD80218, 0xBFD8021С)

        dump_PA25_DAC_1 = 3;
        on_pBut_READ_clicked(0xBFD80220, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80210: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 0 полоска 6 (0xBFD80230, 0xBFD80234)
    // Канал 0 полоска 7 (0xBFD80238, 0xBFD8023С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready3)){

        ui->lens1line5->setText(QString::number(udac1.r.line5));
        lbl = (udac1.r.line5 * 2500) / 4095;
        ui->label_lins_1_5->setText(QString::number(lbl)); // Канал 0 полоска 4 (0xBFD80220, 0xBFD80224)

        ui->lens1line6->setText(QString::number(udac1.r.line6));
        lbl = (udac1.r.line6 * 2500) / 4095;
        ui->label_lins_1_6->setText(QString::number(lbl)); // Канал 0 полоска 5 (0xBFD80238, 0xBFD8023С)

        dump_PA25_DAC_1 = 4;
        on_pBut_READ_clicked(0xBFD80230, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80220: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 0 полоска 8 (0xBFD80240, 0xBFD80244)
    // Канал 0 полоска 9 (0xBFD80248, 0xBFD8024С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready4)){

        ui->lens1line7->setText(QString::number(udac1.r.line7));
        lbl = (udac1.r.line7 * 2500) / 4095;
        ui->label_lins_1_7->setText(QString::number(lbl)); // Канал 0 полоска 6 (0xBFD80230, 0xBFD80234)

        ui->lens1line8->setText(QString::number(udac1.r.line8));
        lbl = (udac1.r.line8 * 2500) / 4095;
        ui->label_lins_1_8->setText(QString::number(lbl)); // Канал 0 полоска 7 (0xBFD80238, 0xBFD8023С)

        dump_PA25_DAC_1 = 5;
        on_pBut_READ_clicked(0xBFD80240, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80230: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 0 полоска 10 (0xBFD80250, 0xBFD80254)
    // Канал 0 полоска 11 (0xBFD80258, 0xBFD8025С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready5)){

        ui->lens1line9->setText(QString::number(udac1.r.line9));
        lbl = (udac1.r.line9 * 2500) / 4095;
        ui->label_lins_1_9->setText(QString::number(lbl)); // Канал 0 полоска 8 (0xBFD80240, 0xBFD80244)

        ui->lens1line10->setText(QString::number(udac1.r.line10));
        lbl = (udac1.r.line10 * 2500) / 4095;
        ui->label_lins_1_10->setText(QString::number(lbl)); // Канал 0 полоска 9 (0xBFD80248, 0xBFD8024С)

        dump_PA25_DAC_1 = 6;
        on_pBut_READ_clicked(0xBFD80250, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80240: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 0 полоска 12 (0xBFD80260, 0xBFD80264)
    // Канал 0 полоска 13 (0xBFD80268, 0xBFD8026С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready6)){

        ui->lens1line11->setText(QString::number(udac1.r.line11));
        lbl = (udac1.r.line11 * 2500) / 4095;
        ui->label_lins_1_11->setText(QString::number(lbl)); // Канал 0 полоска 10 (0xBFD80250, 0xBFD80254)

        ui->lens1line12->setText(QString::number(udac1.r.line12));
        lbl = (udac1.r.line12 * 2500) / 4095;
        ui->label_lins_1_12->setText(QString::number(lbl)); // Канал 0 полоска 11 (0xBFD80258, 0xBFD8025С)

        dump_PA25_DAC_1 = 7;
        on_pBut_READ_clicked(0xBFD80260, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80250: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 0 полоска 14 (0xBFD80270, 0xBFD80274)
    // Канал 0 полоска 15 (0xBFD80278, 0xBFD8027С)
    if(!waitingFor(&MainWindow::DAC1_25PA_ready7)){

        ui->lens1line13->setText(QString::number(udac1.r.line13));
        lbl = (udac1.r.line13 * 2500) / 4095;
        ui->label_lins_1_13->setText(QString::number(lbl)); // Канал 0 полоска 12 (0xBFD80260, 0xBFD80264)

        ui->lens1line14->setText(QString::number(udac1.r.line14));
        lbl = (udac1.r.line14 * 2500) / 4095;
        ui->label_lins_1_14->setText(QString::number(lbl)); // Канал 0 полоска 13 (0xBFD80268, 0xBFD8026С)

        dump_PA25_DAC_1 = 8;
        on_pBut_READ_clicked(0xBFD80270, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80260: превышено время ожидания чтения 25ПА;");
    }

    // Завершил чтение 25ПА, жду дамп адресов 0xBFD80270, 0xBFD80274, 0xBFD80278, 0xBFD8027С;
    if(!waitingFor(&MainWindow::DAC1_25PA_ready8)){
        dump_PA25_DAC_1 = 0;

        ui->lens1line15->setText(QString::number(udac1.r.line15));
        lbl = (udac1.r.line15 * 2500) / 4095;
        ui->label_lins_1_15->setText(QString::number(lbl)); // Канал 0 полоска 14 (0xBFD80270, 0xBFD80274)

        ui->lens1line16->setText(QString::number(udac1.r.line16));
        lbl = (udac1.r.line16 * 2500) / 4095;
        ui->label_lins_1_16->setText(QString::number(lbl)); // Канал 0 полоска 15 (0xBFD80278, 0xBFD8027С)

    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80270: превышено время ожидания чтения 25ПА;");
        dump_PA25_DAC_1 = 0;
    }

}


void MainWindow::on_arbitary_KU_clicked()
{
    try {
        ushort pdat[32];
        memset(pdat, 0, sizeof(pdat));
        pdat[0] =  ui->lineEdit_KOD_KU->text().toUInt(nullptr,16) & 0xFFFF;
        pdat[1] = 0;
        pdat[2] = ui->KU_SD3->text().toUInt(nullptr,16) & 0xFFFF;       // СД3
        pdat[3] = ui->KU_SD4->text().toUInt(nullptr,16) & 0xFFFF;       // СД4
        pdat[4] = ui->KU_SD5->text().toUInt(nullptr,16) & 0xFFFF;       // СД5
        pdat[5] = ui->KU_SD6->text().toUInt(nullptr,16) & 0xFFFF;       // СД6
        pdat[6] = ui->KU_SD7->text().toUInt(nullptr,16) & 0xFFFF;       // СД7
        pdat[7] = ui->KU_SD8->text().toUInt(nullptr,16) & 0xFFFF;       // СД8
        pdat[8] = ui->KU_SD9->text().toUInt(nullptr,16) & 0xFFFF;       // СД9
        pdat[9] = ui->KU_SD10->text().toUInt(nullptr,16) & 0xFFFF;      // СД10
        pdat[10] = ui->KU_SD11->text().toUInt(nullptr,16) & 0xFFFF;     // СД11
        pdat[11] = ui->KU_SD12->text().toUInt(nullptr,16) & 0xFFFF;     // СД12
        pdat[12] = ui->KU_SD13->text().toUInt(nullptr,16) & 0xFFFF;     // СД13
        pdat[13] = ui->KU_SD14->text().toUInt(nullptr,16) & 0xFFFF;     // СД14
        pdat[14] = ui->KU_SD15->text().toUInt(nullptr,16) & 0xFFFF;     // СД15
        pdat[15] = ui->KU_SD16->text().toUInt(nullptr,16) & 0xFFFF;     // СД16
        pdat[16] = ui->KU_SD17->text().toUInt(nullptr,16) & 0xFFFF;     // СД17
        pdat[17] = ui->KU_SD18->text().toUInt(nullptr,16) & 0xFFFF;     // СД18
        pdat[18] = ui->KU_SD19->text().toUInt(nullptr,16) & 0xFFFF;     // СД19
        pdat[19] = ui->KU_SD20->text().toUInt(nullptr,16) & 0xFFFF;     // СД20
        pdat[20] = ui->KU_SD21->text().toUInt(nullptr,16) & 0xFFFF;     // СД21
        pdat[21] = ui->KU_SD22->text().toUInt(nullptr,16) & 0xFFFF;     // СД22
        pdat[22] = ui->KU_SD23->text().toUInt(nullptr,16) & 0xFFFF;     // СД23
        pdat[23] = ui->KU_SD24->text().toUInt(nullptr,16) & 0xFFFF;     // СД24
        pdat[24] = ui->KU_SD25->text().toUInt(nullptr,16) & 0xFFFF;     // СД25
        pdat[25] = ui->KU_SD26->text().toUInt(nullptr,16) & 0xFFFF;     // СД26
        pdat[26] = ui->KU_SD27->text().toUInt(nullptr,16) & 0xFFFF;     // СД27
        pdat[27] = ui->KU_SD28->text().toUInt(nullptr,16) & 0xFFFF;     // СД28
        pdat[28] = ui->KU_SD29->text().toUInt(nullptr,16) & 0xFFFF;     // СД29
        pdat[29] = ui->KU_SD30->text().toUInt(nullptr,16) & 0xFFFF;     // СД30
        pdat[30] = ui->KU_SD31->text().toUInt(nullptr,16) & 0xFFFF;     // СД31

        preparingArrForTransmitWithData(&pdat[0]);
        toTXline(pcBuf);

        int num = ui->comboBox->currentText().toInt();
        mko.writeToRT(1,num,1,32,pcBuf);
        // Запрос Квитанции
        QTimer::singleShot(100,this, [this](){
            ui->pBut_read_Kvit->click();
        });
    }  catch (const std::exception &e) {
        qDebug()<< "Exception:" << e.what();
    }

}


void MainWindow::on_clear_KK_OU_clicked()
{
    ui->textEditTX->clear();
}


void MainWindow::on_clear_OU_KK_clicked()
{
    ui->textEditRX->clear();
}

void MainWindow::on_btn_lens1line1_clicked()
{
    unsigned short value = ui->lens1line1->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80200, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80204, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens1line2_clicked()
{
    unsigned short value = ui->lens1line2->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80208, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8020C, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line3_clicked()
{
    unsigned short value = ui->lens1line3->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80210,  value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80214, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens1line4_clicked()
{
    unsigned short value = ui->lens1line4->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80218, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8021C, (value >> 8) & 0xFF );

}


void MainWindow::on_btn_lens1line5_clicked()
{
    unsigned short value = ui->lens1line5->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80220, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80224, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line6_clicked()
{
    unsigned short value = ui->lens1line6->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80228, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8022C, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line7_clicked()
{
    unsigned short value = ui->lens1line7->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80230, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80234, (value >> 8) & 0xFF);


}


void MainWindow::on_btn_lens1line8_clicked()
{
    unsigned short value = ui->lens1line8->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80238, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8023C, (value >> 8) & 0xFF);
}


void MainWindow::on_btn_lens1line9_clicked()
{
    unsigned short value = ui->lens1line9->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80240, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80244, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line10_clicked()
{
    unsigned short value = ui->lens1line10->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80248, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8024C, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line11_clicked()
{
    unsigned short value = ui->lens1line11->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80250, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80254, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line12_clicked()
{
    unsigned short value = ui->lens1line12->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80258, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8025C, (value >> 8) & 0xFF);
}


void MainWindow::on_btn_lens1line13_clicked()
{
    unsigned short value = ui->lens1line13->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80260, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80264, (value >> 8) & 0xFF);
}


void MainWindow::on_btn_lens1line14_clicked()
{
    unsigned short value = ui->lens1line14->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80268, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8026C, (value >> 8) & 0xFF);
}


void MainWindow::on_btn_lens1line15_clicked()
{
    unsigned short value = ui->lens1line15->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80270, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80274, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens1line16_clicked()
{
    unsigned short value = ui->lens1line16->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80278, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8027C, (value >> 8) & 0xFF);
}


void MainWindow::on_btn_read_lins2_clicked()
{
    uint16_t lbl;
    dump_PA25_DAC_2 = 1;
    // 8 СД (16 разряд)
    // Канал 1 полоска 0 (0xBFD80280, 0xBFD80284)
    // Канал 1 полоска 1 (0xBFD80288, 0xBFD8028С)
    on_pBut_READ_clicked(0xBFD80280, 8);

    // 8 СД (16 разряд)
    // Канал 1 полоска 2 (0xBFD80280, 0xBFD80284)
    // Канал 1 полоска 3 (0xBFD80288, 0xBFD8028С)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready1)){

        ui->lens2line1->setText(QString::number(udac2.r.line1));
        lbl = (udac2.r.line1 * 2500) / 4095;
        ui->label_lins_2_1->setText(QString::number(lbl)); // Канал 1 полоска 0 (0xBFD80280, 0xBFD80284)

        ui->lens2line2->setText(QString::number(udac2.r.line2));
        lbl = (udac2.r.line2 * 2500) / 4095;
        ui->label_lins_2_2->setText(QString::number(lbl)); // Канал 1 полоска 1 (0xBFD80288, 0xBFD8028С)

        dump_PA25_DAC_2 = 2;
        on_pBut_READ_clicked(0xBFD80290, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80280: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 1 полоска 4 (0xBFD80290, 0xBFD80294)
    // Канал 1 полоска 5 (0xBFD80298, 0xBFD8029С)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready2)){

        ui->lens2line3->setText(QString::number(udac2.r.line3));
        lbl = (udac2.r.line3 * 2500) / 4095;
        ui->label_lins_2_3->setText(QString::number(lbl)); // Канал 1 полоска 2 (0xBFD80290, 0xBFD80294)

        ui->lens2line4->setText(QString::number(udac2.r.line4));
        lbl = (udac2.r.line4 * 2500) / 4095;
        ui->label_lins_2_4->setText(QString::number(lbl)); // Канал 1 полоска 3 (0xBFD80298, 0xBFD8029С)

        dump_PA25_DAC_2 = 3;
        on_pBut_READ_clicked(0xBFD802A0, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80290: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 1 полоска 6 (0xBFD802A0, 0xBFD802A4)
    // Канал 1 полоска 7 (0xBFD802A8, 0xBFD802AС)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready3)){

        ui->lens2line5->setText(QString::number(udac2.r.line5));
        lbl = (udac2.r.line5 * 2500) / 4095;
        ui->label_lins_2_5->setText(QString::number(lbl)); // Канал 1 полоска 4 (0xBFD802A0, 0xBFD802A4)

        ui->lens2line6->setText(QString::number(udac2.r.line6));
        lbl = (udac2.r.line6 * 2500) / 4095;
        ui->label_lins_2_6->setText(QString::number(lbl)); // Канал 1 полоска 5 (0xBFD802A8, 0xBFD802AС)

        dump_PA25_DAC_2 = 4;
        on_pBut_READ_clicked(0xBFD802B0, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD802A0: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 1 полоска 8 (0xBFD802B0, 0xBFD802B4)
    // Канал 1 полоска 9 (0xBFD802B8, 0xBFD802BС)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready4)){

        ui->lens2line7->setText(QString::number(udac2.r.line7));
        lbl = (udac2.r.line7 * 2500) / 4095;
        ui->label_lins_2_7->setText(QString::number(lbl)); // Канал 1 полоска 6 (0xBFD802B0, 0xBFD802B4)

        ui->lens2line8->setText(QString::number(udac2.r.line8));
        lbl = (udac2.r.line8 * 2500) / 4095;
        ui->label_lins_2_8->setText(QString::number(lbl)); // Канал 1 полоска 7 (0xBFD802B8, 0xBFD802BС)

        dump_PA25_DAC_2 = 5;
        on_pBut_READ_clicked(0xBFD802C0, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD802B0: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 1 полоска 10 (0xBFD802C0, 0xBFD802C4)
    // Канал 1 полоска 11 (0xBFD802C8, 0xBFD802CС)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready5)){

        ui->lens2line9->setText(QString::number(udac2.r.line9));
        lbl = (udac2.r.line9 * 2500) / 4095;
        ui->label_lins_2_9->setText(QString::number(lbl)); // Канал 1 полоска 8 (0xBFD802C0, 0xBFD802C4)

        ui->lens2line10->setText(QString::number(udac2.r.line10));
        lbl = (udac2.r.line10 * 2500) / 4095;
        ui->label_lins_2_10->setText(QString::number(lbl)); // Канал 1 полоска 9 (0xBFD802C8, 0xBFD802CС)

        dump_PA25_DAC_2 = 6;
        on_pBut_READ_clicked(0xBFD802D0, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD802C0: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 1 полоска 12 (0xBFD802D0, 0xBFD802D4)
    // Канал 1 полоска 13 (0xBFD802D8, 0xBFD802DС)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready6)){

        ui->lens2line11->setText(QString::number(udac2.r.line11));
        lbl = (udac2.r.line11 * 2500) / 4095;
        ui->label_lins_2_11->setText(QString::number(lbl)); // Канал 1 полоска 10 (0xBFD802D0, 0xBFD802D4)

        ui->lens1line12->setText(QString::number(udac2.r.line12));
        lbl = (udac1.r.line12 * 2500) / 4095;
        ui->label_lins_2_12->setText(QString::number(lbl)); // Канал 1 полоска 11 (0xBFD802D8, 0xBFD802DС)

        dump_PA25_DAC_2 = 7;
        on_pBut_READ_clicked(0xBFD80E0, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD802D0: превышено время ожидания чтения 25ПА;");
    }

    // 8 СД (16 разряд)
    // Канал 1 полоска 14 (0xBFD802E0, 0xBFD802E4)
    // Канал 1 полоска 15 (0xBFD802E8, 0xBFD802EС)
    if(!waitingFor(&MainWindow::DAC2_25PA_ready7)){

        ui->lens2line13->setText(QString::number(udac2.r.line13));
        lbl = (udac2.r.line13 * 2500) / 4095;
        ui->label_lins_2_13->setText(QString::number(lbl)); // Канал 1 полоска 12 (0xBFD802E0, 0xBFD802E4)

        ui->lens2line14->setText(QString::number(udac2.r.line14));
        lbl = (udac2.r.line14 * 2500) / 4095;
        ui->label_lins_2_14->setText(QString::number(lbl)); // Канал 1 полоска 13 (0xBFD802E8, 0xBFD802EС)

        dump_PA25_DAC_2 = 8;
        on_pBut_READ_clicked(0xBFD802F0, 8);
    } else {
        PrintERR("Ошибка чтения адреса 0xBFD802E0: превышено время ожидания чтения 25ПА;");
    }

    // Завершил чтение 25ПА, жду дамп адресов 0xBFD802F0, 0xBFD802F4, 0xBFD802F8, 0xBFD802FС;
    if(!waitingFor(&MainWindow::DAC2_25PA_ready8)){
        dump_PA25_DAC_2 = 0;

        ui->lens2line15->setText(QString::number(udac2.r.line15));
        lbl = (udac2.r.line15 * 2500) / 4095;
        ui->label_lins_2_15->setText(QString::number(lbl)); // Канал 1 полоска 14 (0xBFD802F0, 0xBFD802F4)

        ui->lens2line16->setText(QString::number(udac2.r.line16));
        lbl = (udac2.r.line16 * 2500) / 4095;
        ui->label_lins_2_16->setText(QString::number(lbl)); // Канал 1 полоска 15 (0xBFD802F8, 0xBFD802FС)

    } else {
        PrintERR("Ошибка чтения адреса 0xBFD80270: превышено время ожидания чтения 25ПА;");
        dump_PA25_DAC_2 = 0;
    }

}

void MainWindow::on_btn_lens2line1_clicked()
{
    unsigned short value = ui->lens2line1->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80280, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80284, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line2_clicked()
{
    unsigned short value = ui->lens2line2->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80288, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8028C, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line3_clicked()
{
    unsigned short value = ui->lens2line3->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80290, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD80294, (value >> 8) & 0xFF);
}

void MainWindow::on_btn_lens2line4_clicked()
{
    unsigned short value = ui->lens2line4->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD80298, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD8029C, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line5_clicked()
{
    unsigned short value = ui->lens2line5->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802A0, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802A4, (value >> 8) & 0xFF);
}

void MainWindow::on_btn_lens2line6_clicked()
{
    unsigned short value = ui->lens2line6->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802A8, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802AC, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line7_clicked()
{
    unsigned short value = ui->lens2line7->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802B0, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802B4, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line8_clicked()
{
    unsigned short value = ui->lens2line8->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802B8, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802BC, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line9_clicked()
{
    unsigned short value = ui->lens2line9->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802C0, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802C4, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line10_clicked()
{
    unsigned short value = ui->lens2line10->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802C8, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802CC, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line11_clicked()
{
    unsigned short value = ui->lens2line11->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802D0, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802D4, (value >> 8) & 0xFF);

}

void MainWindow::on_btn_lens2line12_clicked()
{
    unsigned short value = ui->lens2line12->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802D8, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802DC, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens2line13_clicked()
{
    unsigned short value = ui->lens2line13->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802E0, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802E4, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens2line14_clicked()
{
    unsigned short value = ui->lens2line14->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802E8, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802EC, (value >> 8) & 0xFF);

}


void MainWindow::on_btn_lens2line15_clicked()
{
    unsigned short value = ui->lens2line15->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802F0, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802F4, (value >> 8) & 0xFF);



}


void MainWindow::on_btn_lens2line16_clicked()
{
    unsigned short value = ui->lens2line16->text().toUInt();

    on_pBut_WRITE_parametr_clicked(0xBFD802F8, value & 0xFF);
    QThread::msleep(50);
    on_pBut_WRITE_parametr_clicked(0xBFD802FC, (value >> 8) & 0xFF);



}

void MainWindow::set_init_state_DC(bool state_DC){
    init_state_DC = state_DC;
}


