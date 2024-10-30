#include "config.h"
#include "stdbool.h"
#include "zx_emu/hw/hw_util.h"


/*
N регистра	Назначение или содержание	Значение	
0, 2, 4 	Нижние 8 бит частоты голосов А, В, С 	0 - 255
1, 3, 5 	Верхние 4 бита частоты голосов А, В, С 	0 - 15
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

//===================================================================
static uint32_t noise_ay1_count=0;
static bool is_envelope_begin1=false;
static uint16_t ampl_ENV_1 = 0;
static uint32_t envelope_ay1_count = 0;
//-------------------------------------------------------------------
// чтение из регистров AY
uint8_t FAST_FUNC(AY_get_reg1)()
{
/* 
    switch (N_sel_reg_1)
    {
    case 8:  // громкость A
    case 9:  // громкость B
    case 10: // громкость C
        return reg_ay1[N_sel_reg_1] & 0x0f;
    default:
        break;
    } */
    return reg_ay1[N_sel_reg_1]; // & 0x0f ?
}
//----------------------------------------------------------
    // запись в регистры AY
    void FAST_FUNC (AY_set_reg1)(uint8_t val)
    {
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

    uint16_t *FAST_FUNC(get_AY_Out1)(uint8_t delta)
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
            };

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
               // ampl_ENV_1 = ampls_AY[ Envelopes [reg_ay1[13]] [envelope_ay1_count]  ];
              // ampl_ENV_1 = ampls_AY[ Envelopes_const [reg_ay1[13]] [envelope_ay1_count]  ];
                ampl_ENV_1 = Envelopes [reg_ay1[13]] [envelope_ay1_count]  ;
                envelope_ay1_count++;
            }
        } //end вычисление амплитуды огибающей 


static uint16_t outs1[3];
#define outA1 (outs1[0])
#define outB1 (outs1[1])
#define outC1 (outs1[2])
          

        outA1 = chA_bit_1Out ? ((reg_ay1[8] & 0xf0) ? ampl_ENV_1 : ampls_AY[reg_ay1[8]]) : 0;
        outB1 = chB_bit_1Out ? ((reg_ay1[9] & 0xf0) ? ampl_ENV_1 : ampls_AY[reg_ay1[9]]) : 0;
        outC1 = chC_bit_1Out ? ((reg_ay1[10] & 0xf0) ? ampl_ENV_1 : ampls_AY[reg_ay1[10]]) : 0;



        return outs1;
    

};