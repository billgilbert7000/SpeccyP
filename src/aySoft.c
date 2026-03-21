#include "config.h"  
#include "stdbool.h"
#include "hw_util.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "inttypes.h"
#include "hardware/pio.h"
#include <string.h>
#include "aySoft.h"
#ifndef GENERAL_SOUND
#include "audio_i2s.h"
#endif

void (*AY_out_FFFD)(uint8_t);// определение указателя на функцию
void (*AY_out_BFFD)(uint8_t);  // определение указателя на функцию
void (*hw_beep_out)(bool); // определение указателя на функцию  
uint8_t valLoad;

 uint8_t beep_data;
 uint8_t beep_data_old;
uint8_t hardAY_on_off;

#ifdef  GENERAL_SOUND     

#include "picobus/picobus.h"
#include "gs_picobus.h"

uint16_t beepPWM;

/* регулировка громкости*/
void init_vol_ay(void)
{
 while (picobus_busy) { busy_wait_us(100);} // ожидание свободной шины picobus
 PBUS_CS_0;
 if (conf.vol_i2s>100) conf.vol_i2s=100;//!!!!!   
 const   uint8_t init_msg[] = { SERVICE_COMMAND, TS_VOLUME , conf.vol_i2s}; 
  send_buffer(init_msg, sizeof(init_msg));
 PBUS_CS_1;
}
/* регулировка усилителя*/
void set_audio_buster(void)
{
  while (picobus_busy) { busy_wait_us(100);} // ожидание свободной шины picobus  
  PBUS_CS_0;
  const  uint8_t init_msg[] = { SERVICE_COMMAND, TS_BUSTER , conf.audio_buster}; 
  send_buffer(init_msg, sizeof(init_msg));
  PBUS_CS_1;
//  busy_wait_us(10);// задержка для стабильности

}

 
//    BEEP 
void __not_in_flash_func(gsp_beep_out)(bool val)
{
    static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	beep_data_old=beep_data;
  //  beepPWM = out ;
    gpio_put(BEEP_PIN,val);
}

// установки для работы звука GS
 void select_audio(void)
     {
        conf.type_sound=I2S_AY;

        hw_beep_out = gsp_beep_out;

        init_vol_ay();  // установка громкости AY
        set_audio_buster();// установка усилителя i2s
   } 
//--------------------------------------------------------------
     void AY_reset()
     {
    while (picobus_busy) { busy_wait_us(10);} // ожидание свободной шины picobus      
    PBUS_CS_0;
    const uint8_t init_msg[] = { SERVICE_COMMAND, TS_RESET };  
    send_buffer(init_msg, sizeof(init_msg));
    PBUS_CS_1;
     }

uint8_t  __not_in_flash_func(AY_in_FFFD)()
{ 
         /*Заглушка при GS*/
}

// чтение из регистров AY
uint8_t fast(AY_get_reg)()
{
 /*Заглушка при GS*/
}

// запись в регистры AY
void fast(AY_set_reg)(uint8_t val)
    {
 /*Заглушка при GS*/
    }



// оставлено для старых подпрограмм в util_z80.c
void AY_select_reg(uint8_t N_reg) 
{    /*Заглушка при GS*/
     } 

// выключение звука 
    void hardAY_off()    
{    
    while (picobus_busy) { busy_wait_us(10);} // ожидание свободной шины picobus  
    PBUS_CS_0;
    const uint8_t init_msg[] = { SERVICE_COMMAND, MUTE_GLOBAL , 0x01}; 
    send_buffer(init_msg, sizeof(init_msg));
    PBUS_CS_1;
     } 

// включение звука железных AY
    void hardAY_on()
{  
    while (picobus_busy) { busy_wait_us(100);} // ожидание свободной шины picobus  
    PBUS_CS_0;
    const uint8_t init_msg[] = { SERVICE_COMMAND, MUTE_GLOBAL , 0x00}; 
    send_buffer(init_msg, sizeof(init_msg));
    PBUS_CS_1;
} 


#endif




#ifndef  GENERAL_SOUND  




 uint16_t ay595;
 uint16_t beep595;


 uint16_t beepPWM;
 

 //uint8_t type_sound;
 //uint8_t type_sound_out;
//регистры состояния AY0
 uint8_t reg_ay0[16];
 uint16_t ay0_R12_R11;
//uint8_t ay0_R7_m;// копия регистра смесителя
 uint16_t ay0_A_freq;
 uint16_t ay0_B_freq;
 uint16_t ay0_C_freq;

// регистры состояния AY1
 uint8_t reg_ay1[16];
 uint16_t ay1_R12_R11;
//uint8_t ay1_R7_m; // копия регистра смесителя
 uint16_t ay1_A_freq;
 uint16_t ay1_B_freq;
 uint16_t ay1_C_freq;
 uint8_t maskAY[16] = {0xff,0x0f,0xff,0x0f,0xff,0x0f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};
 //const uint8_t maskAY[16] = {0xff,0x0f,0xff,0x0f,0xff,0x0f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};
 //const uint8_t maskAY_FM[16] = {0xff,0x1f,0xff,0x1f,0xff,0x1f ,0x1f, 0xff, 0x1f,0x1f,0x1f, 0xff , 0xff, 0x0f,0xff,0xff};

    uint16_t *AY_data;
	uint16_t *AY_data1;
	 uint16_t outL = 0;
	 uint16_t outR = 0;


	 uint inx;
     void (*audio_out)(void);

     uint8_t N_sel_reg;
     uint8_t N_sel_reg_1;
    

bool chip_ay;

bool audio_flag;
 uint16_t vol_soft_ay;
 uint16_t vol;
 uint16_t vol_beep;
 
uint16_t ay595;
uint16_t beep595;



//uint16_t ampls_AY[32];
// В RAM для скорости 
//const uint8_t ampls_AY_table[16] __attribute__((section(".data"))) = {0,1,1,1,2,2,3,5,6,9,13,17,22,29,36,45};/*AY_TABLE*/
const uint8_t const_ampls_AY_table[16] __attribute__((section(".data"))) = {0,1,1,1,2,2,3,5,6,9,13,17,22,29,36,45};/*AY_TABLE*/

uint16_t ampls_AY_table[16];/*AY_TABLE*/
/* const uint8_t ampls[7][16] =
{
   {0,1,1,1,2,2,3,5,6,9,13,17,22,29,36,45},/*AY_TABLE
   {0,1,2,3,4,5,6,8,10,12,16,19,23,28,34,40},/*снятя с китайского чипа
   {0,3,5,8,11,13,16,19,21,24,27,29,32,35,37,40},/*линейная
   {0,10,15,20,23,27,29,31,32,33,35,36,38,39,40,40},/*выгнутая
   {0,1,2,2,3,3,4,6,7,9,12,15,19,25,32,41},/*степенная зависимость
   {0,2,4,5,7,8,9,11,12,14,16,19,23,28,34,40},/*гибрид линейной и китайского чипа
   {0,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31},/*Rampa_AY_table
}; */
// В RAM для скорости 
const uint8_t Envelopes_const[16][32] __attribute__((section(".data"))) =
{
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*0*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*1*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*2*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*3*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*4*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*5*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*6*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*7*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },/*8*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },/*9*/
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 }, /*10 0x0a */
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },/*11  0x0b */
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },/*12  0x0c*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },/*13  0x0d*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },/*14  0x0e*/
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }/*15 0x0d*/
};  
 //----------------------------------   


 //----------------------------------   
//uint8_t N_sel_reg=0;
//uint8_t N_sel_reg_1=0;
/*
N регистра	Назначение или содержание	Значение	
0, 2, 4 	Нижние 8 бит частоты тона А, В, С 	0 - 255
1, 3, 5 	Верхние 4 бита частоты тона А, В, С 	0 - 15
6 	        Управление частотой генератора шума 	0 - 31
7 	        Управление смесителем и вводом/выводом 	0 - 255
8, 9, 10 	Управление амплитудой каналов А, В, С 	0 - 15
11 	        Нижние 8 бит управления периодом пакета 	0 - 255
12 	        Верхние 8 бит управления периодом пакета 	0 - 255
13 	        Выбор формы волнового пакета 	0 - 15
14, 15 	    Регистры портов ввода/вывода 	0 - 255

R7

  7 	  6 	  5 	  4 	  3 	  2 	  1 	  0
порт В	порт А	шум С	шум В	шум А	тон С	тон В	тон А
управление
вводом/выводом	выбор канала для шума	выбор канала для тона
*/
//############################################################################
//Оптимизированный 32-битный LFSR time_us_32(); // Используем время как seed
uint32_t __not_in_flash_func(noise_state) = 0xACE1u;

