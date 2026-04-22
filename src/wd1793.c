/*
    Термины:
      * Sector (Сектор) - сектор TR-DOS имеет размер 256 байт, номера 1-16
      * Block (Блок) - минимальный объем данных на SD/TF карте для чтения/записи составляет 512 байт
*/
//#define  DEBUG
#include "config.h"  
#ifdef TRDOS_1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include "util_sd.h"
#include "ff.h"
#include "boot.h"
#include "fdi_stream.h"

//=============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ МОДУЛЯ
//=============================================================================

uint8_t sectors_per_track;
FDD_CTX fdd = {0};

// Буфер для данных диска (переопределен на глобальный буфер SD карты)
#define DiskBuf sd_buffer

uint64_t old_t;                 // Переменная для отсчета таймаутов (предыдущее время)
uint16_t BufferPos = 0;         // Текущая позиция в буфере DiskBuf
uint16_t SectorPos = 0;         // Счетчик байт внутри текущего сектора
uint16_t pos;                   // Позиция сектора на диске (LBA)
uint8_t BufferUpdated = 0;      // Флаг: 1 - буфер изменен, нужна запись на SD карту
uint16_t FormatCounter = 0;     // Счетчик байт 0x4E при форматировании
uint8_t NewDrive = 5;           // Номер дисковода, который нужно смонтировать
uint8_t OldDrive = 5;           // Номер ранее активного дисковода

// Переменные файловой системы FATFS
FIL fileTRD;                    // Файл образа диска
FATFS fss;
DIR dir;
FILINFO finfo;
FRESULT res;
uint8_t FilesCount = 0;

unsigned int br;                // Количество прочитанных/записанных байт
uint8_t CmdType = 1;            // Тип текущей команды (1 - позиционирование, 2 - сектор, 3 - трек)
uint16_t CurrentDiskPos = DEFULTDISKPOS; // Текущая позиция в файле образа (чтобы не сикать постоянно)

WD1793_struct WD1793;           // Экземпляр контроллера

uint8_t Requests = 0;           // Регистр запросов (INTRQ/DRQ) для выдачи в порт
uint8_t DRV = 5;                // Текущий активный привод (0-3)
uint8_t Prevwd1793_PortFF = 0xff; // Предыдущее значение порта #FF
uint8_t DiskSector;             // Счетчик сектора для команд чтения адреса
bool DiskIndex;                 // Флаг индексного отверстия
uint8_t wd1793_PortFF;          // Текущее значение порта #FF

// Прототипы внутренних функций
void WD1793_CmdStartIdle();

uint8_t NewCommandReceived = 0;  // Флаг: получена новая команда для исполнения

typedef void (*Command) ();     // Тип указателя на функцию-команду
Command CurrentCommand = WD1793_CmdStartIdle; // Текущая выполняемая команда
Command NextCommand = NULL;     // Команда, которая будет выполнена после задержки

// Таблица векторов команд контроллера WD1793 (индекс = код команды >> 4)
Command CommandTable[16] = {
        WD1793_Cmd_Restore,     // 00 Restore      Восстановление                  - %0000 hvrr
        WD1793_Cmd_Seek,        // 10 Seek         Поиск/Позиционирование          - %0001 hvrr
        WD1793_Cmd_Step,        // 20 Step         Шаг в предыдущем направлении    - %001t hvrr
        WD1793_Cmd_Step,        // 30 Step         Шаг в предыдущем направлении    - %001t hvrr
        WD1793_Cmd_Step,        // 40 Step In      Шаг вперед                      - %010t hvrr
        WD1793_Cmd_Step,        // 50 Step In      Шаг вперед                      - %010t hvrr
        WD1793_Cmd_Step,        // 60 Step Out     Шаг назад                       - %011t hvrr
        WD1793_Cmd_Step,        // 70 Step Out     Шаг назад                       - %011t hvrr
        WD1793_Cmd_ReadSector,  // 80 Read Sector  Чтение сектора                  - %100m seca
        WD1793_Cmd_ReadSector,  // 90 Read Sector  Чтение сектора                  - %100m seca
        WD1793_Cmd_WriteSector, // A0 Write Sector Запись сектора                  - %101m sec0
        WD1793_Cmd_WriteSector, // B0 Write Sector Запись сектора                  - %101m sec0
        WD1793_Cmd_ReadAddress, // C0 Read Address Чтение адреса                   - %1100 0e00
        WD1793_CmdStartIdle,    // D0 Force Int    Принудительное прерывание        - %1101 iiii
        WD1793_Cmd_ReadTrack,   // E0 Read Track   Чтение дорожки                  - %1110 0e00
        WD1793_Cmd_WriteTrack   // F0 Write Track  Запись дорожки/форматир-е       - %1111 0e00
    };

