#pragma once
#ifndef ZX_MASHINE_H_
#define ZX_MASHINE_H_

#include <Z/constants/pointer.h> /* Z_NULL */
#include "Z80.h"

#include "inttypes.h"
// функции, которые надо определить аппаратно
#include "util.h"

bool hw_zx_get_bit_LOAD();
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

void zx_machine_flashATTR(void);
void zx_generator_int (void);
// void zx_machine_set_vbuf(uint8_t* vbuf);
void zx_machine_enable_vbuf(bool en_vbuf);

void fast(zx_machine_set_7ffd_out)(uint8_t val);
uint8_t zx_machine_get_7ffd_lastOut();
 //uint32_t zx_RAM_bank_active;
 //uint8_t zx_RAM_bank_7ffd;


// запись в память 128 стандарт
//inline static void fast(write_z80)( uint16_t addr, uint8_t val);


// volatile z80 cpu;
//uint8_t *zx_ram_bank[8]; // Хранит адреса 8ми банков памяти
//uint8_t *zx_cpu_ram[4];  // Адреса 4х областей памяти CPU при использовании страниц uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))

//static bool trdos;
//uint8_t VG_PortFF;



void init_mashine_and_extram(uint8_t conf);

void zx_machine_enable_vbuf(bool en_vbuf);

 //uint8_t rom_n;
 //uint8_t rom_n1;
void rom_select(void);

uint8_t zx_machine_get_zx_RAM_bank_active();
uint8_t zx_machine_get_1ffd_lastOut();
uint8_t zx_machine_get_rom();




// Пентагон с кеш
//static
 //uint8_t cash_f=0;
// Дизассемблер
// uint16_t address_pc;
//char tmp_opcode[16];

void disassembler(void); // 
uint8_t opcode_z80(void);
//uint16_t dis_adres;
void list_disassm(void);
void list_dump(void);
int OpcodeLen( uint16_t p );// calculate the length of an opcode


void turbo_switch(void); 



// Настройки и функции для эмулятора Z80 REDCODE

 typedef struct {
        void* context;
        zuint8 (* read)(void *context);
        void (* write)(void *context, zuint8 value);
        zuint16 assigned_port;
} Device; 
//
 typedef struct {
        zusize  cycles;
        Z80     cpu;
        Device* devices;
        zusize  device_count;
} Machine; 
//




extern Machine machine;
extern Machine* z1;

void select_cpu_z80(Machine *self);



 //  Machine machine;  // Создаем экземпляр машины
 //  Machine* z1 = &machine;  // Создаем указатель на машину
//


//void fast(write_z80)(Machine *self, uint16_t addr, uint8_t val);
 

/*
 // Макросы для регистров Z80
#define PC      Z80_PC(z1->cpu)
#define SP      Z80_SP(z1->cpu)
#define AF      Z80_AF(z1->cpu)
#define BC      Z80_BC(z1->cpu)
#define DE      Z80_DE(z1->cpu)
#define HL      Z80_HL(z1->cpu)
#define IX      Z80_IX(z1->cpu)
#define IY      Z80_IY(z1->cpu)
#define MEMPTR  Z80_MEMPTR(z1->cpu)

// Макросы для 8-битных регистров
#define A       Z80_A(z1->cpu)
#define F       Z80_F(z1->cpu)
#define B       Z80_B(z1->cpu)
#define C       Z80_C(z1->cpu)
#define D       Z80_D(z1->cpu)
#define E       Z80_E(z1->cpu)
#define H       Z80_H(z1->cpu)
#define L       Z80_L(z1->cpu)

// Макросы для альтернативных регистров
#define AF_     Z80_AF_(z1->cpu)
#define BC_     Z80_BC_(z1->cpu)
#define DE_     Z80_DE_(z1->cpu)
#define HL_     Z80_HL_(z1->cpu)
#define A_      Z80_A_(z1->cpu)
#define F_      Z80_F_(z1->cpu)
#define B_      Z80_B_(z1->cpu)
#define C_      Z80_C_(z1->cpu)
#define D_      Z80_D_(z1->cpu)
#define E_      Z80_E_(z1->cpu)
#define H_      Z80_H_(z1->cpu)
#define L_      Z80_L_(z1->cpu)

// Макросы для частей регистров
#define PCH     Z80_PCH(z1->cpu)
#define PCL     Z80_PCL(z1->cpu)
#define SPH     Z80_SPH(z1->cpu)
#define SPL     Z80_SPL(z1->cpu)
#define IXH     Z80_IXH(z1->cpu)
#define IXL     Z80_IXL(z1->cpu)
#define IYH     Z80_IYH(z1->cpu)
#define IYL     Z80_IYL(z1->cpu)

// Специальные регистры
#define I       (z1->cpu.i)
//#define R       z80_r(&z1->cpu) //???


// Полный код для доступа к IFF, IM и другим регистрам состояния:
#define IFF1    (z1->cpu.iff1)
#define IFF2    (z1->cpu.iff2)
#define IM      (z1->cpu.im)       // режим прерываний (0,1,2)
#define I       (z1->cpu.i)        // регистр I
#define R       z80_r(&z1->cpu)    // регистр R (используйте функцию)
#define Q       (z1->cpu.q)        // Q-фактор


//##############################################

*/


#endif