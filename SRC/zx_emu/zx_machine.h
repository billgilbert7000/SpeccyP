#pragma once
#include "inttypes.h"
// функции, которые надо определить аппаратно

void hw_zx_set_snd_out(bool val);

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
uint8_t *zx_machine_screen_get(uint8_t *current_screen);

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
void zx_machine_main_loop_start(); // функция содержит бесконечный цикл
void zx_machine_flashATTR(void);
void zx_machine_int (void);
// void zx_machine_set_vbuf(uint8_t* vbuf);
void zx_machine_enable_vbuf(bool en_vbuf);

void zx_machine_set_7ffd_out(uint8_t val);
uint8_t zx_machine_get_7ffd_lastOut();
uint8_t zx_RAM_bank_active;
// volatile z80 cpu;
uint8_t *zx_ram_bank[8]; // Хранит адреса 8ми банков памяти
uint8_t *zx_cpu_ram[4];  // Адреса 4х областей памяти CPU при использовании страниц uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))

bool trdos;
uint8_t VG_PortFF;

void init_extram(uint8_t conf);

void zx_machine_enable_vbuf(bool en_vbuf);

uint8_t rom_n;
uint8_t rom_n1;
void rom_select(void);