/* 
    Расшифровка битов команд (справочно):
    rr - скорость позиционирования головки: 00 - 6мс, 01 - 12мс, 10 - 20мс, 11 - 30мс (1Mhz).
    v  - проверка номера дорожки после позиционирования.
    h  - загрузка головки (всегда должен быть в 1!).
    t  - изменение номера дорожки в регистре дорожки после каждого шага.
    a  - тип адресной метки (0 - #fb, стирание сектора запрещено 1 - #F8, стирание сектора разрешено).
    c  - проверка номера стороны диска при идентификации индексной области.
    e  - задержка после загрузки головки на 30мс (1Mhz).
    s  - сторона диска (0 - верх/1 - низ).
    m  - мультисекторная операция.
    i  - условие прерывания.
*/

static uint8_t last_track = 0xff; // Последняя загруженная дорожка (для оптимизации FDI)
static uint8_t last_side  = 0xff; // Последняя загруженная сторона (для оптимизации FDI)

//=============================================================================
// ФУНКЦИИ РАБОТЫ С ФАЙЛАМИ ОБРАЗОВ
//=============================================================================

//-----------------------------------------------------------------------------
// Открытие TRD файла
//-----------------------------------------------------------------------------
bool OpenTRDFile(char *sn, uint8_t drv)
{   
    // Сброс кеша загруженной дорожки
    last_track = 0xff;
    last_side  = 0xff;

    // Установка геометрии диска по умолчанию (стандартный TR-DOS)
    fdd.sector_size = 256;
    fdd.data_offset = 0;
    fdd.sectors_per_track = 16;
    fdd.cylinders = 80;
    fdd.heads = 2;

    // Сохранение имени файла в конфигурации
    strcpy(conf.Disks[drv & 0x03], sn);
    strncpy(dir_patch_info, conf.Disks[drv & 0x03], (DIRS_DEPTH*(LENF+16)) );

    // Открытие файла
    f_open(&fileTRD, sn, FA_READ);   

    // Определение типа TRD файла (стандартный или урезанный)
    if (file_type[drv] == TRD)
    {
        if (sd_file_size(&fileTRD) < 655360) 
            file_type[drv] = TRDS; // Нестандартный размер (TRD Short)
        // else file_type[drv]=TRD; // Стандарт
    }

    // Сброс состояния привода
    DRV = 5;
    NewDrive = drv;
    NoDisk = 0;
    return true;
}

//-----------------------------------------------------------------------------
// Контекст и интерфейс для работы с FDI Stream
//-----------------------------------------------------------------------------
static fdi_stream_ctx_t fdi_ctx;
static fdi_file_io_t fdi_io = {
    .open = fatfs_open,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .read = fatfs_read,
    .tell = fatfs_tell,
    .size = fatfs_size
};

//-----------------------------------------------------------------------------
// Открытие FDI файла (гибкий формат образов дисков)
//-----------------------------------------------------------------------------
bool OpenFDI_File(char *sn, uint8_t drv)
{
    file_type[drv] = FDI;
    last_track = 0xff;
    last_side  = 0xff;
    fdd.data_offset = 0;

    // Сохранение имени файла в конфигурации
    strcpy(conf.Disks[drv & 0x03], sn);
    strncpy(dir_patch_info, conf.Disks[drv & 0x03], (DIRS_DEPTH*(LENF+16)) );

    // Открытие потока FDI и чтение геометрии диска
    fdi_stream_result_t res = fdi_stream_open(&fdi_ctx, &fdi_io, sn);
    fdd.cylinders = fdi_ctx.cylinders;
    fdd.heads = fdi_ctx.heads;

    // Сброс состояния привода
    DRV = 5;
    NewDrive = drv;
    NoDisk = 0;
    return true;
}

//=============================================================================
// ФУНКЦИИ ЧТЕНИЯ/ЗАПИСИ РЕГИСТРОВ (ЭМУЛЯЦИЯ ПОРТОВ)
//=============================================================================

//-----------------------------------------------------------------------------
// Запись в порт контроллера со стороны Z80
// Address: 0 - 0x1F, 1 - 0x3F, 2 - 0x5F, 3 - 0x7F
//-----------------------------------------------------------------------------
void WD1793_Write(uint8_t Address, uint8_t Value)
{ 
    switch (Address & 0x03)
    {
        case 0 : // 0x1F Запись в регистр команд
            Address = 4;
            NewCommandReceived = 1; // Установка флага новой команды
            break;
        case 2 :
            // Запись в регистр сектора (обычно не требует особой обработки)
            break;
        case 3 : // 0x7F Запись в регистр данных
            // Сброс флага запроса данных (DRQ) при записи в регистр данных
            WD1793.StatusRegister &= ~_BV(stsDRQ);
            Requests &= ~_BV(rqDRQ);
            break;
    }

    WD1793.Regs[Address] = Value; // Сохранение значения в теневом регистре
}

//-----------------------------------------------------------------------------
// Чтение из порта контроллера со стороны Z80
//-----------------------------------------------------------------------------
uint8_t WD1793_Read(uint8_t Address)
{ 
    Address &= 0x03;
    switch (Address)
    {
        case 0 : // 0x1F Чтение регистра состояния
            // Сброс флага прерывания (INTRQ) после чтения статуса
            Requests &= ~_BV(rqINTRQ);
            break;
        case 3 : // 0x7F Чтение регистра данных
            // Сброс флага запроса данных (DRQ) после чтения данных
            WD1793.StatusRegister &= ~_BV(stsDRQ);
            Requests &= ~_BV(rqDRQ);
            break;
    }

    return WD1793.Regs[Address];
}

