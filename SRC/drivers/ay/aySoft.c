#include "config.h"
#include "stdbool.h"
#include "zx_emu/hw/hw_util.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "pico/platform.h"

#include "inttypes.h"
#include "hardware/pio.h"
#include <string.h>



    uint16_t *AY_data;
	uint16_t *AY_data1;
	 uint16_t outL = 0;
	 uint16_t outR = 0;
//	 uint8_t outL = 0;
//	 uint8_t outR = 0;



	 uint inx;
 //----------------------------------   
uint8_t N_sel_reg=0;
uint8_t N_sel_reg_1=0;
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

/* GenNoise (c) Hacker KAY & Sergey Bulba */

bool __not_in_flash_func(get_random)()
{
    static uint32_t Cur_Seed = 0xffff;
    Cur_Seed = (Cur_Seed * 2 + 1) ^
               (((Cur_Seed >> 16) ^ (Cur_Seed >> 13)) & 1);
    if ((Cur_Seed >> 16) & 1)
        return true;
    return false;
}
/* End of GenNoise (c) Hacker KAY & Sergey Bulba */

//===================================================================

static uint32_t noise_ay0_count=0;
static bool is_envelope_begin=false;
static uint16_t ampl_ENV = 0;
static uint32_t envelope_ay_count = 0;
//---------------------------------------------------------------- 
 //  void FAST_FUNC(AY_out_FFFD)(uint8_t N_reg) 
//----------------------------------------------------------------
void FAST_FUNC(ay_set_reg_soft)(uint8_t N_reg) 
{ 
     N_sel_reg=N_reg; 
}
//----------------------------------------------------------------
void FAST_FUNC (ay_set_reg_soft_ts)(uint8_t N_reg) 
{
    if (N_reg == 0xff) {chip_ay = 0;return;} 
    if (N_reg == 0xfe) {chip_ay = 1;return;}
    if (chip_ay==0) {N_sel_reg=N_reg;} // если 0 то чип 0
    else N_sel_reg_1=N_reg; // если 1 то регистр чипа 1

}
//----------------------------------------------------------------
void FAST_FUNC(ay_set_reg_hard)(uint8_t N_reg) 
{      N_sel_reg=N_reg;  // только чип 0
        // WRITE REGISTER NUMBER
        //send595(AY0_FIX, N_sel_reg); // bdir 1 bc 1
        //send595(AY0_Z, N_sel_reg);   // bdir 1 bc 1
        send595_0(AY0_FIX, N_sel_reg); // bdir 1 bc 1
}   
//---------------------------------------------------------------- 
    void FAST_FUNC(ay_set_reg_hard_ts)(uint8_t N_reg) 
{   if (N_reg == 0xff) {chip_ay = 0;return;} 
    if (N_reg == 0xfe) {chip_ay = 1;return;}
    if (chip_ay==0) {N_sel_reg=N_reg; } // если 0 то чип 0
    else N_sel_reg_1=N_reg; // если 1 то регистр чипа 1
        if (chip_ay == 0) // если 0 то чип 0
    {
     // WRITE REGISTER NUMBER
     //   send595(AY0_FIX, N_sel_reg); // bdir 1 bc 1
     //   send595(AY0_Z, N_sel_reg);   // bdir 1 bc 1
        send595_0(AY0_FIX, N_sel_reg); // bdir 1 bc 1
    }

    else // если 1 то регистр чипа 1
    {
        // WRITE REGISTER NUMBER
       // send595(AY1_FIX, N_sel_reg_1); // bdir 1 bc 1
       // send595(AY1_Z, N_sel_reg_1);   // bdir 1 bc 1
        send595_1(AY1_FIX, N_sel_reg_1); // bdir 1 bc 1
    }
}   
//--------------------------------------------------------------
// оставлено для старых подпрограмм в main.c
void AY_select_reg(uint8_t N_reg) 
{    N_sel_reg=N_reg; 
     } 