bool __not_in_flash_func(get_random)()
{
    // Качественный 32-битный LFSR с периодом ~4 миллиарда
    noise_state = (noise_state >> 1) ^ (-(noise_state & 1u) & 0xD0000001u);
    return noise_state & 0x80000000; // Старший бит для лучшей случайности
}
//###########################################################################
/* GenNoise (c) Hacker KAY & Sergey Bulba */

/* bool __not_in_flash_func(get_random1)()
{
    static uint32_t Cur_Seed = 0xffff;
    Cur_Seed = (Cur_Seed * 2 + 1) ^
               (((Cur_Seed >> 16) ^ (Cur_Seed >> 13)) & 1);
    if ((Cur_Seed >> 16) & 1)
        return true;
    return false;
} */

/*  GenNoise     */
/* uint8_t __not_in_flash_func(get_random_noise)(void)
{
    static uint32_t Cur_Seed = 0xffff;
    Cur_Seed = (Cur_Seed * 2 + 1) ^
               (((Cur_Seed >> 16) ^ (Cur_Seed >> 13)) & 1);
        return (uint8_t)Cur_Seed&0x1f;
} */

//---------------------------------------------------------------- 
 //  void fast(AY_out_FFFD)(uint8_t N_reg) 
//----------------------------------------------------------------
void fast(ay_set_reg_soft)(uint8_t N_reg) 
{ 
     N_sel_reg=N_reg; 
}
//----------------------------------------------------------------
void fast (ay_set_reg_soft_ts)(uint8_t N_reg) 
{
    if (N_reg == 0xff) {chip_ay = 0;return;} 
    if (N_reg == 0xfe) {chip_ay = 1;return;}
    if (chip_ay==0) {N_sel_reg=N_reg;} // если 0 то чип 0
    else N_sel_reg_1=N_reg; // если 1 то регистр чипа 1
}
//================================================================
// HARD AY
//================================================================ 
// запись адреса регистра bdir 1 bc 1  WR=1 A0=1  // WRITE REGISTER NUMBER
void fast(ay_set_reg_hard)(uint8_t N_reg) 
{      N_sel_reg=N_reg;  // только чип 0
// адрес регистра чипа 0
//                            *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
PIO_AY595->txf[SM_AY595] = (0b0100101100000000  | N_sel_reg | beep595) << 16;// фиксация адреса 
//                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
PIO_AY595->txf[SM_AY595] = (0b0100100000000000  | N_sel_reg | beep595) << 16;// чип 0 Z состояние   
}   
//===========================================================
// передача данных  // hard AY // WRITE REGISTER DATA // bdir 1 bc 0
// out BFFD,(val)
void fast(ay_set_data_hard)(uint8_t val)
{// только чип 0
        AY_set_reg(val);       
// данные регистра чип 0
//                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
PIO_AY595->txf[SM_AY595] = (0b0100101000000000  | val| beep595) << 16;
//                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
PIO_AY595->txf[SM_AY595] = (0b0100100000000000  | val | beep595) << 16;//чип 0 z состояние */
}
//================================================================
// END HARD AY
//================================================================ 
//================================================================
// HARD TS
//================================================================
// запись адреса регистра bdir 1 bc 1  WR=1 A0=1  // WRITE REGISTER NUMBER
    void fast(ay_set_reg_hard_ts)(uint8_t N_reg) 
{   if (N_reg == 0xff) {chip_ay = 0;return;} 
    if (N_reg == 0xfe) {chip_ay = 1;return;}
    
    if (chip_ay==0) {N_sel_reg=N_reg; } // если 0 то чип 0
    else N_sel_reg_1=N_reg; // если 1 то регистр чипа 1
        if (chip_ay == 0) // если 0 то чип 0
    {
// адрес регистра чипа 0
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100101100000000  | N_sel_reg | beep595) << 16;// фиксация адреса 
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100100000000000  | N_sel_reg  | beep595) << 16;// чип 0 Z состояние
    }
    else // если 1 то регистр чип 1
    {
//  адрес регистра чип 1
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100011100000000  | N_sel_reg_1 | beep595) << 16;// фиксация адреса
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100010000000000  |  N_sel_reg_1 | beep595) << 16;// чип 1 Z состояние
    }
}   
//----------------------------------------------------------------
// передача данных  // hard TS // bdir 1 bc 0    WR=1 A0=0
// out BFFD,(val)
void fast(ay_set_data_hard_ts)(uint8_t val)
{ 
    if (chip_ay == 0) // если 0 то чип 0
    {
        AY_set_reg(val);
    // данные регистра чип 0
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100101000000000  | val| beep595) << 16;
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100100000000000  | val | beep595) << 16;//чип 0 z состояние
    }
    else // если 1 то регистр чип 1
    {
        AY_set_reg1(val);
    // данные регистра чип 1
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100011000000000  | val | beep595) << 16;
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100010000000000  | val |  beep595) << 16;// чип 1 z состояние
    };
}
//================================================================
// END TS
//================================================================
//===============================================================
// TSFM
//===============================================================
// установка регистра // заготовка для TSFM
// out FFFD ,(val)
								/*	1<<0	выбор чипа модуля TS
									1<<1	статус 0 - чтение статуса 1 - нет чтения статуса
									1<<2	0 - включение  FM 1 - выключение FM
									1<<3	задумано под SAA1099
								*/
uint8_t TSFM_MaskReadyBit=0xff;		// 0x7f NotReady , 0xff ready
void fast(ay_set_reg_tsfm)(uint8_t N_reg) // Запись номера регистра TSFM
{ 
    if(N_reg>=0xf0){
        if ((N_reg & 0x01) == 1) {chip_ay = 0;} //запись в виртуальный регистр TS 0xff и выход // 0xff
        if ((N_reg & 0x01) == 0) {chip_ay = 1;}// запись в виртуальный регистр TS 0xff и выход // 0xfe
        if ((N_reg & 0x02) == 0) {TSFM_MaskReadyBit = 0x7f;}else{TSFM_MaskReadyBit = 0xff;}  // read status enable // 
        return;
        }
     //   if (N_reg > 0x1f) {TSFM_MaskReadyBit = 0x7f;}  // read status enable // 

    if (chip_ay == 0) // если 0 то чип 0
        {
            N_sel_reg=N_reg;
////////////////////////////////////////////////////////////////////////////////
// адрес регистра чипа 0  WR=0 A0=0 CS=0 запись при WR=0->1 или CS=0->1
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100100000000000  | N_sel_reg | beep595) << 16;// WR=0 A0=0
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100111000000000  | N_sel_reg  | beep595) << 16;// WR=1 CS=1 A0=0
///////////////////////////////////////////////////////////////////////////////        
        }

        else // если 1 то регистр чипа 1
        {
            N_sel_reg_1=N_reg;
            //set_reg_tsfm1(N_sel_reg_1);
//  адрес регистра чип 1 WR=0 A0=0 CS=0 запись при WR=0->1 или CS=0->1
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100010000000000  | N_sel_reg_1 | beep595) << 16;// WR=0 A0=0
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100111000000000  |  N_sel_reg_1 | beep595) << 16;// WR=1 CS=1 A0=0
        }
   // busy_wait_us(12);    
}  
//==============================================================================================
// передача данных  // заготовка для TSFM
// out BFFD,(val)
void fast(ay_set_data_tsfm)(uint8_t val)
{
    if (chip_ay == 0) // если 0 то чип 0
    { 
      // set_data_tsfm0(N_sel_reg);    
       AY_set_reg(val & TSFM_MaskReadyBit);// для эмуляции чтения регистров
// данные регистра чип 0 WR=0 A0=1 CS=0 запись при WR=0->1 или CS=0->1
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100100100000000  | val| beep595) << 16;// WR=0 A0=1
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100111100000000  | val | beep595) << 16;//// WR=1 CS=1 A0=1      
    }

    else // если 1 то регистр чипа 1
    {
    //set_data_tsfm1(N_sel_reg_1); 
     AY_set_reg1(val & TSFM_MaskReadyBit);// для эмуляции чтения регистров
// данные регистра чип 1 WR=0 A0=1 CS=0 запись при WR=0->1 или CS=0->1
    //                            *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100010100000000  | val | beep595) << 16;// WR=0 A0=1
   
    //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
    PIO_AY595->txf[SM_AY595] = (0b0100111100000000  | val |  beep595) << 16;//// WR=1 CS=1 A0=1    
    };
    busy_wait_us(12);
   // TSFM_MaskReadyBit=0xff;// это пришлось убрать иначе тест не проходило
}
void __not_in_flash_func (audio_out_tsfm)(void)
{	
   //заглушка по прерыванию ничего выводить не надо ;)
}
//==============================================================
// END TSFM
//==============================================================