//=============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
//=============================================================================

//-----------------------------------------------------------------------------
// Проверка истечения временного периода (для создания задержек)
//-----------------------------------------------------------------------------
uint16_t HasTimePeriodExpired_old(uint16_t period)
{
    static uint16_t t = 0;  
    if(t >= period)
    {
        t = 0;
        return 1;
    }
    t++;
    return 0;
}

//-----------------------------------------------------------------------------
// Проверка истечения временного периода с использованием реального времени (мкс)
//-----------------------------------------------------------------------------
uint16_t HasTimePeriodExpired(uint64_t period)
{
    static uint64_t t = 0;
    t = (time_us_64() - old_t);
    if (t >= period)
    {
        old_t = time_us_64(); // Обновление точки отсчета
        return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Вывод сообщения об ошибке чтения/записи на экран (через глобальные переменные GUI)
//-----------------------------------------------------------------------------
void printError(char* msg, uint8_t code)
{
    // Установка сообщения в статус баре
    msg_bar = 18;   // ERR TR:
    wait_msg = 3000; 
    // printf(msg); // Отладочный вывод
}

//-----------------------------------------------------------------------------
// Сброс буфера на диск (запись измененных данных на SD карту)
//-----------------------------------------------------------------------------
void WD1793_FlushBuffer()
{   
    if (file_type[0] == FDI) return; // Для FDI запись пока не реализована или не нужна

    if (BufferUpdated == 1)
    {    
        // Установка сообщения в статус баре
        // msg_bar = 15; // WRITE TR:
        // wait_msg = 3000; 
                
        // Открытие файла для чтения/записи
        res = f_open(&fileTRD, (const char *)conf.Disks[DRV], FA_READ | FA_WRITE);
        
        // Позиционирование на нужный сектор
        f_lseek(&fileTRD, pos << 8); 
        
        // Запись буфера на SD карту
        res = f_write(&fileTRD, DiskBuf, BUFFERSIZE, &br);
        
        // Синхронизация с файловой системой (важно для сохранения данных)
        res = f_sync(&fileTRD);
    }

    BufferUpdated = 0; // Сброс флага обновления
}

//-----------------------------------------------------------------------------
// Вычисление линейной позиции (LBA) сектора в файле образа
//-----------------------------------------------------------------------------
uint32_t ComputeDiskPosition(uint8_t Sector, uint8_t Track, uint8_t Side)
{
    switch (file_type[DRV])
    {
        case SCL:
            return (((uint16_t)Track * fdd.heads + (uint16_t)Side) * fdd.sectors_per_track + (uint16_t)Sector);
        case TRD:
        case TRDS:
            return (((uint16_t)Track * fdd.heads + (uint16_t)Side) * fdd.sectors_per_track + (uint16_t)Sector);
        default:
            return 0;
    }
}

//-----------------------------------------------------------------------------
// Установка позиции чтения в файле образа (для TRD)
//-----------------------------------------------------------------------------
void SetDiskPosition(uint16_t pos)
{
    if (file_type[0] == FDI) return;

    f_lseek(&fileTRD, (uint32_t)pos << 8);
    res = f_read(&fileTRD, DiskBuf, fdd.sector_size, &br);
    if (res != 0)
        printError("Reading error: ", res);
    CurrentDiskPos = pos;
}

//-----------------------------------------------------------------------------
// Установка позиции чтения для SCL файла (особый формат)
//-----------------------------------------------------------------------------
void SetDiskPositionSCL(uint16_t pos)
{
    SCL_read_sector();
    CurrentDiskPos = pos;
}

//-----------------------------------------------------------------------------
// Сброс контроллера и переоткрытие файла образа при смене диска
//-----------------------------------------------------------------------------
void WD1793_Reset(uint8_t drive)
{   
    if (file_type[0] == FDI) 
    {
        NoDisk = 0;
        return;
    }
    
    // Сброс буфера на диск перед переоткрытием
    WD1793_FlushBuffer();
    res = f_sync(&fileTRD);
    CurrentDiskPos = DEFULTDISKPOS;
    
    // Открытие нового файла образа
    res = f_open(&fileTRD, (const char *)conf.Disks[drive & 0x03], FA_READ);
   
    if (res)
    {
        NoDisk = 1; // Ошибка открытия - диска нет
        // printError("Error open: ", res);
    }
    else
    {
        NoDisk = 0; // Диск вставлен
    }   
}

//=============================================================================
// РЕАЛИЗАЦИЯ КОМАНД КОНТРОЛЛЕРА WD1793
//=============================================================================

//-----------------------------------------------------------------------------
// Состояние ожидания (Idle)
//-----------------------------------------------------------------------------
void WD1793_CmdIdle()
{
    if (NoDisk == 1) return;

    switch (CmdType)
    {
        case 1:
            // Эмуляция индексного импульса (длительность 4 мс, период 200 мс)
            if ( ((time_us_64() - tindex) < (200000 - 4000)) ) // 4ms = 4000 µs
            {
                WD1793.StatusRegister |= _BV(stsIndexImpuls); // Установка бита импульса
            }
            else
            {
                WD1793.StatusRegister &= ~_BV(stsIndexImpuls); // Сброс бита импульса
                DiskSector = 0;
            }

            if ( ((time_us_64() - tindex) > (200000)) ) 
                tindex = time_us_64(); // Сброс таймера оборота
            break;
    }  
}

//-----------------------------------------------------------------------------
// Команда D0: Принудительное прерывание (Force Interrupt)
//-----------------------------------------------------------------------------
void WD1793_CmdStartIdle()
{  
    WD1793.StatusRegister &= ~_BV(stsBusy); // Сброс бита занятости
    Requests &= ~_BV(rqDRQ);                // Сброс запроса данных
    Requests |= _BV(rqINTRQ);               // Установка запроса прерывания

    WD1793_FlushBuffer();
    tindex = time_us_64();
    CurrentCommand = WD1793_CmdIdle;
}

//-----------------------------------------------------------------------------
// Принудительное прерывание без выдачи INTRQ (i=0)
//-----------------------------------------------------------------------------
void WD1793_CmdStartIdleForceInt0()
{
    WD1793.StatusRegister &= ~_BV(stsBusy); // Сброс бита занятости
    Requests &= ~_BV(rqINTRQ);              // Сброс запроса прерывания
    
    WD1793_FlushBuffer();
    tindex = time_us_64();
    CurrentCommand = WD1793_CmdIdle;
}

//-----------------------------------------------------------------------------
// Установка статуса после завершения команды позиционирования (Type 1)
//-----------------------------------------------------------------------------
void WD1793_CmdType1Status()
{
    if (WD1793.RealTrack == 0)
    {
        WD1793.StatusRegister |= _BV(stsTrack0); // Головка на нулевой дорожке
    }

    if (WD1793.CommandRegister & 0x08) // Проверка бита загрузки головки (h)
    { 
        WD1793.StatusRegister |= _BV(stsLoadHead);
    }

    CurrentCommand = WD1793_CmdStartIdle; // Переход в состояние ожидания
}

//-----------------------------------------------------------------------------
// Состояние задержки (используется для эмуляции времени выполнения команд)
//-----------------------------------------------------------------------------
void WD1793_CmdDelay()
{
    // Ожидание истечения таймера (закомментировано в оригинале)
    // if (!HasTimePeriodExpired(Delay)) return;
    CurrentCommand = NextCommand;
}

//-----------------------------------------------------------------------------
// Чтение сектора для SCL файла (специальная обработка нулевой дорожки)
//-----------------------------------------------------------------------------
void SCL_read_sector(void)
{
    if ((WD1793.TrackRegister == 0) && (WD1793.Side == 0)) // Если дорожка 0 и сторона 0
    {
        if (WD1793.SectorRegister < 10) // Секторы 1-9
        {
            int seekptr = (WD1793.SectorRegister - 1) << 8;
            for (int i = 0; i < 256; i++)
                DiskBuf[i] = track0[seekptr + i];
        }
        else if (WD1793.SectorRegister == 10) // Сектор 10 (boot)
        {
            for (int i = 0; i < 256; i++) 
                DiskBuf[i] = boot256[i];    
        }
        else if (WD1793.SectorRegister > 11)
        {
            for (int i = 0; i < 256; i++)
                DiskBuf[i] = 0; // Пустые секторы заполняются нулями
        }
    }       
    else // Остальные дорожки читаются из файла
    {
        f_lseek(&fileTRD, (uint32_t)((( pos - 16) << 8) + sclDataOffset) );
        res = f_read(&fileTRD, DiskBuf, BUFFERSIZE, &br);
        if (res) printError("Reading error: ", res);    
    }
}               

//-----------------------------------------------------------------------------
// Побайтовое чтение сектора (SCL версия)
//-----------------------------------------------------------------------------
void WD1793_CmdReadingSectorSCL()
{
    if(!HasTimePeriodExpired(BYTE_READ_TIME)) return; // Задержка между байтами

    // Проверка на потерю данных (если предыдущий байт не был считан)
    if(SectorPos != 0 && Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);  
    }

    if (SectorPos >= fdd.sector_size)
    {
        if(WD1793.Multiple)
        {
            if(BufferPos != 0) SCL_read_sector();
            SectorPos = 0;
            if(++WD1793.RealSector > fdd.sectors_per_track)
            {
                CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
                return;
            }
            CurrentCommand = WD1793_CmdStartReadingSector;
            return;
        }
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
        return;
    }
                
    WD1793.DataRegister = DiskBuf[BufferPos]; // Выдача байта в регистр данных
    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ);     // Установка запроса данных
    BufferPos++;
    SectorPos++;
}