//-------------------------------------------------------------------
//=====================================================
void  AY_reset()
{
    memset(reg_ay0,0,16);
    reg_ay0[7]=reg_ay0[14]=0xff;

    memset(reg_ay1,0,16);
    reg_ay1[7]=reg_ay1[14]=0xff;

   if (type_sound==1)// если hard ts
   {
    hardAY_on();
	//send595(AY0_RES ,0x00);// res ay
	//send595(AY_Z ,0x00);
    send595_0(AY_RES ,0x00);// res ay
   }
};

uint8_t FAST_FUNC(AY_in_FFFD)()
{
    if (chip_ay==0) return AY_get_reg();  // если 0 то чип 0
    else return AY_get_reg1();  // если 1 то регистр чип 1  
}

//-------------------------------------------
//void FAST_FUNC(AY_out_BFFD)(uint8_t val)
//--------------------------------------------

//--------------------------------------------
void FAST_FUNC(ay_set_data_soft_ts)(uint8_t val)
{   
    if (chip_ay==0) {AY_set_reg(val); return;} // если 0 то чипа 1


    else{ AY_set_reg1(val);  return;}// если 1 то регистр чипа 0   
}
//-----------------------------------------------------------------
void FAST_FUNC(ay_set_data_hard)(uint8_t val)
{// только чип 0
        // WRITE REGISTER DATA
       // send595(AY0_WRITE, val); // bdir 1 bc 0
       // send595(AY0_Z, val); // bdir 0 bc 0
        send595_0(AY0_WRITE, val); // bdir 1 bc 0
        AY_set_reg(val);
}
//----------------------------------------------------------------
void FAST_FUNC(ay_set_data_hard_ts)(uint8_t val)
{
    
    if (chip_ay == 0) // если 0 то чип 0
    {
        // WRITE REGISTER DATA
       // send595(AY0_WRITE, val); // bdir 1 bc 0
       // send595(AY0_Z, val); // bdir 0 bc 0
        send595_0(AY0_WRITE, val); // bdir 1 bc 0
        AY_set_reg(val);
    }

    else // если 1 то регистр чипа 1
    {
        // WRITE REGISTER DATA
        //send595(AY1_WRITE, val); // bdir 1 bc 0
        //send595(AY1_Z, val); // bdir 0 bc 0
        send595_1(AY1_WRITE, val); // bdir 1 bc 0
        AY_set_reg1(val);
    };
}
//=================================================================
// чтение из регистров AY
uint8_t FAST_FUNC(AY_get_reg)()
{
/* 
    switch (N_sel_reg)
    {
    case 8:  // громкость A
    case 9:  // громкость B
    case 10: // громкость C
        return reg_ay0[N_sel_reg] & 0x0f;
    default:
        break;
    } */
    return reg_ay0[N_sel_reg]; // & 0x0f ?
}
//-----------------------------------------------------------------
// запись в регистры AY
void FAST_FUNC(AY_set_reg)(uint8_t val)
    {
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
    //------------------------

//--------------------------------------------------------------------------
uint16_t*  FAST_FUNC(get_AY_Out)(uint8_t delta)
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
               // ampl_ENV = ampls_AY[ Envelopes [reg_ay0[13]] [envelope_ay_count]  ];
              // ampl_ENV = ampls_AY[ Envelopes_const [reg_ay0[13]] [envelope_ay_count]  ];
                ampl_ENV =  Envelopes[reg_ay0[13]] [envelope_ay_count]  ; // из оперативки
                envelope_ay_count++;
            }
        } //end  амплитуды огибающей 


static uint16_t outs[3];