//--------------------------------------------------------------
// оставлено для старых подпрограмм в util_z80.c
void AY_select_reg(uint8_t N_reg) 
{    N_sel_reg=N_reg;  } 
//-------------------------------------------------------------------
//=====================================================
     void AY_reset()
     {
       // gpio_put(25,1);
         switch (conf.type_sound)
         {
         case SOFT_AY:
         case SOFT_TS:
         case I2S_AY:
         case I2S_TS:
             memset(reg_ay0, 0, 16);
             reg_ay0[14] = 0xff;
             memset(reg_ay1, 0, 16);
             reg_ay1[14] = 0xff;
             break;
         case HARD_AY:
         case HARD_TS:
             pwm_set_gpio_level(CLK_AY_PIN, 2); // включение AY частоты
             send595(AY_RES, 0x00);             // res ay
             send595(AY_Z, 0x00);
         case TSFM:
         pwm_set_gpio_level(CLK_AY_PIN, 2); // включение AY частоты
         send595(AY_RES, 0x00);             // res ay
         send595(AY_Z, 0x00);         
         case TS_SAA:
             pwm_set_gpio_level(CLK_AY_PIN, 2); // включение AY частоты
             send595(AY_RES, 0x00);             // res ay
             send595(AY_Z, 0x00);
    //  busy_wait_us(500);
    for (int i = 0; i < 0x20; i++){
        saa1099_write(1, i);
        saa1099_write(0, 0x00);
    }


         default:
             break;
         }
     };
//----------------------------------------------------------

uint8_t  __not_in_flash_func(AY_in_FFFD)()
{ 
   if (chip_ay==0) {return AY_get_reg();}  // если 0 то чип 0
  else {return AY_get_reg1();}  // если 1 то регистр чип 1 
}

/*******************************************/
//void fast(AY_out_BFFD)(uint8_t val)
/*******************************************/
//--------------------------------------------
void fast(ay_set_data_soft_ts)(uint8_t val)
{   
    if (chip_ay==0) {AY_set_reg(val); return;} // если 0 то чипа 1
    else{ AY_set_reg1(val);  return;}// если 1 то регистр чипа 0   
}

//###################################################################
//                  CHIP AY 0
//###################################################################
static uint32_t noise_ay0_count=0;
static bool is_envelope_begin=false;
static uint8_t ampl_ENV = 0;
static uint8_t envelope_ay_count = 0;

// чтение из регистров AY
uint8_t fast(AY_get_reg)()
{
    return reg_ay0[N_sel_reg]; // & 0x0f ?
}
//-----------------------------------------------------------------
// запись в регистры AY
void fast(AY_set_reg)(uint8_t val)
    {
        if (N_sel_reg>0x0f) return;// для TSFM
      //  if (conf.type_sound==TSFM) reg_ay0[N_sel_reg] = val & maskAY_FM[N_sel_reg];// для TSFM
      //  else
         reg_ay0[N_sel_reg] = val & maskAY[N_sel_reg];
        switch (N_sel_reg)
        {
        case 6:  // частота генератора шума 	0 - 31
            noise_ay0_count = 0;
            return;
        case 13://Выбор формы волнового пакета 	0 - 15
            is_envelope_begin = true;
            return;
        }

    }
