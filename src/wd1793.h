#pragma once

#ifndef _WD1793_H_
#define _WD1793_H_

#include <stdint.h>
#include "stdbool.h"

//=============================================================================
// ОПРЕДЕЛЕНИЯ КОНСТАНТ
//=============================================================================

#define  SECTORLENGTH         256U    // Длина сектора TR-DOS
#define  SECTORPERTRACK       16U     // Секторов на дорожку
#define  DISKSIDES            2U      // Количество сторон диска
#define  S_SIZE_TO_CODE(x)    ((((x)>>8)|(((x)&1024)>>9)|(((x)&1024)>>10))&3)
#define  S_CODE_TO_SIZE(x)    (1<<(7+(x)))

#define  BUFFERSIZE           0x0100  // Размер буфера (256 байт)
#define  BUFFERSIZEFACTOR     (BUFFERSIZE/SECTORLENGTH)
#define  DEFULTDISKPOS        0xFFFFU // Позиция по умолчанию (вне диска)

//=============================================================================
// ВРЕМЕННЫЕ ЗАДЕРЖКИ ЭМУЛЯЦИИ
//=============================================================================
// Один такт Z80 = 0.2857142857142857 us
// boot ENLIGHT очень чувствителен к этому числу если оно маленькое!
#define  BYTE_READ_TIME       64      // Время чтения байта (в мкс или попугаях)
#define  ADDRESS_READ_TIME    32      // Время чтения адресного маркера
#define  BYTE_WRITE_TIME      96      // Время записи байта

//=============================================================================
// СТРУКТУРА ДАННЫХ ДЛЯ КОНТЕКСТА ДИСКОВОДА (FDD)
//=============================================================================
typedef struct {
    uint32_t data_offset;           // Смещение начала данных (обычно 4096 для FDI)
    uint8_t  sector_n;              // Код размера сектора
    uint32_t sector_size;           // Реальный размер сектора в байтах (из заголовка)
    uint8_t  sectors_per_track;     // Количество секторов на дорожку
    uint8_t  heads;                 // Количество головок (сторон)
    uint16_t cylinders;             // Количество цилиндров (дорожек)
    uint32_t total_sectors;         // Всего секторов в образе
    uint32_t tracks_count;          // Всего треков (cylinders * heads)
    bool     valid;                 // Флаг валидности образа FDI
    uint8_t  sector_f;              // Флаги сектора (CRC, удаленные данные)
    uint16_t extra_len;
} FDD_CTX;

extern FDD_CTX fdd;

//=============================================================================
// БИТЫ РЕГИСТРА СОСТОЯНИЯ (Status Register)
//=============================================================================
#define stsBusy            0 // Контроллер занят
#define stsDRQ             1 // Запрос данных (Data Request)
#define stsIndexImpuls     1 // Индексный импульс (совпадает с битом DRQ по номеру, но в разных контекстах)
#define stsTrack0          2 // Головка на нулевой дорожке
#define stsLostData        2 // Потеря данных (совпадает с Track0)
#define stsCRCError        3 // Ошибка контрольной суммы
#define stsSeekError       4 // Ошибка поиска дорожки
#define stsRecordNotFound  4 // Запись не найдена (совпадает с SeekError)
#define stsLoadHead        5 // Головка загружена
#define stsRecordType      5 // Тип записи (совпадает с LoadHead)
#define stsWriteError      5 // Ошибка записи (совпадает с LoadHead)
#define stsWriteProtect    6 // Защита от записи
#define stsNotReady        7 // Устройство не готово

//=============================================================================
// БИТЫ ЗАПРОСОВ ПРЕРЫВАНИЙ (Requests)
//=============================================================================
#define rqDRQ   6 // Флаг запроса данных
#define rqINTRQ 7 // Флаг прерывания

#define _BV(bit) (1 << (bit))

//=============================================================================
// СТРУКТУРА ЭМУЛЯЦИИ КОНТРОЛЛЕРА WD1793
//=============================================================================
typedef struct
{
    // Регистры контроллера (доступны для чтения/записи процессором)
    union {
        uint8_t Regs[5];
        struct {
            uint8_t StatusRegister;     // 0x1F (R) Регистр состояния
            uint8_t TrackRegister;      // 0x3F (R/W) Регистр дорожки
            uint8_t SectorRegister;     // 0x5F (R/W) Регистр сектора
            uint8_t DataRegister;       // 0x7F (R/W) Регистр данных
            uint8_t CommandRegister;    // 0x1F (W) Регистр команд
        };
    };
    
    // Внутренние переменные состояния эмуляции
    uint8_t RealSector;         // Реальный (физический) номер сектора под головкой
    uint8_t RealTrack;          // Реальный (физический) номер дорожки
    uint8_t Direction;          // Направление движения головки
    uint8_t Side;               // Текущая сторона диска
    uint8_t Multiple;           // Флаг мультисекторной операции
}  WD1793_struct;

//=============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
//=============================================================================
void WD1793_Reset(uint8_t drive);
void WD1793_Execute();
void WD1793_Init();

extern uint8_t Requests; // Байт запросов (INTRQ/DRQ)

// Инлайн функция получения состояния запросов (для быстрого доступа)
inline uint8_t WD1793_GetRequests()
{
    return Requests;
}

// Макросы для разбора порта ввода-вывода (#FF)
#define GET_DRIVE()             (wd1793_PortFF & 0b11)       // Выделить номер дисковода (A, B, C, D)
#define SIDE_PIN                4
#define GET_SIDE()              ((~wd1793_PortFF & _BV(SIDE_PIN)) >> SIDE_PIN) // Выделить сторону (0 - низ/верх в зависимости от инверсии)
#define GET_RES()               (wd1793_PortFF & _BV(RES_PIN)) // Проверить аппаратный сброс

// Команды WD1793
void WD1793_Cmd_Restore();
void WD1793_Cmd_Seek();
void WD1793_Cmd_Step();
void WD1793_Cmd_ReadSector();
void WD1793_Cmd_WriteSector();
void WD1793_Cmd_ReadAddress();
void WD1793_Cmd_ReadTrack();
void WD1793_Cmd_WriteTrack();
void WD1793_CmdStartReadingSector();

void diskindex_callback();

// Экспортируемые переменные
extern WD1793_struct WD1793;

// Функции интерфейса с Z80
uint8_t WD1793_Read(uint8_t Address);
void WD1793_Write(uint8_t Address, uint8_t Value);

// Функции работы с файлами образов
bool OpenTRDFile(char* sn, uint8_t drv);
bool OpenFDI_File(char *sn, uint8_t drv);

void SCL_read_sector(void);

#endif // _WD1793_H_