#define outA (outs[0])
#define outB (outs[1])
#define outC (outs[2])

        outA = chA_bitOut ? ((reg_ay0[8] & 0xf0) ? ampl_ENV : ampls_AY [reg_ay0[8]]) : 0;
        outB = chB_bitOut ? ((reg_ay0[9] & 0xf0) ? ampl_ENV : ampls_AY [reg_ay0[9]]) : 0;
        outC = chC_bitOut ? ((reg_ay0[10] & 0xf0) ? ampl_ENV : ampls_AY [reg_ay0[10]]) : 0;

        return outs;
};
//===================================================================================
 uint8_t noise_fdd = 0;
 uint16_t noise_fddI2S = 0;

    void sound_fdd(void)
    {
        if (!vbuf_en) return;
        if (conf.sound_ffd) return;
       
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
           noise_fddI2S = noise_fdd<<2;
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
// программа  стейт машины для работы с регистром сдвига 595 без задержки 
#if defined(HTS_DEFAULT)
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
#endif
// программа  стейт машины для работы с регистром сдвига 595 c задержкой
#if defined(HTS_SAGIN)
static const uint16_t program_instructions595[] = {
            //     .wrap_target
    0x80a0, //  0: pull   block                      
    0x6101, //  1: out    pins, 1                [1] 
    0x1141, //  2: jmp    x--, 1                 [17]
    0xea2f, //  3: set    x, 15                  [10]
            //     .wrap
};
#endif

//---------------------------------------------
static const struct pio_program program595 = {
    .instructions = program_instructions595,
    .length = 4,
    .origin = -1,
};
// PWM для 595
static void PWM_init_pin(uint pinN)
    {
        gpio_set_function(pinN, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(pinN);  
        pwm_config c_pwm=pwm_get_default_config();
        pwm_config_set_clkdiv(&c_pwm,clock_get_hz(clk_sys)/(4.0*1750000));
        pwm_config_set_wrap(&c_pwm,3);//MAX PWM value
        pwm_init(slice_num,&c_pwm,true);
    }


    void hardAY_off()
    {
        if (type_sound==1)
        {
        pwm_set_gpio_level(CLK_AY_PIN, 0); // отключение AY
        // отключение AY смесителем регистр R7
        ay0_R7_m = reg_ay0[7];
        ay1_R7_m = reg_ay1[7];
        reg_ay0[7] = 0xff;
        reg_ay1[7] = 0xff; // второй чип
        }
    }
    void hardAY_on()
    {
        if (type_sound==1)
        {  
        pwm_set_gpio_level(CLK_AY_PIN, 2); // включение AY
                                           // включение AY смесителем регистр R7
        reg_ay0[7] = ay0_R7_m;
        reg_ay1[7] = ay1_R7_m;
        }
    }
//--------------------------------------------------------------
void Uninit_AY_595() {
    pio_sm_unclaim(pioAY595,  sm_AY595);
  //  pio_remove_program(pioAY595, 4);
}


//--------------------------------------------------------------
void Init_AY_595()
{
    PWM_init_pin(CLK_AY_PIN);
    pwm_set_gpio_level(CLK_AY_PIN,2);

    uint offset = pio_add_program(pioAY595, &program595);
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset + (program595.length-1)); 
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    //настройка side set
    sm_config_set_sideset_pins(&c,CLK_LATCH_595_BASE_PIN);
    sm_config_set_sideset(&c,2,false,false);
    for(int i=0;i<2;i++)
        {           
            pio_gpio_init(pioAY595, CLK_LATCH_595_BASE_PIN+i);
        }
    
    pio_sm_set_pins_with_mask(pioAY595, sm_AY595, 3u<<CLK_LATCH_595_BASE_PIN, 3u<<CLK_LATCH_595_BASE_PIN);
    pio_sm_set_pindirs_with_mask(pioAY595, sm_AY595,  3u<<CLK_LATCH_595_BASE_PIN,  3u<<CLK_LATCH_595_BASE_PIN);
    //


    
    pio_gpio_init(pioAY595, DATA_595_PIN);//резервируем под выход PIO
  
    pio_sm_set_consecutive_pindirs(pioAY595, sm_AY595, DATA_595_PIN, 1, true);//конфигурация пинов на выход

    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_out_pins(&c, DATA_595_PIN, 1);

    pio_sm_init(pioAY595, sm_AY595, offset, &c);
    pio_sm_set_enabled(pioAY595, sm_AY595, true);
//float fdiv=(clock_get_hz(clk_sys)/40000000);//частота 100 MHz
 float fdiv=(clock_get_hz(clk_sys)/80000000);//частота 50 MHz
  //    float fdiv=(clock_get_hz(clk_sys)/160000000);//частота 25 MHz
    uint32_t fdiv32=(uint32_t) (fdiv * (1 << 16));
    fdiv32=fdiv32&0xfffff000;//округление делителя
//sleep_ms(3000);
 // printf("fdiv32:%d #%x\n",fdiv32,fdiv32);
    pioAY595->sm[sm_AY595].clkdiv=fdiv32; //делитель для конкретной sm

    pioAY595->txf[sm_AY595]=0;
    pioAY595->txf[sm_AY595]=0;

/*        for (int i = 0; i < 4; i++)
     {  
      send595(0b00001100 ,0x00);// включение светодиодов
      sleep_ms(100);
      send595(0b00001100 ,0x00);// выключение светодиодов
      sleep_ms(100);
     }
      send595(0b01000000 ,0x00);// выключение светодиодов
 */
    
};
// 0000 0000     1111 1111
//
void __not_in_flash_func(send595)(uint16_t comand, uint8_t data)
{
    ay595 = comand | data;
    pioAY595->txf[sm_AY595] = (ay595 | beep595) << 16;
}
//
void __not_in_flash_func(send595_0)(uint16_t comand, uint8_t data)
{
    ay595 = comand | data;
    pioAY595->txf[sm_AY595] = (ay595 | beep595) << 16;
    pioAY595->txf[sm_AY595] = (AY0_Z | data | beep595) << 16;
}
//
void __not_in_flash_func(send595_1)(uint16_t comand, uint8_t data)
{
    ay595 = comand | data;
    pioAY595->txf[sm_AY595] = (ay595 | beep595) << 16;
    pioAY595->txf[sm_AY595] = (AY1_Z | data | beep595) << 16;
}
//-----------------------------------------------------------------------
void __not_in_flash_func(out_beep595)(bool val)
{
            
          if (val) beep595 = 0b0000000000000000;
            else   beep595 = 0b0001000000000000;
            pioAY595->txf[sm_AY595]=(ay595 | beep595)<<16;
}
//======================================================================

void __not_in_flash_func(audio_out_soft_ay)(void)
{	

	pwm_set_gpio_level(ZX_AY_PWM_PIN0, outL); // Право
    pwm_set_gpio_level(ZX_AY_PWM_PIN1, outR); // Лево

        sound_fdd();
		AY_data = get_AY_Out(AY_DELTA);										  ///??????
/* 		outL = vol_ay * (AY_data[0] + AY_data[1]); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE;
		outR = vol_ay * ((AY_data[2] + AY_data[1]) + (noise_fdd)); // звук перемешения головок //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE;  */
		outL =  (AY_data[0] + AY_data[1]); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE;
		outR =  ((AY_data[2] + AY_data[1]) + (noise_fdd)); // звук перемешения головок //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE; */      

#ifdef WS2812
music();
#endif


}
//----------------------------------------------------------------------
void __not_in_flash_func(audio_out_soft_ts)(void)
{	
	pwm_set_gpio_level(ZX_AY_PWM_PIN0, outL); // 
    pwm_set_gpio_level(ZX_AY_PWM_PIN1, outR); // 

		AY_data = get_AY_Out(AY_DELTA);			
		AY_data1 = get_AY_Out1(AY_DELTA);		
          sound_fdd();
     //    outL = vol_ay * ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE; 

    //    outR = vol_ay * (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1]) + (noise_fdd); // звук перемешения головок

         outL = ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE; 

        outR =  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1]) + (noise_fdd); // звук перемешения головок