//--------------------------------------------------------------------------
uint16_t*  fast(get_AY_Out)(uint8_t delta)
{   
    static bool bools[4];

    #define chA_bit (bools[0])
    #define chB_bit (bools[1])
    #define chC_bit (bools[2])
    #define noise_bit (bools[3])
  
    static uint32_t main_ay_count_env=0;

    //отдельные счётчики
    static uint32_t chA_count=0;
    static uint32_t chB_count=0;
    static uint32_t chC_count=0;

    //копирование прошлого значения каналов для более быстрой работы

    register bool chA_bitOut=chA_bit;
    register bool chB_bitOut=chB_bit;
    register bool chC_bitOut=chC_bit;

        ay0_A_freq =((reg_ay0[1]<<8)|reg_ay0[0]);
        ay0_B_freq =((reg_ay0[3]<<8)|reg_ay0[2]);
        ay0_C_freq =((reg_ay0[5]<<8)|reg_ay0[4]);
        ay0_R12_R11= ((reg_ay0[12]<<8)|reg_ay0[11]);

    #define nR7 (~reg_ay0[7])
    //nR7 - инвертированый R7 для прямой логики - 1 Вкл, 0 - Выкл

    if (nR7&0x1) {chA_count+=delta;if (chA_count>= ay0_A_freq && (ay0_A_freq>=delta) ) {chA_bit^=1;chA_count=0;}} else {chA_bitOut=1;chA_count=0;}; /*Тон A*/
    if (nR7&0x2) {chB_count+=delta;if (chB_count>= ay0_B_freq && (ay0_B_freq>=delta) ) {chB_bit^=1;chB_count=0;}} else {chB_bitOut=1;chB_count=0;}; /*Тон B*/
    if (nR7&0x4) {chC_count+=delta;if (chC_count>= ay0_C_freq && (ay0_C_freq>=delta) ) {chC_bit^=1;chC_count=0;}} else {chC_bitOut=1;chC_count=0;}; /*Тон C*/

    //проверка запрещения тона в каналах
    if (reg_ay0[7]&0x1) chA_bitOut=1; 
    if (reg_ay0[7]&0x2) chB_bitOut=1;
    if (reg_ay0[7]&0x4) chC_bitOut=1;

    //добавление шума, если разрешён шумовой канал
    if (nR7&0x38)//есть шум хоть в одном канале
        {

            noise_ay0_count+=delta;
          if (noise_ay0_count>=(reg_ay0[6]<<1)) {noise_bit=get_random(); noise_ay0_count=0;}//отдельный счётчик для шумового
                                // R6 - частота шума
            
               if(!noise_bit)//если бит шума ==1, то он не меняет состояние каналов

                {            
                    if ((chA_bitOut)&&(nR7&0x08)) chA_bitOut=0;//шум в канале A
                    if ((chB_bitOut)&&(nR7&0x10)) chB_bitOut=0;//шум в канале B
                    if ((chC_bitOut)&&(nR7&0x20)) chC_bitOut=0;//шум в канале C

                };
           
        }

       // амплитуды огибающей
        if ((reg_ay0[8] & 0x10) | (reg_ay0[9] & 0x10) | (reg_ay0[10] & 0x10)) // отключение огибающей
        {   
            main_ay_count_env += delta;
            if (is_envelope_begin)
            {
                envelope_ay_count = 0;
                main_ay_count_env = 0;
                is_envelope_begin = false;
            };

            if (((main_ay_count_env) >= (ay0_R12_R11 << 1))) // без операции деления
            {
                   main_ay_count_env -= ay0_R12_R11 << 1;
  
             if (envelope_ay_count >= 32)
             {
              //  if ((reg_ay0[13]==0x08)| (reg_ay0[13]==0x0a) | (reg_ay0[13]==0x0c) | (reg_ay0[13]==0x0e))
              if ((reg_ay0[13]&0x08) && ((~reg_ay0[13])&0x01))
                 envelope_ay_count = 0;  // loop 
                 else envelope_ay_count = 31;

             }

                uint8_t x =  Envelopes_const [ reg_ay0[13]] [envelope_ay_count] ;// 0 ... 15
                ampl_ENV = ampls_AY_table [x]; // 0... 45
                envelope_ay_count++;
            }
        } //end  амплитуды огибающей 


static uint16_t outs[3];

#define outA (outs[0])
#define outB (outs[1])
#define outC (outs[2])
              
        outA = chA_bitOut ? ((reg_ay0[ 8] & 0xf0) ? ampl_ENV : ampls_AY_table [reg_ay0[8]]) : 0;
        outB = chB_bitOut ? ((reg_ay0[ 9] & 0xf0) ? ampl_ENV : ampls_AY_table [reg_ay0[9]])   >>1: 0;
        outC = chC_bitOut ? ((reg_ay0[10] & 0xf0) ? ampl_ENV : ampls_AY_table [reg_ay0[10]]) : 0;
      
        return outs;
};
//===================================================================
//                  CHIP AY 1
//===================================================================
static uint32_t noise_ay1_count=0;
static bool is_envelope_begin1=false;
static uint8_t ampl_ENV_1 = 0;
static uint8_t envelope_ay1_count = 0;
//-------------------------------------------------------------------
// чтение из регистров AY
uint8_t fast(AY_get_reg1)()
{
    return reg_ay1[N_sel_reg_1]; // & 0x0f ?
}
//----------------------------------------------------------
    // запись в регистры AY
    void fast (AY_set_reg1)(uint8_t val)
    {
        if (N_sel_reg_1>0x0f) return;// для TSFM
      //  if (conf.type_sound==TSFM) reg_ay1[N_sel_reg_1] = val & maskAY_FM[N_sel_reg_1];// для TSFM
       // else 
        reg_ay1[N_sel_reg_1] = val & maskAY[N_sel_reg_1];
        switch (N_sel_reg_1)
        {
        case 6:
            noise_ay1_count = 0;
            return;
        case 13:
            is_envelope_begin1 = true;
            return;
        }

    }
    //------------------------

    uint16_t *fast(get_AY_Out1)(uint8_t delta)
    {

    static bool bools1[4];
    #define chA_bit_1 (bools1[0])
    #define chB_bit_1 (bools1[1])
    #define chC_bit_1 (bools1[2])
    #define noise_bit_1 (bools1[3])
  
    static uint32_t main_ay1_count_env=0;

    //отдельные счётчики
    static uint32_t chA1_count=0;
    static uint32_t chB1_count=0;
    static uint32_t chC1_count=0;

    //копирование прошлого значения каналов для более быстрой работы

    register bool chA_bit_1Out=chA_bit_1;
    register bool chB_bit_1Out=chB_bit_1;
    register bool chC_bit_1Out=chC_bit_1;
    
        ay1_A_freq =((reg_ay1[1]<<8)|reg_ay1[0]);
        ay1_B_freq =((reg_ay1[3]<<8)|reg_ay1[2]);
        ay1_C_freq =((reg_ay1[5]<<8)|reg_ay1[4]);
        ay1_R12_R11= ((reg_ay1[12]<<8)|reg_ay1[11]);

    #define n1R7 (~reg_ay1[7])

    //n1R7 - инвертированый R7 для прямой логики - 1 Вкл, 0 - Выкл

    if (n1R7&0x1) {chA1_count+=delta;if (chA1_count>=ay1_A_freq && (ay1_A_freq>=delta) ) {chA_bit_1^=1;chA1_count=0;}} else {chA_bit_1Out=1;chA1_count=0;}; /*Тон A*/
    if (n1R7&0x2) {chB1_count+=delta;if (chB1_count>=ay1_B_freq && (ay1_B_freq>=delta) ) {chB_bit_1^=1;chB1_count=0;}} else {chB_bit_1Out=1;chB1_count=0;}; /*Тон B*/
    if (n1R7&0x4) {chC1_count+=delta;if (chC1_count>=ay1_C_freq && (ay1_C_freq>=delta) ) {chC_bit_1^=1;chC1_count=0;}} else {chC_bit_1Out=1;chC1_count=0;}; /*Тон C*/

    //проверка запрещения тона в каналах
    if (reg_ay1[7]&0x1) chA_bit_1Out=1; 
    if (reg_ay1[7]&0x2) chB_bit_1Out=1;
    if (reg_ay1[7]&0x4) chC_bit_1Out=1;

    //добавление шума, если разрешён шумовой канал
    if (n1R7&0x38)//есть шум хоть в одном канале
        {
            noise_ay1_count+=delta;
            if (noise_ay1_count>=(reg_ay1[6]<<1)) {noise_bit_1=get_random();noise_ay1_count=0;}//отдельный счётчик для шумового
                                  // R6 - частота шума
            
            if(!noise_bit_1)//если бит шума ==1, то он не меняет состояние каналов
                {            
                    if ((chA_bit_1Out)&&(n1R7&0x08)) chA_bit_1Out=0;//шум в канале A
                    if ((chB_bit_1Out)&&(n1R7&0x10)) chB_bit_1Out=0;//шум в канале B
                    if ((chC_bit_1Out)&&(n1R7&0x20)) chC_bit_1Out=0;//шум в канале C
                };
        }
        // вычисление амплитуды огибающей
        if ((reg_ay1[8] & 0x10) | (reg_ay1[9] & 0x10) | (reg_ay1[10] & 0x10)) // отключение огибающей
        {   
            main_ay1_count_env += delta;
            if (is_envelope_begin1)
            {
                envelope_ay1_count = 0;
                main_ay1_count_env = 0;
                is_envelope_begin1 = false;
            }

            if (((main_ay1_count_env) >= (ay1_R12_R11 << 1))) // без операции деления
            {
                   main_ay1_count_env -= ay1_R12_R11 << 1;

             if (envelope_ay1_count >= 32)
             {
               // if ((reg_ay1[13]==0x08)| (reg_ay1[13]==0x0a) | (reg_ay1[13]==0x0c) | (reg_ay1[13]==0x0e))
                if ((reg_ay1[13]&0x08) && ((~reg_ay1[13])&0x01))
                 envelope_ay1_count = 0;
                 else envelope_ay1_count = 31;
             }
                uint8_t x =  Envelopes_const [ reg_ay1[13]] [envelope_ay1_count] ;// 0 ... 15
                ampl_ENV_1 = ampls_AY_table [x]; // 0... 45
                envelope_ay1_count++;
            }
        } //end вычисление амплитуды огибающей 


static uint16_t outs1[3];
#define outA1 (outs1[0])
#define outB1 (outs1[1])
#define outC1 (outs1[2])
          

        outA1 = chA_bit_1Out ? ((reg_ay1[8] & 0xf0) ? ampl_ENV_1 : ampls_AY_table[reg_ay1[8]]) : 0;
        outB1 = chB_bit_1Out ? ((reg_ay1[9] & 0xf0) ? ampl_ENV_1 : ampls_AY_table[reg_ay1[9]])  >>1 : 0;
        outC1 = chC_bit_1Out ? ((reg_ay1[10] & 0xf0) ? ampl_ENV_1 : ampls_AY_table[reg_ay1[10]]) : 0;
        return outs1;
    
};
//=================================================================================
//              END SOFT AY
//=================================================================================
//============================================
// NOISE FDD
//============================================
 uint16_t noise_fdd = 0;
 uint16_t noise_fddI2S = 0;

     void sound_fdd(void)
    {
        if (!vbuf_en) return;
      if (conf.sound_fdd) return;
       
        static uint8_t old_sound_track = 0;
        static uint16_t st = 32768;
        if (sound_track != old_sound_track)
        {
            if (st == 32768)
                st = 0;
            old_sound_track = sound_track;
        }
        st++;
        if (st < 32000)
        {
        noise_fdd = (g_gbuf[st+1]) &0x1f;
        //  noise_fdd = ((g_gbuf[st+1])+g_gbuf[get_random_noise()*get_random_noise()])&0x1f;
        //  noise_fdd =get_random_noise();
         //  noise_fddI2S = noise_fdd<<2;
        // noise_fdd = (ampls_AY[st+1]) &0x1f;
           noise_fddI2S = noise_fdd*conf.vol_i2s;
        }   
        else
        {
            noise_fdd = 0;
            noise_fddI2S = 0;
            st = 32768;
        }
    } 