//-----------------------------------------------------------------------------
// Побайтовое чтение сектора (TRD версия)
//-----------------------------------------------------------------------------
void WD1793_CmdReadingSectorTRD()
{
    if(!HasTimePeriodExpired(BYTE_READ_TIME)) return; // Задержка 32us

    if(SectorPos != 0 && Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);  
    }

    if (SectorPos >= fdd.sector_size)
    {
        if(WD1793.Multiple)
        {
            if(BufferPos != 0)
            {
l_povtor:       
                res = f_read(&fileTRD, DiskBuf, fdd.sector_size, &br);
                if (res) goto l_povtor; // Повтор при ошибке чтения
            }
            SectorPos = 0;
            if(++WD1793.RealSector > fdd.sectors_per_track)
            {
                CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
                return;
            }
            CurrentCommand = WD1793_CmdStartReadingSector;
            return;
        }
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
        return;
    }
                
    WD1793.DataRegister = DiskBuf[BufferPos];
    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ);     // Установка запроса данных
    BufferPos++;
    SectorPos++;
}

//-----------------------------------------------------------------------------
// Чтение сектора для FDI формата (с поддержкой защиты и CRC)
//-----------------------------------------------------------------------------
bool FDI_read_sector(uint8_t track, uint8_t side, uint8_t sector_num)
{
    // Загрузка трека (если он еще не загружен)
    // gpio_put(LED_BOARD, 1); // Индикация обращения
    if (last_track != track || last_side != side) 
    {
        if (fdi_stream_load_track(&fdi_ctx, track, side) != FDI_STREAM_OK) 
            return false;
        last_track = track;
        last_side = side;
    }
    
    // Получение информации о секторе
    fdi_sector_info_t *sec = fdi_stream_get_sector(&fdi_ctx, sector_num);
    if (!sec) return false;
    fdd.sector_size = sec->size;
    fdd.sector_n = sec->n;
    fdd.sector_f = sec->flags;
    
    // Чтение данных сектора
    if (fdi_stream_read_sector_data(&fdi_ctx, sec, DiskBuf, fdd.sector_size) != FDI_STREAM_OK) 
        return false;

    // Здесь должна быть обработка флагов CRC и удаленных данных
    // В оригинале закомментирована
    
    return true;
}