#ifdef WS2812
music();
#endif     
        
}
//----------------------------------------------------------------------
void __not_in_flash_func (audio_out_hard_ay)(void)
{	
   
}
//======================================================================
void __not_in_flash_func(audio_out_i2s_ay)(void)
{	


        sound_fdd();

		AY_data = get_AY_Out(AY_DELTA);										  ///??????
	//uint16_t	outLL = vol_i2s*  (AY_data[0] + AY_data[1]+beepPWM); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE;
	//uint16_t	outRR = vol_i2s *  (AY_data[2] + AY_data[1]+ noise_fddI2S ); // звук перемешения головок //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE; */

	uint16_t	outLL =  (AY_data[0] + AY_data[1]+beepPWM); //+(b_save *10);;//+(b_save*10);//+valBEEP+valSAVE;
	uint16_t	outRR =   (AY_data[2] + AY_data[1]+beepPWM + noise_fddI2S ); // звук перемешения головок //+(b_beep*10);//+(b_save*10);//+valBEEP+valSAVE; */


   //     outL =  SoftSpec48_AY_table[AY_data[0]];
   //     outR =  SoftSpec48_AY_table[AY_data[2]];
     // i2s_out(outRR<<1,outLL<<1);//*2
      i2s_out(outRR,outLL);
    /*    if (conf.ay_filter) return; // фильтр частот
         outR =   Filtr_R(outR);
         outL =   Filtr_L(outL);
          */



}
//----------------------------------------------------------------------
void __not_in_flash_func(audio_out_i2s_ts)(void)
{	

       sound_fdd();

		AY_data = get_AY_Out(AY_DELTA);			
		AY_data1 = get_AY_Out1(AY_DELTA);										  ///??????
/* uint16_t	outLL = vol_i2s *  ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]+beepPWM); //
uint16_t   outRR =vol_i2s *  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1]+beepPWM + noise_fddI2S ) ; // звук перемешения головок
 */
uint16_t	outLL =   ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]+beepPWM); // outLL
uint16_t   outRR =  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1]+beepPWM + noise_fddI2S ) ; // звук перемешения головок outRR 