//----------------------------------------------------------------------------------
// hard ay
//-----------------------------------------------------------------------------------
// программа  стейт машины для работы с регистром сдвига 595 
static const uint16_t program_instructions595[] = {
            //     .wrap_target
    0x80a0, //  0: pull   block                      
    0x6001, //  1: out    pins, 1                    
    0x1041, //  2: jmp    x--, 1                 [16]
    0xe82f, //  3: set    x, 15                  [8] 
            //     .wrap
/*             //     .wrap_target
    0x80a0, //  0: pull   block           side 0     
    0x6001, //  1: out    pins, 1         side 0     
    0x1041, //  2: jmp    x--, 1          side 2     
    0xeb2f, //  3: set    x, 15           side 1        0xe82f   [3] // задержка latch
            //     .wrap */
};
//---------------------------------------------
static const struct pio_program program595 = {
    .instructions = program_instructions595,
    .length = 4,
    .origin = -1,
    #if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
    #endif
};
// PWM для 595 частота clock ay
static void PWM_init_pin(uint pinN)
    {
        gpio_set_function(pinN, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(pinN);  
        pwm_config c_pwm=pwm_get_default_config();

        switch (conf.type_sound)
        {
        case HARD_AY:
        case HARD_TS:
         // pwm_config_set_clkdiv(&c_pwm, clock_get_hz(clk_sys) / (4.0 * 1750000));
            pwm_config_set_clkdiv(&c_pwm, clock_get_hz(clk_sys) / (4.0 * 1773000));
            break;
        case TSFM:
            pwm_config_set_clkdiv(&c_pwm, clock_get_hz(clk_sys) / (4.0 * 3590000));
            break;
        case TS_SAA:
            pwm_config_set_clkdiv(&c_pwm, clock_get_hz(clk_sys) / (4.0 * 7000000));
            break;           
        default:
            return;
            break;
        }        
        pwm_config_set_wrap(&c_pwm,3);//MAX PWM value
        pwm_init(slice_num,&c_pwm,true);
    }

//==============================================================================
// выключение звука железных AY
    void hardAY_off()
    { 
     //   if (hardAY_on_off==1) return; // два раза регистры AY не стоит писать
        hardAY_on_off=1;

        // type_sound = conf.type_sound; 
         switch (conf.type_sound)
         {
         case TSFM:
         case TS_SAA:
         // отключение AY частотой самый простой способ 
         pwm_set_gpio_level(CLK_AY_PIN, 0); ///0
         break;
         case HARD_AY:
         case HARD_TS:
          // отключение AY частотой самый простой способ если CLOCK AY с Pico
          pwm_set_gpio_level(CLK_AY_PIN, 0); 
          // остальное извращение из-за внешнего генератора CLOCK AY (

         // отключение AY смесителем регистр R7
        send595_0(AY0_FIX, 7);// регистр 7
        send595_0(AY0_WRITE, 0xff); // запись в регистр
        send595_1(AY1_FIX, 7); // регистр 7
        send595_1(AY1_WRITE, 0xff); // запись в регистр
        // R8  - управление амплитудой канала A
        send595_0(AY0_FIX, 8);// регистр 
        send595_0(AY0_WRITE, 0x00); // запись в регистр
        send595_1(AY1_FIX, 8); // регистр  
        send595_1(AY1_WRITE, 0x00); // запись в регистр
        // R9  - управление амплитудой канала B
        send595_0(AY0_FIX, 9);// регистр 
        send595_0(AY0_WRITE, 0x00); // запись в регистр
        send595_1(AY1_FIX, 9); // регистр 
        send595_1(AY1_WRITE, 0x00); // запись в регистр
         //R10 - управление амплитудой канала C
         send595_0(AY0_FIX, 10);// регистр 
        send595_0(AY0_WRITE, 0x00); // запись в регистр
        send595_1(AY1_FIX, 10); // регистр 
        send595_1(AY1_WRITE, 0x00); // запись в регистр  

        // огибающая 11 12 13
        send595_0(AY0_FIX, 11);// регистр 
        send595_0(AY0_WRITE, 0x00); // запись в регистр
        send595_1(AY1_FIX, 11); // регистр 
        send595_1(AY1_WRITE, 0x00); // запись в регистр  

        send595_0(AY0_FIX, 12);// регистр 
        send595_0(AY0_WRITE, 0x00); // запись в регистр
        send595_1(AY1_FIX, 12); // регистр 
        send595_1(AY1_WRITE, 0x00); // запись в регистр  

        send595_0(AY0_FIX, 13);// регистр 
        send595_0(AY0_WRITE, 15); // запись в регистр
        send595_1(AY1_FIX, 13); // регистр 
        send595_1(AY1_WRITE, 15); // запись в регистр  


        beep595 = 0;
        send595(AY_Z , 0x00); // неактивны AY
        break;
        
        default:
         return; // SOFT_AY, SOFT_TS, I2S_AY , I2S_TS  вырубаются другим способом   
        break;
        }
    }
//==============================================================    
    void hardAY_on()
    {
       // type_sound = conf.type_sound; 
        switch (conf.type_sound)
        {
        case TSFM:
        case TS_SAA:
        // включение AY частотой самый простой способ 
        pwm_set_gpio_level(CLK_AY_PIN, 2); 
        break;
        case HARD_AY:
        case HARD_TS:
         // включение AY частотой самый простой способ если CLOCK AY с Pico
          pwm_set_gpio_level(CLK_AY_PIN, 2); // включение AY частоты
         // остальное извращение из-за внешнего генератора CLOCK AY (

    //     if (hardAY_on_off==0) return;// два раза регистры AY не стоит писать
          hardAY_on_off=0;
         // R8  - управление амплитудой канала A
        send595_0(AY0_FIX, 8);// регистр 
        send595_0(AY0_WRITE, reg_ay0[8]); // запись в регистр
        send595_1(AY1_FIX, 8); // регистр  
        send595_1(AY1_WRITE, reg_ay1[8]); // запись в регистр
        // R9  - управление амплитудой канала B
        send595_0(AY0_FIX, 9);// регистр 
        send595_0(AY0_WRITE,reg_ay0[9]); // запись в регистр
        send595_1(AY1_FIX, 9); // регистр 
        send595_1(AY1_WRITE, reg_ay1[9]); // запись в регистр
         //R10 - управление амплитудой канала C
         send595_0(AY0_FIX, 10);// регистр 
        send595_0(AY0_WRITE, reg_ay0[10]); // запись в регистр
        send595_1(AY1_FIX, 10); // регистр 
        send595_1(AY1_WRITE, reg_ay1[10]); // запись в регистр   
         // включение AY смесителем регистр R7
        send595_0(AY0_FIX, 7);// регистр 7
        send595_0(AY0_WRITE, reg_ay0[7]); // запись в регистр
        send595_1(AY1_FIX, 7); // регистр 7
        send595_1(AY1_WRITE, reg_ay1[7]); // запись в регистр
        break;     
        default:
         return; // SOFT_AY, SOFT_TS, I2S_AY , I2S_TS  включаются другим способом   
        break;
        }
    }
//==================================================================
// деинсталяция программы PIO для 595 недоделано
void Uninit_AY_595() {
    pio_sm_unclaim(PIO_AY595,  SM_AY595);
  //  pio_remove_program(PIO_AY595, 4);
}
//--------------------------------------------------------------
//инициализация программы PIO для 595
void Init_AY_595()
{
    uint offset = pio_add_program(PIO_AY595, &program595);
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset + (program595.length-1)); 
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    //настройка side set
    sm_config_set_sideset_pins(&c,CLK_LATCH_595_BASE_PIN);
    sm_config_set_sideset(&c,2,false,false);
    for(int i=0;i<2;i++)
        {           
            pio_gpio_init(PIO_AY595, CLK_LATCH_595_BASE_PIN+i);
        }
    
    pio_sm_set_pins_with_mask(PIO_AY595, SM_AY595, 3u<<CLK_LATCH_595_BASE_PIN, 3u<<CLK_LATCH_595_BASE_PIN);
    pio_sm_set_pindirs_with_mask(PIO_AY595, SM_AY595,  3u<<CLK_LATCH_595_BASE_PIN,  3u<<CLK_LATCH_595_BASE_PIN);
    //
    pio_gpio_init(PIO_AY595, DATA_595_PIN);//резервируем под выход PIO
    pio_sm_set_consecutive_pindirs(PIO_AY595, SM_AY595, DATA_595_PIN, 1, true);//конфигурация пинов на выход

    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_out_pins(&c, DATA_595_PIN, 1);

    pio_sm_init(PIO_AY595, SM_AY595, offset, &c);
    pio_sm_set_enabled(PIO_AY595, SM_AY595, true);

    // Вычисление делителя частоты для 595
    float system_clk = clock_get_hz(clk_sys); // Получить текущую системную частоту
    float div = system_clk / (float)FREQ_595;

    // Проверка на минимальную/максимальную частоту
    if (div < 1.0f) {
        // Делитель не может быть меньше 1 (макс. частота = system_clk)
        div = 1.0f;
    } else if (div > 65536.0f) {
        // Делитель не может превышать 65536.0 (16.16 формат)
        div = 65536.0f;
    }

    // Преобразование в формат регистра CLKDIV (16.16)
    uint32_t div_int = (uint32_t)div;
    uint32_t div_frac = (uint32_t)((div - div_int) * 65536);
    uint32_t clkdiv = (div_int << 16) | div_frac;
    PIO_AY595->sm[SM_AY595].clkdiv=clkdiv; //делитель для конкретной sm

    PWM_init_pin(CLK_AY_PIN);
    pwm_set_gpio_level(CLK_AY_PIN,2);

  PIO_AY595->txf[SM_AY595]=0;// сброс AY
 //  PIO_AY595->txf[SM_AY595]=0;
  
};