//-----------------------------------------------------------------------------
// Побайтовое чтение сектора (FDI версия)
//-----------------------------------------------------------------------------
void WD1793_CmdReadingSectorFDI()
{
    if(!HasTimePeriodExpired(BYTE_READ_TIME)) return; // Задержка

    if(SectorPos != 0 && Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);  
    }

    if (SectorPos >= fdd.sector_size)
    {
        if(WD1793.Multiple)
        {
            if(BufferPos != 0) 
                FDI_read_sector(WD1793.TrackRegister, WD1793.Side, WD1793.SectorRegister);
            SectorPos = 0;
            if(++WD1793.RealSector > sectors_per_track) // Если секторов больше нет
            {
                CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
                return;
            }  
            CurrentCommand = WD1793_CmdStartReadingSector;
            return;
        }
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
        return;
    }        
    
    WD1793.DataRegister = DiskBuf[BufferPos]; // Чтение байта
    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ);     // Запрос данных
    BufferPos++;
    SectorPos++;
}

//-----------------------------------------------------------------------------
// Диспетчер чтения сектора (вызывает нужную функцию в зависимости от типа файла)
//-----------------------------------------------------------------------------
void WD1793_CmdReadingSector()
{
    switch (file_type[DRV])
    {
        case SCL:
            WD1793_CmdReadingSectorSCL();
            return;
        case TRD:
        case TRDS:
            WD1793_CmdReadingSectorTRD();
            return; // !!! В оригинале тут нет return и break, но так безопаснее
        case FDI:
            WD1793_CmdReadingSectorFDI();   
            return;
    }
}

//-----------------------------------------------------------------------------
// Команда записи сектора (A0/B0)
//-----------------------------------------------------------------------------
void WD1793_CmdWritingSector()
{
    if (!HasTimePeriodExpired_old(BYTE_WRITE_TIME))
        return; // Задержка 32us
    
    if (Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);
    }
    
    if (!(WD1793.StatusRegister & _BV(stsLostData)))
    {
        DiskBuf[BufferPos] = WD1793.DataRegister; // Запись байта в буфер
    }
    
    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ); // Запрос следующего байта

    // Проверка защиты от записи
    int msg_code = 0;
    if (file_type[DRV] == SCL) {
        msg_code = 10;  // SCL files Read Only!
    }
    else if (file_type[DRV] == TRDS) {
        msg_code = 10;  // TRD the file is non-standard Read Only
    }
    else if (file_type[DRV] == FDI) {
        msg_code = 10;  // FDI Read Only
    }
    else if ((file_attr[DRV] & AM_RDO) == AM_RDO) {
        msg_code = 10;  // This file is Read Only
    }

    if (msg_code != 0) {
        WD1793.StatusRegister = 0b01000000; // Установка бита защиты записи (Write Protect)
        CurrentCommand = WD1793_CmdStartIdle; // Принудительное завершение команды
        msg_bar = 10;  // Вывод сообщения "Read Only"
        wait_msg = 3000;
        return;
    }

    BufferPos++;
    SectorPos++;

    if (SectorPos >= fdd.sector_size)
    {
        BufferUpdated = 1; // Включить флаг записи на SD карту
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
    }
}