//uint16_t	outLL =   ( AY_data1[0] + AY_data1[1] + AY_data[0] + AY_data[1]); //
//uint16_t   outRR =  (AY_data1[2] + AY_data1[1] +AY_data[2] + AY_data[1]+ noise_fddI2S ) ; // звук перемешения головок

    //  i2s_out(outRR<<1,outLL<<1);//*2
      i2s_out(outRR,outLL);

}
//----------------------------------------------------------------------

//======================================================================
// beep & save

void __not_in_flash_func(hw_zx_set_snd_out)(bool val) // beep
{
	  beep_data=(beep_data&0b01)|(val<<1);   
}

void __not_in_flash_func(hw_out595_save_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	//gpio_put(ZX_BEEP_PIN,out);
    out_beep595(out);
	beep_data_old=beep_data;
	
}
#define BEEP 1
#if BEEP == 0
// вывод звук бипера на пин классика
void __not_in_flash_func(hw_outpin_save_out)(bool val)
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
void __not_in_flash_func(hw_outpin_save_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	beep_data_old=beep_data;
  //  pwm_set_gpio_level(ZX_BEEP_PIN,vol_ay * (beep_data<<4));
  // pwm_set_gpio_level(ZX_BEEP_PIN,conf.vol_bar * (beep_data<<4));
  //   if (out)pwm_set_gpio_level(ZX_BEEP_PIN,0);// pwm_set_gpio_level(ZX_BEEP_PIN,conf.vol_bar );
    //    else //pwm_set_gpio_level(ZX_BEEP_PIN,0);
     //   pwm_set_gpio_level(ZX_BEEP_PIN,conf.vol_bar );
   switch (out)
   {
   case 0:
    pwm_set_gpio_level(ZX_BEEP_PIN,0);
    break;
   case 1:
    pwm_set_gpio_level(ZX_BEEP_PIN,conf.vol_bar );
    break;   
  
   }


}
#endif