//
void __not_in_flash_func(send595)(uint16_t comand, uint8_t data)
{
    ay595 = comand | data;
    PIO_AY595->txf[SM_AY595] = (ay595 | beep595) << 16;
     PIO_AY595->txf[SM_AY595] = (ay595 ) << 16;
}
//
void __not_in_flash_func(send595_0)(uint16_t comand, uint8_t data)
{
    ay595 = comand | data;
    PIO_AY595->txf[SM_AY595] = (ay595 | beep595) << 16;
    PIO_AY595->txf[SM_AY595] = (AY0_Z | data | beep595) << 16;
}
//
void __not_in_flash_func(send595_1)(uint16_t comand, uint8_t data)
{
    ay595 = comand | data;
    PIO_AY595->txf[SM_AY595] = (ay595 | beep595) << 16;
    PIO_AY595->txf[SM_AY595] = (AY1_Z | data | beep595) << 16;
}
//-----------------------------------------------------------------------
void __not_in_flash_func(out_beep595)(bool val)
{         
          if (val) beep595 = 0b0000000000000000;
            else   beep595 = 0b0001000000000000;
           PIO_AY595->txf[SM_AY595]=(ay595 | beep595)<<16;
}
//======================================================================
void __not_in_flash_func(audio_out_soft_ay)(void)
{	 
	pwm_set_gpio_level(ZX_AY_PWM_R, outR); // Право
    pwm_set_gpio_level(ZX_AY_PWM_L, outL); // Лево

        sound_fdd();
		AY_data = get_AY_Out(AY_DELTA);										  ///??????
		outL =  (AY_data[0] + AY_data[1])*vol_soft_ay; //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE;
		outR =  (AY_data[2] + AY_data[1])*vol_soft_ay;// + (noise_fdd)); // звук перемешения головок //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE; */      
}
//----------------------------------------------------------------------
void __not_in_flash_func(audio_out_soft_ts)(void)
{	
    
    pwm_set_gpio_level(ZX_AY_PWM_R, outR); // Право
    pwm_set_gpio_level(ZX_AY_PWM_L, outL); // Лево 

		AY_data = get_AY_Out(AY_DELTA);			
		AY_data1 = get_AY_Out1(AY_DELTA);		
          sound_fdd();
          outL = ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]) * vol_soft_ay; //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE; 
          outR =  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1]) * vol_soft_ay; //+ (noise_fdd); // звук перемешения головок      
}
//----------------------------------------------------------------------
void __not_in_flash_func (audio_out_hard_ay)(void)
{	
   // заглушка
}
//======================================================================

//----------------------------------------------------------------------
/* void __not_in_flash_func(audio_out_i2s_ts)(void)
{	

       sound_fdd();

		AY_data = get_AY_Out(AY_DELTA);			
		AY_data1 = get_AY_Out1(AY_DELTA);										  ///??????


// uint16_t covox_left= (SoundLeft_A*SoundLeft_B )*vol_i2s;
// uint16_t covox_right= (SoundRight_A*SoundRight_B )*vol_i2s;
        outL =   ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1] +beepPWM );//+covox_left; // outLL
        outR =  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1] +beepPWM  + noise_fddI2S );//+covox_right; // звук перемешения головок outRR 

//uint16_t outLL =  ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]);//+covox_left; // outLL
//uint16_t outRR =  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1] + noise_fddI2S );//+covox_right; // звук перемешения головок outRR 
//beepPWM =0;

        i2s_out(outR,outL);

} */
//----------------------------------------------------------------------

//###########################################
// Смешивание каналов звука и вывод по DMA
//###########################################
//#define exponential_volume
#define linear_volume
//###########################################
// Конфигурируемый параметр громкости (0-100%)
#define AY_VOLUME_PERCENT 80  // Громкость по умолчанию 80%

#define CH_TS_MAX_VALUE    255 //(45*3)      // Максимальное значение 3х каналов TS
#define CH_TS_SCALE_TO     0xffff //64000       // До какого значения масштабировать канал TS uint
#define OUTPUT_MAX_VALUE   0xffff //64000/// (CH_GS_MAX_VALUE *3)  // Ограничение суммы каналов (CH_GS_MAX_VALUE * 3)  uint
#define OUTPUT_MIDPOINT    (OUTPUT_MAX_VALUE/2) // середина диапазона  uint
// Глобальная переменная громкости
static uint8_t current_volume = AY_VOLUME_PERCENT;
// Таблицы для быстрого преобразования
//static uint16_t ay_scale_table[CH_TS_MAX_VALUE + 4];

//###########################################

//##################################################################
// Конфигурируемый параметр громкости (0-100%)

// Таблица умножения для громкости (0-100% -> 0-256)
static uint16_t volume_mult_table[101];

// Таблицы для быстрого преобразования
static uint16_t ay_scale_table[CH_TS_MAX_VALUE + 4];

//##################################################################
void init_audio_tables_optimized(void) {
    // Таблица для канала TS
    for (int i = 0; i <= CH_TS_MAX_VALUE; i++) {
        ay_scale_table[i] = (i * CH_TS_SCALE_TO) / CH_TS_MAX_VALUE;
    }
    
    // Таблица для громкости: volume * 256 / 100
    for (int i = 0; i <= 100; i++) {
        volume_mult_table[i] = (i * (256)) / 100;
    }
}

void set_vol_i2s(uint8_t volume_percent) {
    if (volume_percent > 100) volume_percent = 100;
    current_volume = volume_percent;
    init_audio_tables_optimized();
}

void set_audio_volume(uint8_t volume_percent) {
    if (volume_percent > 100) volume_percent = 100;
    current_volume = volume_percent;
}

uint8_t get_audio_volume(void) {
    return current_volume;
}
//##################################################################
void __not_in_flash_func(audio_out_i2s_ts)(void)
{	
    AY_data  = get_AY_Out(AY_DELTA);			
    AY_data1 = get_AY_Out1(AY_DELTA);		

    uint32_t sumL = AY_data[0] + AY_data[1] + AY_data1[0] + AY_data1[1] + beepPWM + valLoad;
    uint32_t sumR = AY_data[2] + AY_data[1] + AY_data1[2] + AY_data1[1] + beepPWM ;
    
    sumL = (sumL > CH_TS_MAX_VALUE) ? CH_TS_MAX_VALUE : sumL;
    sumR = (sumR > CH_TS_MAX_VALUE) ? CH_TS_MAX_VALUE : sumR;

    uint32_t totalL = ay_scale_table[sumL] ;
    uint32_t totalR = ay_scale_table[sumR] ;
    
    //if (totalL > OUTPUT_MAX_VALUE) totalL = OUTPUT_MAX_VALUE;
    //if (totalR > OUTPUT_MAX_VALUE) totalR = OUTPUT_MAX_VALUE;
    
    // Преобразуем в int16_t диапазон
    int32_t outL = (int32_t)totalL - OUTPUT_MIDPOINT ;
    int32_t outR = (int32_t)totalR - OUTPUT_MIDPOINT ;
    
    // Применяем громкость БЕЗ ДЕЛЕНИЯ - только умножение и сдвиг
    outL = (outL * volume_mult_table[current_volume]) >> 8;
    outR = (outR * volume_mult_table[current_volume]) >> 8;
    
    // Ограничиваем результат   можно и без этого
  //  outL = (outL > 32767) ? 32767 : (outL < -32768) ? -32768 : outL;
  //  outR = (outR > 32767) ? 32767 : (outR < -32768) ? -32768 : outR;

    i2s_out((int16_t)outR, (int16_t)outL);
}

