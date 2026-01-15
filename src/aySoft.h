#pragma once
#ifndef _AYSOFT_H_
#define _AYSOFT_H_


#include "inttypes.h" 
#include "pico/stdlib.h"

//регистры состояния AY0
extern  uint8_t reg_ay0[16];
extern  uint16_t ay0_R12_R11;
//uint8_t ay0_R7_m;// копия регистра смесителя
extern  uint16_t ay0_A_freq;
extern  uint16_t ay0_B_freq;
extern  uint16_t ay0_C_freq;

// регистры состояния AY1
extern  uint8_t reg_ay1[16];
extern  uint16_t ay1_R12_R11;
//uint8_t ay1_R7_m; // копия регистра смесителя
extern  uint16_t ay1_A_freq;
extern  uint16_t ay1_B_freq;
extern  uint16_t ay1_C_freq;


void select_audio(void);   
extern void (*audio_out)(void);

void AY_select_reg(uint8_t N_reg);

uint8_t  AY_get_reg();
void (AY_set_reg(uint8_t val));

uint16_t*  get_AY_Out(uint8_t delta);
//==============================================
void AY_select_reg1 (uint8_t N_reg);
uint8_t AY_get_reg1();
void AY_set_reg1(uint8_t val);
uint16_t*  get_AY_Out1(uint8_t delta);
//==============================================

 void  AY_reset();
 uint8_t __not_in_flash_func(AY_in_FFFD)(void);

 void hardAY_off();
 void hardAY_on();

 void init_vol_ay(void);
 void set_audio_buster(void);

 void ay_mute(void);
 void ay_reinit(void);

extern void (*AY_out_FFFD)(uint8_t);// определение указателя на функцию
extern void (*AY_out_BFFD)(uint8_t);  // определение указателя на функцию
extern void (*hw_beep_out)(bool); // определение указателя на функцию  

//------------------------
// данные портов SOUND DRIVE
extern uint8_t SoundLeft_A;
extern uint8_t SoundLeft_B;
extern uint8_t SoundRight_A;
extern uint8_t SoundRight_B;

//==================================================================================================
//                    0Rsb00IB
//   #define AY_Z     0b01000000 //неактивен 


// all chip AY
#define AY_Z      0b0100110000000000 //неактивны оба чипа
#define FM_Z      0b0100111100000000 //неактивны оба чипа
#define AY_RES    0b0000001100000000// reset
// chip AY 1
#define AY0_Z     0b0100100000000000 // 0x4800 активен чип 0
#define AY1_Z     0b0100010000000000 // 0x4400 активен чип 1
//#define AY1_READ  0b0100010100000000// чтение 
#define AY1_WRITE 0b0100011000000000// запись
#define AY1_FIX   0b0100011100000000// фиксация адреса
//#define AY1_RES   0b0000011100000000// reset

// chip AY 0
#define AY0_Z     0b0100100000000000 // 0x4800 активен чип 0
//#define AY0_READ  0b0100100100000000// чтение 
#define AY0_WRITE 0b0100101000000000// запись
#define AY0_FIX   0b0100101100000000// фиксация адреса
#define AY0_RES   0b0000101100000000// reset

//--------------------------------------------------------------------------------------------------

void Init_AY_595();

bool __not_in_flash_func(get_random)();

void __not_in_flash_func(send595)(uint16_t comand,uint8_t data);
void __not_in_flash_func(send595_0)(uint16_t comand, uint8_t data);
void __not_in_flash_func(send595_1)(uint16_t comand, uint8_t data);
void __not_in_flash_func(set_reg_tsfm0)(uint8_t data);
void __not_in_flash_func(set_reg_tsfm1)(uint8_t data); 
void __not_in_flash_func(set_data_tsfm0)(uint8_t data); 
void __not_in_flash_func(set_data_tsfm1)(uint8_t data); 

void out_beep595(bool val);

/* void (*hw_beep_out)(bool val); // определение указателя на функцию */
void (hw_out595_beep_out)(bool val);
void (hw_outpin_beep_out)(bool val);
void (hw_outi2s_beep_out)(bool val);

//void init_i2s_sound();


 void __not_in_flash_func(saa1099_write)(uint8_t addr, uint16_t byte);

extern uint8_t valLoad;
 #endif