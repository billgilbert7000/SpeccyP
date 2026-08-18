// SpeccyP.h
#ifndef _SPECCY_P_H_
#define _SPECCY_P_H_

#include <stdbool.h>
#include <stdint.h>

// ===============================================================
// СОСТОЯНИЕ ЭМУЛЯЦИИ
// ===============================================================
typedef enum {
    EMU_RUNNING = 0,    // Эмуляция ZX Spectrum работает
    EMU_STOPPED = 1     // Эмуляция остановлена (меню/пауза/дизассемблер)
} EmuState;

// Объявляем глобальную переменную как extern
extern EmuState emu_state;

// Объявляем функции управления состоянием
/**
 * @brief Остановить эмуляцию (войти в режим меню)
 */
void emu_stop(void);
/**
 * @brief Запустить эмуляцию (выйти из режима меню)
 */
void emu_start(void);
/**
 * @brief Переключить состояние эмуляции
 */
void emu_toggle(void);
/**
 * @brief Временно приостановить эмуляцию с сохранением состояния
 * @return предыдущее состояние
 */
EmuState emu_suspend(void);
/**
 * @brief Восстановить состояние эмуляции
 */
void emu_restore(EmuState state);
/**
 * @brief Проверить, запущена ли эмуляция
 */
bool emu_is_running(void);
/**
 * @brief Проверить, остановлена ли эмуляция
 */
bool emu_is_stopped(void);
/**
 * @brief Проверить, только что остановлена ли эмуляция
 */
bool emu_just_stopped(void);
// ===============================================================
bool save_config(void);
 void pico_reset(void);

 typedef struct {
    const char name[24];
    uint8_t NeedPSRAM;
    int id;
} ZxMachineVariant;

#endif // _SPECCY_P_H_