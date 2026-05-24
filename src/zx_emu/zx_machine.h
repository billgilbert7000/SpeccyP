#pragma once
#ifndef ZX_MASHINE_H_
#define ZX_MASHINE_H_

#include <stddef.h>
#ifndef Z_NULL
#define Z_NULL NULL
#endif
#include "Z80.h"

#include "inttypes.h"

#include "util.h"
#include "disassembler.h"

// функции, которые надо определить аппаратно
bool fast(hw_zx_get_bit_LOAD)();
//void hw_zx_set_snd_out(bool val);

// количество бит на пиксел 4 или 8
#define ZX_BPP (4)
// количество графических буферов 1-я, 2-я, 3-я буферизация(2-я уменьшает FPS)
#define ZX_NUM_GBUF (1)

// размеры экрана для отрисовки
#define ZX_SCREENW (320)
#define ZX_SCREENH (240)
////цвета спектрума в формате 6 бит
extern uint8_t zx_color[];

// буфер для отрисовки
uint8_t *fast(zx_machine_screen_get)(uint8_t *current_screen);

// ввод сперктрума
typedef struct ZX_Input_t
{
    uint8_t kb_data[8];
    uint8_t kempston;
} ZX_Input_t;
 
void zx_machine_input_set(ZX_Input_t *input_data);

// работа со звуком - функции реального времени

// функции управления zx машиной
void zx_machine_reset(uint8_t rom_x);
void zx_machine_init();
void fast(zx_machine_main_loop_start)(); // функция содержит бесконечный цикл

void fast(zx_machine_flashATTR)(void);

// void zx_machine_set_vbuf(uint8_t* vbuf);
void zx_machine_enable_vbuf(bool en_vbuf);

void fast(zx_machine_set_7ffd_out)(uint8_t val);
uint8_t zx_machine_get_7ffd_lastOut();

void init_mashine_and_extram(uint8_t conf);

void zx_machine_enable_vbuf(bool en_vbuf);

void fast(rom_select)(void);

uint8_t zx_machine_get_zx_RAM_bank_active();
uint8_t zx_machine_get_1ffd_lastOut();
uint8_t zx_machine_get_rom();


void disassembler(void); // 
uint8_t opcode_z80(void);

void list_disassm(void);
void list_dump(void);
int OpcodeLen( uint16_t p );// calculate the length of an opcode


void turbo_switch(void); 

// Глобальные переменные состояния машины (нужны для дизассемблера и других модулей)
extern uint8_t zx_7ffd_lastOut;
extern uint8_t zx_1ffd_lastOut;
extern uint8_t zx_0000_lastOut;
extern uint8_t zx_aff7_lastOut;
extern uint8_t rom;
extern uint32_t zx_RAM_bank_active;
extern uint8_t cash_f;

zuint8 fast(read_memory)(Z80 *cpu, zuint16 addr);

// Настройки и функции для эмулятора Z80 REDCODE

//===========================================================================
// Z80 - один экземпляр для Spectrum
//===========================================================================
extern Z80 cpu_zx;      // Z80 для Spectrum

void select_cpu_z80(Z80 *cpu);
void zx_cpu_init(Z80 *cpu);

#endif