//###############################################################
void __not_in_flash_func(audio_out_i2s_ay)(void)
{	
      //  sound_fdd();

    AY_data = get_AY_Out(AY_DELTA);			
 	
    uint32_t sumL = AY_data[0] + AY_data[1] + beepPWM + valLoad;
    uint32_t sumR = AY_data[2] + AY_data[1] + beepPWM;
    
    sumL = (sumL > CH_TS_MAX_VALUE) ? CH_TS_MAX_VALUE : sumL;
    sumR = (sumR > CH_TS_MAX_VALUE) ? CH_TS_MAX_VALUE : sumR;

 //   uint32_t totalL = ay_scale_table[sumL] ;
 //   uint32_t totalR = ay_scale_table[sumR] ;
    
    //if (totalL > OUTPUT_MAX_VALUE) totalL = OUTPUT_MAX_VALUE;
    //if (totalR > OUTPUT_MAX_VALUE) totalR = OUTPUT_MAX_VALUE;
    
    // Преобразуем в int16_t диапазон
    int32_t outL = (int32_t)ay_scale_table[sumL] - OUTPUT_MIDPOINT ;
    int32_t outR = (int32_t)ay_scale_table[sumR]  - OUTPUT_MIDPOINT ;
    
    // Применяем громкость БЕЗ ДЕЛЕНИЯ - только умножение и сдвиг
    outL = (outL * volume_mult_table[current_volume]) >> 8;
    outR = (outR * volume_mult_table[current_volume]) >> 8;
    
    // Ограничиваем результат   можно и без этого
  //  outL = (outL > 32767) ? 32767 : (outL < -32768) ? -32768 : outL;
  //  outR = (outR > 32767) ? 32767 : (outR < -32768) ? -32768 : outR;

    i2s_out((int16_t)outR, (int16_t)outL);
  
}


/* void __not_in_flash_func(audio_out_i2s_ay)(void)
{	
        sound_fdd();
		AY_data = get_AY_Out(AY_DELTA);										  ///??????
	    outL =  (AY_data[0] + AY_data[1]+beepPWM); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE;
	    outR =   (AY_data[2] + AY_data[1]+beepPWM + noise_fddI2S ); // звук перемешения головок //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE; 
        i2s_out(outR,outL);       
}

 */
//======================================================================
// beep & save

void __not_in_flash_func(hw_zx_set_snd_out)(bool val) // beep
{
	  beep_data=(beep_data&0b01)|(val<<1);   
}

void __not_in_flash_func(hw_out595_beep_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	//gpio_put(ZX_BEEP_PIN,out);
    out_beep595(out);
	beep_data_old=beep_data;
	
}
//#define BEEP 1
#if BEEP == 0
// вывод звук бипера на пин классика
void __not_in_flash_func(hw_outpin_beep_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	gpio_put(ZX_BEEP_PIN,out);
	beep_data_old=beep_data;
}
#endif
#if BEEP == 1
// вывод звук бипера на отдельный шим
void __not_in_flash_func(hw_outpin_beep_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	beep_data_old=beep_data;
   switch (out)
   {
   case 0:
     pwm_set_gpio_level(ZX_BEEP_PIN,0);  
    break;
   case 1:
    pwm_set_gpio_level(ZX_BEEP_PIN,vol_beep);
    break;   
   }
}
#endif

#if BEEP == 2
// вывод звук бипера на ШИМ 
void __not_in_flash_func(hw_outpin_beep_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
//	gpio_put(ZX_BEEP_PIN,out);
	beep_data_old=beep_data;
   beepPWM = beep_data*256;// вывод бипера на ШИМ
}
#endif

#if BEEP == 3
// вывод звук бипера на отдельный шим
void __not_in_flash_func(hw_outpin_beep_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10);//|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	
 if (out)  pwm_set_gpio_level(ZX_BEEP_PIN,conf.vol_ay);
 else  pwm_set_gpio_level(ZX_BEEP_PIN,0 );

 beep_data_old=beep_data;


}
#endif
//---------------------------------------------------------------------
void __not_in_flash_func(hw_outi2s_beep_out)(bool val)
{
    static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	beep_data_old=beep_data;
   //  switch (out)

/*    switch (val)
   {
   case 0:
    beepPWM = 0 ;
    break;
   case 1:
     beepPWM =vol_beep;
    break;   
  
   } */

   beepPWM = val ? vol_beep : 0;

//i2s_out(beepPWM,beepPWM);
}


/**




void __not_in_flash_func(hw_outi2s_beep_out)(bool val)
{
    static bool out=0;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
if((beep_data!=0)||(beep_data_old!=0))

{
    beepPWM = vol_beep;
  //  i2s_out(beepPWM,beepPWM);
 i2s_out(outR,outL);
}

  return;



	beep_data_old=beep_data;
   switch (out)
   {
   case 0:
    beepPWM = 0 ;
    return;
    break;
   case 1:
     beepPWM = 0;//vol_beep;
  //  beepPWM = 0 ;
    break;    
   }
    // i2s_out(outR,outL);
  //  i2s_out(beepPWM,beepPWM);
}
  */
//=====================================================================
void PWM_init_pin_SoftAY(uint pinN, uint PWM_WRAP){
    
    gpio_set_drive_strength(pinN,GPIO_DRIVE_STRENGTH_4MA);

    gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,1.0f); // делитель частоты для ay 1.0f    315/1.23 1.23529411765f
	pwm_config_set_wrap(&c_pwm,PWM_WRAP);//MAX PWM value =2000
	pwm_init(slice_num,&c_pwm,true);
}
void PWM_init_pin_SoftBeep(uint pinN, uint PWM_WRAP){
    
    gpio_set_drive_strength(pinN,GPIO_DRIVE_STRENGTH_4MA);

	gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,2.0f); // делитель частоты для ay 1.0f    315/1.23 1.23529411765f
	pwm_config_set_wrap(&c_pwm,PWM_WRAP);//MAX PWM value =100
	pwm_init(slice_num,&c_pwm,true);
}