#if BEEP == 2
// вывод звук бипера на ШИМ 
void __not_in_flash_func(hw_outpin_save_out)(bool val)
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
void __not_in_flash_func(hw_outpin_save_out)(bool val)
{
	static bool out;
	beep_data=(beep_data&0b10);//|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
	
 if (out)  pwm_set_gpio_level(ZX_BEEP_PIN,conf.vol_bar);
 else  pwm_set_gpio_level(ZX_BEEP_PIN,0 );

 beep_data_old=beep_data;


}
#endif
//---------------------------------------------------------------------
void __not_in_flash_func(hw_outi2s_save_out)(bool val)
{
/*     static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
    //gpio_put(ZX_BEEP_PIN,out);
    //out_beep595(out);
	beep_data_old=beep_data;
 //   beepPWM = beep_data*64;
     beepPWM = beep_data*conf.vol_bar*64;

 */
    static bool out;
	beep_data=(beep_data&0b10)|(val<<0);
	out^=(beep_data==beep_data_old)?0:1;
    //gpio_put(ZX_BEEP_PIN,out);
    //out_beep595(out);
	beep_data_old=beep_data;
 //   beepPWM = beep_data*64;
   //  beepPWM = beep_data*conf.vol_bar*64;

 //if (out)  beepPWM =conf.vol_bar<<4;
 //else  beepPWM = 0 ;
   switch (out)
   {
   case 0:
    beepPWM = 0 ;
    break;
   case 1:
     beepPWM =conf.vol_bar<<4;
    break;   
  
   }


i2s_out(beepPWM,beepPWM);

}
//=====================================================================
void PWM_init_pin_SoftAY(uint pinN, uint PWM_WRAP){
	gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,1.0f); // делитель частоты для ay 1.0f    315/1.23 1.23529411765f
	pwm_config_set_wrap(&c_pwm,PWM_WRAP);//MAX PWM value =2000
	pwm_init(slice_num,&c_pwm,true);
}
void PWM_init_pin_SoftBeep(uint pinN, uint PWM_WRAP){
	gpio_set_function(pinN, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(pinN);
	pwm_config c_pwm=pwm_get_default_config();
	pwm_config_set_clkdiv(&c_pwm,2.0f); // делитель частоты для ay 1.0f    315/1.23 1.23529411765f
	pwm_config_set_wrap(&c_pwm,PWM_WRAP);//MAX PWM value =100
	pwm_init(slice_num,&c_pwm,true);
}
//======================================================================
// переключение режимов вывода звука
void select_audio(uint8_t num)
{

    chip_ay = 0;
    type_sound = num>>1; // разделить на 2
    switch (num)
    {
    case 0 /*Soft   AY Sound */:
        AY_out_FFFD = ay_set_reg_soft;
        AY_out_BFFD = AY_set_reg; // ay_set_data_soft
        audio_out = audio_out_soft_ay;
        // пины вывода звука soft ay
        PWM_init_pin_SoftAY(ZX_AY_PWM_PIN0,1000);
        PWM_init_pin_SoftAY(ZX_AY_PWM_PIN1,1000);
        // beep
     #if BEEP == 0  
          gpio_init(ZX_BEEP_PIN);
          gpio_set_dir(ZX_BEEP_PIN, GPIO_OUT);
     #endif    
     #if BEEP == 1
        PWM_init_pin_SoftBeep(ZX_BEEP_PIN,100);
     #endif
        hw_zx_set_save_out = hw_outpin_save_out;
        break;
        break;
    case 1 /*Soft TurboSound*/:
        AY_out_FFFD = ay_set_reg_soft_ts;
        AY_out_BFFD = ay_set_data_soft_ts;
        audio_out = audio_out_soft_ts;
        // пины вывода звука soft ay
        PWM_init_pin_SoftAY(ZX_AY_PWM_PIN0,1000);
        PWM_init_pin_SoftAY(ZX_AY_PWM_PIN1,1000);
        // beep
     #if BEEP == 0  
          gpio_init(ZX_BEEP_PIN);
          gpio_set_dir(ZX_BEEP_PIN, GPIO_OUT);
     #endif    
     #if BEEP == 1
        PWM_init_pin_SoftBeep(ZX_BEEP_PIN,100);
     #endif
        hw_zx_set_save_out = hw_outpin_save_out;
        break;

    case 2 /*Hard   AY Sound */:
        Init_AY_595();
        AY_out_FFFD = ay_set_reg_hard;
        AY_out_BFFD = ay_set_data_hard;
        audio_out = audio_out_hard_ay;
        hw_zx_set_save_out = hw_out595_save_out;
        ay595 = 0;
        beep595 = 0;
        //send595(AY_RES, 0x00);
        //send595(AY_Z, 0x00);
        send595(AY_RES, 0x00);
        send595(AY_Z, 0x00);
        break;

    case 3 /*Hard TurboSound */:
        Init_AY_595();
        AY_out_FFFD = ay_set_reg_hard_ts;
        AY_out_BFFD = ay_set_data_hard_ts;
        audio_out = audio_out_hard_ay;
        hw_zx_set_save_out = hw_out595_save_out;
        ay595 = 0;
        beep595 = 0;
        //send595(AY_RES, 0x00);
        //send595(AY_Z, 0x00);
        send595(AY_RES, 0x00);
        send595(AY_Z, 0x00);
        break;
        //------------------------------------
    case 4 /*I2S  AY Sound */:
        i2s_init();
        audio_out = audio_out_i2s_ay;
        AY_out_FFFD = ay_set_reg_soft;
        AY_out_BFFD = AY_set_reg; // ay_set_data_soft

        // beep
      //  gpio_init(ZX_BEEP_PIN);
      //  gpio_set_dir(ZX_BEEP_PIN, GPIO_OUT);
        hw_zx_set_save_out = hw_outi2s_save_out;
        break;
   //------------------------------------
    case 5 /*I2S  TS Sound */:

        
       //   gpio_init(ZX_AY_PWM_PIN0);
       //   gpio_init(ZX_AY_PWM_PIN1);


       i2s_init();
        audio_out = audio_out_i2s_ts;
        AY_out_FFFD = ay_set_reg_soft_ts;
        AY_out_BFFD = ay_set_data_soft_ts;

        // beep
      //  gpio_init(ZX_BEEP_PIN);
      //  gpio_set_dir(ZX_BEEP_PIN, GPIO_OUT);
        hw_zx_set_save_out = hw_outi2s_save_out;
        break;

    default:
        break;
    }
}
//-----------------------------------------------------------------
#ifdef WS2812


void music ()
{
ws2812_set_rgb(ay1_R8/8, ay1_R9/8, ay1_R10/8);
 }
 #endif
//=================================================================
#define SPS 44000//1000000/AY_SAMPLE_RATE //44000UL  // частота дискретизации АЦП
#define Trc 0.001f  //Trc = R1*C1 = 10000 Ом * (0.1/1000000) Ф = 0.001 с
#define K (SPS*Trc)

uint8_t Filtr_L(uint8_t data)
{
   static uint32_t Dacc_l = 0;
   static uint8_t Dout_l = 0;

   Dacc_l = Dacc_l + data- Dout_l;
   Dout_l = Dacc_l/(uint32_t)K;

   return Dout_l*2;
}

uint8_t Filtr_R(uint8_t data)
{
   static uint32_t Dacc_r = 0;
   static uint8_t Dout_r = 0;

   Dacc_r = Dacc_r + data - Dout_r;
   Dout_r = Dacc_r/(uint32_t)K;

   return Dout_r*2;
}

//===================================================================
// создание таблицы текущей громкости каналов AY
void init_vol_ay(uint8_t vol)
{
    if (type_sound == 0) // soft AY
    {
       if (conf.vol_bar>32) conf.vol_bar=32;
    } 
    if (type_sound == 1)
        vol = 0; // hard ay

    for (uint8_t i = 0; i < 16; i++)
    {
        ampls_AY[i] = vol *  ( ampls[conf.ampl_tab][i]);
    }


    //  если громкость больше 16 0x10
    for (uint8_t i = 16; i < 32; i++)
    {
        ampls_AY[i] = vol *  (ampls[conf.ampl_tab][15]);
    }



    // создание таблицы текущей громкости огибающей AY занимает RAM 2*32*16=1024!
    {
        for (uint8_t j = 0; j < 16; j++)
        {
            for (uint8_t i = 0; i < 32; i++)
            {
                Envelopes[j][i] = ampls_AY[Envelopes_const[j][i]];
            }
        }
    }
}
//===================================================================