#include "mil_std_1553.h"
#include "globaldef.h"

        HANDLE          hEvent;
static  WORD            pcBuf[64];

//MIL_STD_1553
MIL_STD_1553::MIL_STD_1553(QObject *parent) : QObject(parent)
{

}

MIL_STD_1553::~MIL_STD_1553()
{
    qDebug("MIL_STD_1553 By in Thread!");
    emit finished_Port();
}

void MIL_STD_1553::DoWork()
{
    int i = 0;
    int nInt = 0;
    TTmkEventData bcEvD;
    messange_MKO("MKO in Thread!;");

    //   unsigned short wMode;

    while (this->thread()->isFinished() != true)
    {
       switch ( WaitForSingleObject(hEvent, 100) )
       {
       case WAIT_OBJECT_0:        // event
       {
           ResetEvent(hEvent);

           tmkgetevd(&bcEvD);

           nInt = bcEvD.nInt;
 //            wMode = bcEvD.wMode;
       }break;

       case WAIT_TIMEOUT:         // timeout
       {
           nInt = 0;

       }break;

       case WAIT_FAILED:          // error
       {
           nInt = -1;

       }break;

       }

       if ( nInt != -1 )
       {
          do
          {
             switch ( nInt )
             {

             case 0:
             {
                 // no event
             }
             break;

             case 1:
             {
                 // нормальное завершение обмена по МК (не обнаружено ошибок или установленных битов в поле флагов ОС)
                 //SendMessage(BCForm->Handle, WM_BC_INT_NORM, bcEvD.bc.wResult, 0);
                 //qDebug() << "нормальное завершение обмена по МК";
                 emit msgWM_BC_INT_NORM();
             }
             break;

             case 2:
             {
                 // завершение обмена по МК выбранным КК с исключительной ситуацией
                 // (обнаружены ошибки или установлены биты в поле флагов ОС)
                 // SendMessage(BCForm->Handle, WM_BC_INT_EXC, bcEvD.bc.wResult,bcEvD.bc.wAW1 ^ bcEvD.bc.wAW2<<16);
                 qDebug() << "завершение обмена с исключительной ситуацией";
                 emit msgWM_BC_INT_EXC();
             }
             break;

             case 3:
             {
                 // завершение обмена по МК выбранным КК цепочки сообщений
                 // SendMessage(BCForm->Handle, WM_BC_INT_X,bcEvD.bcx.wResultX, bcEvD.bcx.wBase);
                 qDebug() << "завершение обмена по МК выбранным КК цепочки сообщений";
             }
             break;

             case 4:
             {
                 // возникновение дополнительного сигнального прерывания
                 // SendMessage(BCForm->Handle, WM_BC_INT_SIG, 0, bcEvD.bcx.wBase);
                 qDebug() << "возникновение дополнительного сигнального прерывания";
             }
             break;

             }

             tmkgetevd(&bcEvD);

             nInt = bcEvD.nInt;

             if ( nInt != 0 )
             {
                ResetEvent(hEvent);
             }

          } while ( nInt != 0 );
       }
       else
       {
          //Synchronize(ShowMsg);
          //QMessageBox::warning(this, "Ошибка","GetLastError() вернула код"+QString::number(GetLastError()));
          //qDebug() << "Ошибка, GetLastError() вернула код" << QString::number(GetLastError());
       }
    }

}

void MIL_STD_1553::closeDriver(){
    if ( nTMK )
    {
       tmkdefevent(nullptr, true);
       tmkdone(ALL_TMKS);

       if ( hEvent != INVALID_HANDLE_VALUE )  CloseHandle(hEvent);

       TmkClose();
    }
}


void  MIL_STD_1553::initMKO(void)
{
    QString str,srtn;
    int i,j;

    messange_MKO("Старт инициализации МКО;");
    qDebug("Start initMKO;");

    // Инициализация МКО драйвера
    str = InitMKODriver();
    if (str != "")
    {
       error_MKO("Ошибка инициализации драйвера МКО;");
       qDebug(str.toLatin1());
    }
    else
    {
       messange_MKO("Драйвер МКО успешно инициализирован;");
       //
       //Выбора текущего устройства
       res = tmkselect(nTmkNumber);

       // Прежде всего, процесс должен сообщить драйверу идентификатор
       // (handle) используемого события для текущего выбранного ТМК
       tmkdefevent(hEvent, true);

       tmkgetinfo(&TmkConfigData);
       srtn = TmkConfigData.szName;
       messange_MKO("device TMK: " + srtn);

       // BC_MODE
       // Режим КК, ОУ или МТ для выбранного устройства задается функциями bcreset, rtreset или mtreset
       res = bcreset();

       nMaxBase = bcgetmaxbase();

       // Clear RAM
       for ( i = 0; i < BASE_LEN; ++i ) pcBuf[i] = 0;
       for ( j = 0; j < (nMaxBase - 1); ++j )
       {

          bcdefbase(j);
          bcputblk(0, pcBuf, BASE_LEN);
       }

       // Initialization
       // BC_GENER1_BL - задание этого бита блокирует прерывания по обнаружению генерации
       //в основной ЛПИ устройства;
       // BC_GENER2_BL - задание этого бита блокирует прерывания по обнаружению генерации
       //в резервной ЛПИ устройства;
       res = bcdefirqmode(RT_GENER1_BL | RT_GENER2_BL);

       uBus     =   BUS_1;

       // Выбор линии ЛПИ. 0 - функция выполнена успешно;
       res      =   bcdefbus(uBus);

       // Функция настраивает выбранный КК и драйвер на дальнейшую работу с ДОЗУ в
       // указанной базе. 0 - функция выполнена успешно;
       res      =   bcdefbase(1);

       // Функция bcgetstate возвращает двойное
       // слово, младшие 16 разрядов которого содержат номер базы, с которой происходит
       // работа, а старшие 16 разрядов - текущее слово состояния.
       res      =   bcgetstate();
    }
}