//-----------------------------------------------------------------------------
// Команда записи дорожки / форматирования (F0)
//-----------------------------------------------------------------------------
void WD1793_CmdWritingTrack() // Вызывается из WD1793_Cmd_WriteTrack()
{
    // Проверка защиты от записи
    int msg_code = 0;
    if (file_type[DRV] == SCL) {
        msg_code = 10;  // SCL files Read Only!
    }
    else if (file_type[DRV] == TRDS) {
        msg_code = 11;  // TRD the file is non-standard Read Only
    }
    else if ((file_attr[DRV] & AM_RDO) == AM_RDO) {
        msg_code = 12;  // This file is Read Only
    }

    if (msg_code != 0) {
        WD1793.StatusRegister = 0b01000000; // Защита записи
        CurrentCommand = WD1793_CmdStartIdle; // Принудительное завершение
        msg_bar = msg_code;
        wait_msg = 3000;
        return;
    }

    // Для нулевой дорожки эмулируется задержка записи
    if ((WD1793.TrackRegister == 0) && (WD1793.Side == 0))
    {
        if (!HasTimePeriodExpired_old(BYTE_WRITE_TIME)) return;           
    }

    BufferUpdated = 0; // Отключение записи на SD при форматировании

    if (Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);
        CurrentCommand = WD1793_CmdStartIdle; // Завершение при потере данных
        return;
    }

    // Подсчет количества байт 0x4E (маркер форматирования)
    if (WD1793.DataRegister == 0x4E)
        FormatCounter++;
    else
        FormatCounter = 0;

    // Если получено достаточно маркеров, считается что форматирование завершено
    if (FormatCounter > 400)
    {
        if ((WD1793.TrackRegister == 0) && (WD1793.Side == 0))
        {
            BufferUpdated = 1; // Включить запись на SD для стирания 0 дорожки
            memset(DiskBuf, 0, 256); // Очистка буфера
        }
        else
            BufferUpdated = 0; // Для остальных дорожек запись не производится
        
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
        return;
    }

    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ); // Запрос следующего байта данных
}

//-----------------------------------------------------------------------------
// Чтение адресного маркера для FDI
//-----------------------------------------------------------------------------
void ReadingAddressFDI() 
{   
    gpio_put(LED_BOARD, 1);
    uint8_t crc_bit = 1 << (fdd.sector_f & 3);
    DiskSector = (DiskSector + 1) & 0x0f;

    if((SectorPos != 0) && Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);
    }    

    if (BufferPos >= 6)
    {
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
        gpio_put(LED_BOARD, 0);
        return;
    }
        
    switch(BufferPos)
    {
        case 0: // Байт 0: Номер дорожки
            WD1793.DataRegister = WD1793.TrackRegister;  
            break;
        case 1: // Байт 1: Номер стороны
            WD1793.DataRegister = WD1793.Side;
            break;
        case 2: // Байт 2: Номер сектора
            WD1793.DataRegister = WD1793.SectorRegister;
            break;
        case 3: // Байт 3: Код размера сектора
            WD1793.DataRegister = fdd.sector_n;
            break;
        case 4: // Байт 4: CRC (младший)
            if (!(fdd.sector_f & crc_bit)) {
                WD1793.DataRegister = 0x00; // Ошибка CRC
            } else {
                WD1793.DataRegister = 0x00; 
            }
            break;
        case 5: // Байт 5: CRC (старший)
            if (!(fdd.sector_f & crc_bit)) {
                WD1793.DataRegister = 0x00; // Ошибка CRC
            } else {
                WD1793.DataRegister = 0xff; 
            }
            break;
    }
}

//-----------------------------------------------------------------------------
// Чтение адресного маркера для TRD
//-----------------------------------------------------------------------------
void ReadingAddressTRD()
{   
    DiskSector = (DiskSector + 1) & 0x0f;
    uint8_t sector_table[] = {1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15, 8, 16};

    if((SectorPos != 0) && Requests & _BV(rqDRQ))
    {
        WD1793.StatusRegister |= _BV(stsLostData);
    }    

    if (BufferPos >= 6)
    {
        CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
        return;
    }
        
    switch(BufferPos)
    {
        case 0: // Байт 0: Номер дорожки
            WD1793.DataRegister = WD1793.TrackRegister;
            break;
        case 1: // Байт 1: Сторона (всегда 0 для TR-DOS)
            WD1793.DataRegister = 0;
            break;
        case 2: // Байт 2: Номер сектора
            WD1793.DataRegister = sector_table[DiskSector]; // Выдача сектора в порядке чересстрочной развертки TR-DOS
            break;
        case 3: // Байт 3: Размер сектора (1 = 256 байт)
            WD1793.DataRegister = 1;
            break;
        case 4: // CRC 1
        case 5: // CRC 2
            WD1793.DataRegister = 0;
            break;
    }
}

