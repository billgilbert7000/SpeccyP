#pragma once
#ifndef _AYSOFT_H_
#define _AYSOFT_H_

#include "inttypes.h" 
#include "pico/stdlib.h"

//==================================================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ СОСТОЯНИЯ ЗВУКОВЫХ ЧИПОВ
//==================================================================================================

// Регистры состояния AY0 (Основной чип / Левый канал Turbo Sound)
extern uint8_t reg_ay0[16];
extern uint16_t ay0_R12_R11;        // Совмещенное значение периода огибающей (Reg 11 и 12)
extern uint16_t ay0_A_freq;         // Частота тона канала A
extern uint16_t ay0_B_freq;         // Частота тона канала B
extern uint16_t ay0_C_freq;         // Частота тона канала C

// Регистры состояния AY1 (Второй чип для режимов Turbo Sound / TSFM)
extern uint8_t reg_ay1[16];
extern uint16_t ay1_R12_R11;        // Совмещенное значение периода огибающей (Reg 11 и 12)
extern uint16_t ay1_A_freq;         // Частота тона канала A (чип 1)
extern uint16_t ay1_B_freq;         // Частота тона канала B (чип 1)
extern uint16_t ay1_C_freq;         // Частота тона канала C (чип 1)

// Данные для эмуляции портов Sound Drive (Covox)
extern uint8_t SoundLeft_A;
extern uint8_t SoundLeft_B;
extern uint8_t SoundRight_A;
extern uint8_t SoundRight_B;

extern uint8_t valLoad;             // Внешняя загрузка данных для смешивания с аудио (например, Covox)

//==================================================================================================
// УКАЗАТЕЛИ НА ФУНКЦИИ ВВОДА/ВЫВОДА (Эмуляция портов ZX Spectrum)
//==================================================================================================
extern void (*AY_out_FFFD)(uint8_t);    // Указатель на функцию записи номера регистра (порт FFFD)
extern void (*AY_out_BFFD)(uint8_t);    // Указатель на функцию записи данных в регистр (порт BFFD)
extern void (*hw_beep_out)(bool);       // Указатель на функцию вывода бипера (порт FE)

// Указатель на функцию рендеринга звука, вызывается по прерыванию таймера
extern void (*audio_out)(void);

//==================================================================================================
// ПРОТОТИПЫ ФУНКЦИЙ УПРАВЛЕНИЯ AY
//==================================================================================================

// Выбор типа звуковой подсистемы (SOFT, HARD, I2S и т.д.)
void select_audio(void);   

// Установка номера регистра (устаревший интерфейс для util_z80.c)
void AY_select_reg(uint8_t N_reg);

// Чтение/запись данных в регистры программной эмуляции AY
uint8_t AY_get_reg(void);
void AY_set_reg(uint8_t val);

// Получение готовых сэмплов каналов A, B, C для программного AY (чип 0)
uint16_t* get_AY_Out(uint8_t delta);

// Аналогичный интерфейс для второго чипа (Turbo Sound)
uint8_t AY_get_reg1(void);
void AY_set_reg1(uint8_t val);
uint16_t* get_AY_Out1(uint8_t delta);

// Сброс всех аудиочипов в начальное состояние
void AY_reset(void);

// Чтение данных из регистра (эмуляция IN FFFD)
uint8_t __not_in_flash_func(AY_in_FFFD)(void);

// Управление питанием/активностью железных AY (отключение/включение)
void hardAY_off(void);
void hardAY_on(void);

// Установка громкости
void init_vol_ay(void);
void set_audio_buster(void);

// Заглушки для совместимости
void ay_mute(void);
void ay_reinit(void);

//==================================================================================================
// МАКРОСЫ КОМАНД ДЛЯ СДВИГОВОГО РЕГИСТРА 74HC595 (Управление железным AY)
// Формат посылки: *R*B10WA--------
// R - RESET, B - BEEP, 1/0 - Chip Select, W - WR, A - A0
//==================================================================================================

// Общие команды
#define AY_Z      0b0100110000000000  // Оба чипа в Z-состоянии (неактивны)
#define FM_Z      0b0100111100000000  // Z-состояние для FM (TSFM)
#define AY_RES    0b0000001100000000  // Сброс (Reset)

// Команды для Чипа 0 (Основной)
#define AY0_Z     0b0100100000000000  // Активен чип 0 (Z-состояние снято)
#define AY0_WRITE 0b0100101000000000  // Запись данных в регистр (WR=1, A0=0)
#define AY0_FIX   0b0100101100000000  // Фиксация адреса регистра (WR=1, A0=1)

// Команды для Чипа 1 (Turbo Sound)
#define AY1_Z     0b0100010000000000  // Активен чип 1
#define AY1_WRITE 0b0100011000000000  // Запись данных в регистр чипа 1
#define AY1_FIX   0b0100011100000000  // Фиксация адреса регистра чипа 1

//==================================================================================================
// ПРОТОТИПЫ НИЗКОУРОВНЕВЫХ ФУНКЦИЙ ДЛЯ РАБОТЫ С ЖЕЛЕЗОМ
//==================================================================================================

// Инициализация PIO и сдвигового регистра 595
void Init_AY_595(void);

// Генератор случайного шума (LFSR)
bool __not_in_flash_func(get_random)(void);

// Отправка данных в сдвиговые регистры
void __not_in_flash_func(send595)(uint16_t comand, uint8_t data);
void __not_in_flash_func(send595_0)(uint16_t comand, uint8_t data);
void __not_in_flash_func(send595_1)(uint16_t comand, uint8_t data);

// Функции для работы с TSFM (подготовка)
void __not_in_flash_func(set_reg_tsfm0)(uint8_t data);
void __not_in_flash_func(set_reg_tsfm1)(uint8_t data); 
void __not_in_flash_func(set_data_tsfm0)(uint8_t data); 
void __not_in_flash_func(set_data_tsfm1)(uint8_t data); 

// Управление бипером через 595 регистр
void out_beep595(bool val);

// Различные реализации вывода бипера
void hw_out595_beep_out(bool val);
void hw_outpin_beep_out(bool val);
void hw_outi2s_beep_out(bool val);

// Отправка данных в чип SAA1099
void __not_in_flash_func(saa1099_write)(uint8_t addr, uint16_t byte);

#endif // _AYSOFT_H_