//
// Функция подключатает драйвер и конфигурирует все устройства
//
QString MIL_STD_1553::InitMKODriver()
{
    QString str = "";

    messange_MKO("Старт инициализации драйвера МКО;");
    // Функция подключает драйвер к вызвавшему функцию процессу.
    // 0 - функция выполнена успешно
    if (TmkOpen())
    {
        //QString str = QString("<span style=\" color:#ff0000;\">%1</span>").arg(">>>");
        str = "Ошибка при загрузке драйвера 1553Bwdm.sys;";
        error_MKO(str);
        //msgWM_BC_INT_NORM();
        return str;
    }

    // Создаем объект события
    hEvent = CreateEvent(nullptr, true, false, nullptr);
    if ( hEvent == INVALID_HANDLE_VALUE )
    {
        str = "Ошибка при создании объекта Event";
        error_MKO(str);
        return str;
    }

    // Функция возвращает максимальный номер устройства
    nTMK = tmkgetmaxn() + 1;
    // Все доступные устройства МКО
    for ( nTmkNumber = 0; nTmkNumber < nTMK; ++nTmkNumber )
    {
        messange_MKO("найден Tmk: " +  QString::number(nTmkNumber));
       //ui->textEdit->append("найден Tmk: " +  QString::number(nTmkNumber));
    }

    for ( nTmkNumber = 0; nTmkNumber < nTMK; ++nTmkNumber )
    {
        // Функция подключает к вызвавшему процессу устройство с заданным номером.
        // Если устройство с таким номером не существует или уже подключено к другому
        // процессу, то возвращается код ошибки. После выполнения этой функции
        //конфигурируемое устройство остается выбранным для работы.
        // 0 - конфигурирование устройства выполнено

        if ( tmkconfig(nTmkNumber) == 0 )
        {
         //cbTmk->ItemIndex = nTmkNumber;
         //ui->comboBox->setCurrentIndex(nTmkNumber);
            messange_MKO("выбран Tmk: " +  QString::number(nTmkNumber));
            break;
        }
    }


    if ( nTmkNumber == nTMK )
    {
        error_MKO("Неверный номер Tmk");
        //QMessageBox::warning(this, "Внимание","Неверный номер Tmk");
        exit(0);
    }

    return str;
}

void MIL_STD_1553::ShowRAM(void)
{
   static int j;
   QString st = "";

  // bcdefbase(ui->spinBox->value());

   bcgetblk(0, pcBuf, 64);

   for (j = 0; j < 32; ++j )
   {
        st += (QString(" %1").arg(pcBuf[j], 4, 16, QLatin1Char( '0' )));
   }
   messange_MKO(st);
}

//---------------------------------------------------------------------------
// универсальная функция чтения по МКО из ОУ ---> КК
void MIL_STD_1553::readFromRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC)
{
    if (pDWC == 32) pDWC = 0;

    //CW(ADDR,DIR,SUBADDR,NWORDS)
    //CW(Адресс, Направление(прием или передача), подадрес, кол-во слов))
    dwCW = CW(pRTA, RT_TRANSMIT, pSA, pDWC);

    bcdefbase(pBase);
    bcputw(0, dwCW);

    // перед отправкой сообщение возможно нужно обнулить контроллер

    bcstart(pBase, DATA_BC_RT); // старт сообщения (CtrlCode = DATA_BC_RT);
}
//---------------------------------------------------------------------------


void MIL_STD_1553::setRTA(DWORD RTA)
{
    MIL_STD_1553::RTA = RTA;
}

void MIL_STD_1553::setSA(DWORD SA)
{
    MIL_STD_1553::SA = SA;
}

void MIL_STD_1553::setDWC(DWORD DWC)
{
    MIL_STD_1553::DWC = DWC;
}

void MIL_STD_1553::setLPI(bool uBus)
{
    bcdefbus(uBus);
}

WORD* MIL_STD_1553::getPcBuf(){
    return pcBuf;
}

unsigned int MIL_STD_1553::getSizePcBuf(){
    return sizeof (pcBuf);
}

// универсальная функция передачи по МКО из КК ---> ОУ
void MIL_STD_1553::writeToRT(ushort pBase,DWORD pRTA,DWORD pSA,DWORD pDWC, WORD *pcBufData)
{
    WORD            pData[64];

    memset(pData, 0, sizeof(pData));
    memcpy(pData, pcBufData , 64);

    bcputblk(pBase, pData, pDWC);

    dwCW = CW(pRTA, RT_RECEIVE, pSA, pDWC);

    bcdefbase(pBase);
    bcputw(0, dwCW);

    bcstart(pBase, DATA_RT_BC);
}

// CRC16 calculation function
unsigned short MIL_STD_1553::Calc_CRC16(uchar  *pBlk, unsigned int len,unsigned short *pTab16)
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