//-----------------------------------------------------------------------------
// Команда чтения адреса (C0)
//-----------------------------------------------------------------------------
void WD1793_CmdReadingAddress()
{
    if(!HasTimePeriodExpired(ADDRESS_READ_TIME)) return; // Задержка 32us
 
    if (file_type[DRV] == FDI) 
        ReadingAddressFDI();
    else 
        ReadingAddressTRD();

    BufferPos++;
    Requests |= _BV(rqDRQ); // Установка запроса данных
    WD1793.StatusRegister |= _BV(stsDRQ);
}

//-----------------------------------------------------------------------------
// Команда восстановления (00) - перемещение головки на дорожку 0
//-----------------------------------------------------------------------------
void WD1793_Cmd_Restore()
{ 
    WD1793.TrackRegister = 0;
    WD1793.RealTrack = 0;
    WD1793.Direction = 0;
    NextCommand = WD1793_CmdType1Status;
    CurrentCommand = WD1793_CmdDelay;
    CmdType = 1;
}

//-----------------------------------------------------------------------------
// Команда поиска (10) - позиционирование на указанную дорожку
//-----------------------------------------------------------------------------
void WD1793_Cmd_Seek()
{
    if (WD1793.TrackRegister > WD1793.DataRegister)
        WD1793.Direction = 1; // Движение наружу (к центру? зависит от реализации)
    else
        WD1793.Direction = 0; // Движение внутрь

    WD1793.TrackRegister = WD1793.DataRegister;
    WD1793.RealTrack = WD1793.DataRegister;

    NextCommand = WD1793_CmdType1Status;
    CurrentCommand = WD1793_CmdDelay;
    CmdType = 1;
}

//-----------------------------------------------------------------------------
// Команды шага (20/30/40/50/60/70) - перемещение на 1 дорожку
//-----------------------------------------------------------------------------
void WD1793_Cmd_Step()
{ 
    switch (WD1793.CommandRegister & 0xF0)
    {
        case 0x40:
        case 0x50: // Step In (внутрь)
            WD1793.Direction = 0;
            break;
        case 0x60:
        case 0x70: // Step Out (наружу)
            WD1793.Direction = 1;
            break;
    }
    
    if (WD1793.Direction == 0 && WD1793.TrackRegister < 128)
    {
        WD1793.TrackRegister++;
    }
    if (WD1793.Direction == 1 && WD1793.TrackRegister > 0)
    {
        WD1793.TrackRegister--;
    }

    NextCommand = WD1793_CmdType1Status;
    CurrentCommand = WD1793_CmdDelay;
    CmdType = 1;
}

//-----------------------------------------------------------------------------
// Подготовка к чтению сектора (установка параметров и запуск побайтового чтения)
//-----------------------------------------------------------------------------
void WD1793_CmdStartReadingSector()
{
    // Установка сообщения в статус баре
    // msg_bar = 16; // READ TR:
    // wait_msg = 3000; 
    CurrentCommand = WD1793_CmdReadingSector;
}

//-----------------------------------------------------------------------------
// Команда чтения сектора (80/90)
//-----------------------------------------------------------------------------
void WD1793_Cmd_ReadSector()
{ 
    switch (file_type[DRV])
    {    
        case SCL:
            WD1793.Multiple = WD1793.CommandRegister & 0x10;
            pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, WD1793.Side);
            WD1793.RealSector = WD1793.SectorRegister;
            BufferPos = 0; // Сброс позиции в буфере
            SetDiskPositionSCL(pos);
            SectorPos = 0;
            CurrentCommand = WD1793_CmdStartReadingSector;
            CmdType = 2;
            return;

        case TRD:
        case TRDS:
            WD1793.Multiple = WD1793.CommandRegister & 0x10;
            pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, WD1793.Side);
            WD1793.RealSector = WD1793.SectorRegister;
            BufferPos = 0; // Сброс позиции в буфере
            SetDiskPosition(pos);
            SectorPos = 0;
            CurrentCommand = WD1793_CmdStartReadingSector;
            CmdType = 2;
            return;

        case FDI:
            WD1793.Multiple = WD1793.CommandRegister & 0x10;
            WD1793.RealSector = WD1793.SectorRegister;
            BufferPos = 0; // Сброс позиции в буфере
            FDI_read_sector(WD1793.TrackRegister, WD1793.Side, WD1793.SectorRegister);
            SectorPos = 0;
            CurrentCommand = WD1793_CmdStartReadingSector;
            CmdType = 2;
            return;
    }   
}

//-----------------------------------------------------------------------------
// Подготовка к записи сектора
//-----------------------------------------------------------------------------
void WD1793_CmdStartWritingSector()
{
    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ); // Запрос первого байта данных
    CurrentCommand = WD1793_CmdWritingSector;
    // msg_bar = 15; // WRITE TR:
    // wait_msg = 3000; 
}