//======================================================================
// переключение режимов вывода звука
void select_audio(void)
{
 init_audio_tables_optimized();
  

    chip_ay = 0;
    maskAY[1] = maskAY[3] = maskAY[5] = 0x0f; // установки маски по умолчанию для всех режимов кроме TSFM 
         if (conf.type_sound >= AY_END)  conf.type_sound = 0;

    switch (conf.type_sound)
    {
    case SOFT_AY /*Soft   AY Sound */:
        AY_out_FFFD = ay_set_reg_soft;
        AY_out_BFFD = AY_set_reg; // ay_set_data_soft
        audio_out = audio_out_soft_ay;
        // пины вывода звука soft ay
        PWM_init_pin_SoftAY(ZX_AY_PWM_R,AY_PWM_WRAP);
        PWM_init_pin_SoftAY(ZX_AY_PWM_L,AY_PWM_WRAP);
        // beep
     #if BEEP == 0  
          gpio_init(ZX_BEEP_PIN);
          gpio_set_dir(ZX_BEEP_PIN, GPIO_OUT);
     #endif    
     #if BEEP == 1
        PWM_init_pin_SoftBeep(ZX_BEEP_PIN,BEEP_PWM_WRAP);
     #endif
        hw_beep_out = hw_outpin_beep_out;
        break;

        
    case SOFT_TS /*Soft TurboSound*/:
        AY_out_FFFD = ay_set_reg_soft_ts;
        AY_out_BFFD = ay_set_data_soft_ts;
        audio_out = audio_out_soft_ts;
        // пины вывода звука soft ay
        PWM_init_pin_SoftAY(ZX_AY_PWM_R,AY_PWM_WRAP);
        PWM_init_pin_SoftAY(ZX_AY_PWM_L,AY_PWM_WRAP);
        // beep
     #if BEEP == 0  
          gpio_init(ZX_BEEP_PIN);
          gpio_set_dir(ZX_BEEP_PIN, GPIO_OUT);
     #endif    
     #if BEEP == 1
        PWM_init_pin_SoftBeep(ZX_BEEP_PIN,BEEP_PWM_WRAP);
     #endif
        hw_beep_out = hw_outpin_beep_out;
        break;

    case HARD_AY /*Hard   AY Sound */:
        Init_AY_595();
        hardAY_on_off=0;
        AY_out_FFFD = ay_set_reg_hard;
        AY_out_BFFD = ay_set_data_hard;
        audio_out = audio_out_hard_ay;// заглушка
        hw_beep_out = hw_out595_beep_out;
        ay595 = 0;
        beep595 = 0;
      //  pwm_set_gpio_level(CLK_AY_PIN, 2); 
        send595(AY_RES, 0x00);
        send595(AY_Z, 0x00);
        break;

    case HARD_TS /*Hard TurboSound */:
        Init_AY_595();
        hardAY_on_off=0;
        AY_out_FFFD = ay_set_reg_hard_ts;
        AY_out_BFFD = ay_set_data_hard_ts;
        audio_out = audio_out_hard_ay;// заглушка
        hw_beep_out = hw_out595_beep_out;
        ay595 = 0;
        beep595 = 0;
      //  pwm_set_gpio_level(CLK_AY_PIN, 2); 
        send595(AY_RES, 0x00);
        send595(AY_Z, 0x00);
          break;
        //------------------------------------
    case I2S_AY /*I2S  AY Sound */:
      //  init_audio_tables_optimized();
        i2s_init();
        i2s_out(OUTPUT_MIDPOINT, OUTPUT_MIDPOINT);// обнуление каналов звука
        audio_out = audio_out_i2s_ay;
        AY_out_FFFD = ay_set_reg_soft;
        AY_out_BFFD = AY_set_reg; // ay_set_data_soft
        hw_beep_out = hw_outi2s_beep_out;
       
        break;
   //------------------------------------
    case I2S_TS /*I2S  TS Sound */:
     //   init_audio_tables_optimized();
        i2s_init();
        i2s_out(OUTPUT_MIDPOINT, OUTPUT_MIDPOINT);// обнуление каналов звука
        audio_out =   audio_out_i2s_ts;
        AY_out_FFFD = ay_set_reg_soft_ts;
        AY_out_BFFD = ay_set_data_soft_ts;
        hw_beep_out = hw_outi2s_beep_out;
        break;
    //-----------------------------------------
        case TSFM /*Hard   TSFM */:
        maskAY[1] = maskAY[3] = maskAY[5] = 0x1f; // установки маски для  режима TSFM 
        Init_AY_595();
        hardAY_on_off=0;
        AY_out_FFFD = ay_set_reg_tsfm;// заготовка для TSFM
        AY_out_BFFD = ay_set_data_tsfm;// заготовка для TSFM
        audio_out = audio_out_tsfm;// заглушка
        hw_beep_out = hw_out595_beep_out;// вывод звука при save
        ay595 = 0;
        beep595 = 0;
    //    pwm_set_gpio_level(CLK_AY_PIN, 2); 
     //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
     PIO_AY595->txf[SM_AY595] = (0b0000111100000000  | 0x00 ) << 16;// reset FM
     //                           *R*B10WA--------  // R-reset B-beep 1-CS_1 0-CS_0  W-WR A-A0
     PIO_AY595->txf[SM_AY595] = (0b0100111100000000  | 0x00 ) << 16;//// WR=1 CS=1 A0=1  

       break;
    //-----------------------------------------
    case TS_SAA /*Hard TurboSound +SAA*/:
    Init_AY_595();
    hardAY_on_off=0;
    AY_out_FFFD = ay_set_reg_hard_ts;
    AY_out_BFFD = ay_set_data_hard_ts;
    audio_out = audio_out_hard_ay;// заглушка
    hw_beep_out = hw_out595_beep_out;
    ay595 = 0;
    beep595 = 0;
    pwm_set_gpio_level(CLK_AY_PIN, 2); 
    send595(AY_RES, 0x00);
    send595(AY_Z, 0x00);


      break;
    //------------------------------------
    default:
        break;
    }
    init_vol_ay();  // установка громкости AY
}


//===================================================================
// установка громкости каналов AY
void init_vol_ay(void)
{

    if (conf.vol_ay>MAX_VOL_PWM) conf.vol_ay=MAX_VOL_PWM;//!!!!!
    if (conf.vol_i2s>100) conf.vol_i2s=100;//!!!!!   
          switch (conf.type_sound)
          {
          case SOFT_AY:
          case SOFT_TS:
            //копирование таблицы AY в RAM
            for (size_t i = 0; i < 16; i++)
            {
                ampls_AY_table[i]=const_ampls_AY_table[i];
            }
             vol_soft_ay = (conf.vol_ay * 16) / 100;
             vol_beep=(conf.vol_ay * 16) / 100;
          break;
          case I2S_AY:             
          case I2S_TS:
            //копирование таблицы AY в RAM  и умножение на conf.
            for (size_t i = 0; i < 16; i++)
            {
                ampls_AY_table[i]=const_ampls_AY_table[i] *  (conf.audio_buster+1);
            }
           set_vol_i2s(conf.vol_i2s);
           vol_beep=(conf.vol_i2s * 127) / 100;// громкость beep на два канала поэтому /2
            break;          
          default:
            // hard ay
             vol_beep=0;//(conf.vol_ay * 16) / 100;
            break;
          }

         /// vol_beep=0;//(conf.vol_ay * 16) / 100;
    
}
//--------------------------------------------
void set_audio_buster(void)
{
/* регулировка усилителя*/
init_vol_ay();
}
//===================================================================

//++++++++++++++++++++++++++++++++++++++++++++++
// SAA1099
#define CS_SAA1099	 0b1000000000000000//(1<<15)//32768 0x8000
#define BC1          0b0000000100000000//(1<<8)//256  //
static uint16_t control_bits = 0;
#define LOW(x) (control_bits &= ~(x))
#define HIGH(x) (control_bits |= (x))

#define AY_Enable  0b0100000000000000//(1<<14)// 16384 0x4000
#define CS_AY1 (1<<11)// 2048
#define CS_AY0	(1<<10)// 1024
#define Beeper (1<<12)//4096



void __not_in_flash_func(saa1099_write)(uint8_t addr, uint16_t byte) {

     if(addr==0)// BC1=0
    {
        PIO_AY595->txf[SM_AY595] = ( 0b0000000000000000 | byte)<< 16;
        busy_wait_us(5);
        PIO_AY595->txf[SM_AY595] = ( 0b0010000000000000 | byte)<< 16;//HIGH(CS_SAA1099) 5 нога
    }

    if(addr==1)// BC1=1
    {
        PIO_AY595->txf[SM_AY595] = ( 0b0000000100000000 | byte)<< 16;
        busy_wait_us(5);
        PIO_AY595->txf[SM_AY595] = ( 0b0010000100000000 |  byte)<< 16;//HIGH(CS_SAA1099) 5 нога
    }

/*   //  LOW(AY_Enable);

    // const uint16_t a0 = addr ? BC1 : 0;
   
    	if(addr>0){HIGH(BC1);}else{LOW(BC1);}
  
 //   send595(byte | LOW(CS_SAA1099)); //
 LOW(CS_SAA1099 );
    PIO_AY595->txf[SM_AY595] = ( control_bits | byte)<< 16;


    busy_wait_us(5);
  //  send595(byte | HIGH(CS_SAA1099)); // 
  HIGH(CS_SAA1099 ) ;
    PIO_AY595->txf[SM_AY595] = (control_bits | byte)<< 16; */
}

/* if (sound_mode == 4){
    send_to_595(LOW(AY_Enable));
    //  busy_wait_us(500);
    for (int i = 0; i < 0x20; i++){
        saa1099_write(1, i);
        saa1099_write(0, 0x00);
    }
    send_to_595(HIGH(AY_Enable | CS_SAA1099 | CS_AY0 | Beeper |CS_AY1 | BDIR | BC1 | (SAVE)));	
    
    

} */

#endif