//-----------------------------------------------------------------------------
// Команда записи сектора (A0/B0)
//-----------------------------------------------------------------------------
void WD1793_Cmd_WriteSector()
{
    WD1793.Multiple = WD1793.CommandRegister & 0x00; // В оригинале 0x00, но обычно проверяется 0x10
    CmdType = 2;
    pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, WD1793.Side);
    BufferPos = (pos & (BUFFERSIZEFACTOR - 1)) << 8;
    SetDiskPosition(pos);
    SectorPos = 0;
    NextCommand = WD1793_CmdStartWritingSector;
    CurrentCommand = WD1793_CmdDelay;
    CmdType = 2;    
}

//-----------------------------------------------------------------------------
// Команда чтения адреса (C0)
//-----------------------------------------------------------------------------
void WD1793_Cmd_ReadAddress()
{
    BufferPos = 0;
    CurrentCommand = WD1793_CmdReadingAddress;
    CmdType = 3;
}

//-----------------------------------------------------------------------------
// Команда чтения дорожки (E0) - не реализована для TRD/SCL
//-----------------------------------------------------------------------------
void WD1793_Cmd_ReadTrack()
{
    CurrentCommand = WD1793_CmdStartIdle; // Завершение команды
    CmdType = 3;
}

//-----------------------------------------------------------------------------
// Подготовка к форматированию дорожки (запрос первого байта)
//-----------------------------------------------------------------------------
void WD1793_CmdStartWritingTrack()
{
    Requests |= _BV(rqDRQ);
    WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
    CurrentCommand = WD1793_CmdWritingTrack;
}

//-----------------------------------------------------------------------------
// Команда записи дорожки (F0)
//-----------------------------------------------------------------------------
void WD1793_Cmd_WriteTrack()
{
    BufferPos = 0;
    SectorPos = 0;
    FormatCounter = 0;
    CurrentCommand = WD1793_CmdStartWritingTrack;
    CmdType = 3;
}

//-----------------------------------------------------------------------------
// Запуск новой команды из таблицы команд
//-----------------------------------------------------------------------------
void WD1793_CmdStartNewCommand()
{
    uint8_t i = WD1793.CommandRegister >> 4;

    WD1793_FlushBuffer();
    
    // Монтирование образа только если дисковод был изменен
    if (DRV != NewDrive)
    { 
        DRV = NewDrive;
        WD1793_Reset(DRV);
    }
    
    CommandTable[i](); // Вызов соответствующей функции команды
}

/*
    Формат порта #FF (out):
    7 - x
    6 - метод записи (1 - fm, 0 - mfm)
    5 - x
    4 - выбор магнитной головки (0 - верх, 1 - низ)
    3 - загрузка головки (всегда должен быть в 1)
    2 - аппаратный сброс микроконтроллера (если 0)
    1 - номер дисковода (00 - a, 01 - b, 10 - c, 11 - d)
    0 - -/-
*/

//-----------------------------------------------------------------------------
// Главная функция эмуляции WD1793, вызывается в цикле эмуляции Z80
//-----------------------------------------------------------------------------
void WD1793_Execute(void)
{   
    // Проверка изменения порта #FF (смена диска или стороны)
    if(Prevwd1793_PortFF != wd1793_PortFF)
    {
        Prevwd1793_PortFF = wd1793_PortFF;
        NewDrive = GET_DRIVE();               // Получить номер привода (0-3)
        WD1793.Side = GET_SIDE();             // Получить сторону (0 или 1)
    }

    // Если получена новая команда
    if(NewCommandReceived)
    {
        NewCommandReceived = 0; // Сброс флага
        
        if ((WD1793.CommandRegister & 0xF0) == 0xD0) // Команда принудительного прерывания
        { 
            WD1793.StatusRegister &= ~_BV(stsBusy); // Сброс занятости
            CmdType = 1; // Исключение
            // Выбор обработчика в зависимости от бита i0 (если 0 - без прерывания)
            CurrentCommand = WD1793.CommandRegister & 0x0F ? WD1793_CmdType1Status : WD1793_CmdStartIdleForceInt0;
        } 
        else if(!(WD1793.StatusRegister & _BV(stsBusy))) // Если контроллер свободен
        {
            WD1793.StatusRegister = 0x01; // Установка бита занятости, сброс остальных
            Requests = 0;                 // Сброс запросов DRQ и INTRQ
            CurrentCommand = WD1793_CmdStartNewCommand;
        }
    }

    sound_track = WD1793.TrackRegister; // Передача номера дорожки в звуковую подсистему (для звука шагов)

    CurrentCommand(); // Выполнение текущей команды
}

//-----------------------------------------------------------------------------
// Инициализация эмуляции WD1793
//-----------------------------------------------------------------------------
void WD1793_Init(void)
{
    old_t = time_us_64() + 999; // Инициализация таймера
    wd1793_PortFF = 0b00001000;  // Начальное значение порта (бит 3 = 1: головка загружена)
    Prevwd1793_PortFF = 0xff;    // Предыдущее значение
    NewDrive = 0;
    WD1793.Side = 0;
    NoDisk = 0;
    Requests = 0;                // Сброс запросов DRQ и INTRQ
    tindex = time_us_64() + 1000; // Инициализация таймера индекса
}

#endif // TRDOS_1