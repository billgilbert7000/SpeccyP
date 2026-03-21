#include "config.h"  
#include "zx_emu/zx_machine.h"
#include "joy.h"
#include "tusb.h"

volatile int us_val=0;

void d_sleep_us(uint us){
	for(uint i=0;i<us;i++){
		for(int j=0;j<25;j++){
			us_val++;
		}
	}
}
//---------------------------------------------


uint8_t d_joy_get_data()
{
    uint8_t data;
    gpio_put(D_JOY_LATCH_PIN, 1);
    // gpio_put(D_JOY_CLK_PIN,1);
    d_sleep_us(12);
    gpio_put(D_JOY_LATCH_PIN, 0);
    d_sleep_us(6);
    data=0;
    for (int i = 0; i < 8; i++)
    {

        gpio_put(D_JOY_CLK_PIN, 0);
        d_sleep_us(10);
        data <<= 1;
        data |= gpio_get(D_JOY_DATA_PIN);



        d_sleep_us(10);
        gpio_put(D_JOY_CLK_PIN, 1);
        d_sleep_us(10);
    }

  	
    if ((data==0) && (joy_k==0))
    {
        joy_connected = false;
       // data = joy_k;
       // joy_key_ext = joy_k;
        return 0; 
    }
    else
    {  joy_key_ext =0;
        joy_connected = true;
        data = (data & 0x0f) | ((data >> 2) & 0x30) | ((data << 3) & 0x80) | ((data << 1) & 0x40);
        data = ~data; // инверсия битов data
        
        if (!data) return joy_k; // выход если ничего не нажато
        // защита от дурака
        if ((data & 0b00000011) == 0b00000011) data &= 0b11111100;   // если left и right нажаты одновременно то ничего не нажато
        if ((data & 0b00001100) == 0b00001100) data &= 0b11110011;   // если up и down нажаты одновременно то ничего не нажато
       
     //   data &= 0b01111111; // отсекаем кнопку joy [START] или не отсекаем
/* 
        if (data == 0xa0)   joy_key_ext = 0xa0; // START+[A] PAUSE
        if (data == 0x88)   joy_key_ext = 0x88; // START+ Up SetUp
        if (data == 0x84)   joy_key_ext = 0x84; // START+Down фм

        if (data == 0x81)   joy_key_ext = 0x81; // START+ Right LOAD
        if (data == 0x82)   joy_key_ext = 0x82; // START+ Left SAVE

 */

// sleep_ms(200);

if (is_menu_mode) sleep_ms(DELAY_JOY);

        return data | joy_k;
    }
};
//================================================================================

bool decode_joy()
{
   #if D_JOY_CLK_PIN==255        
      return false;
   #endif   

   #if DEVICE_HDMI90_I2S      
      return false;
   #endif   

    data_joy = d_joy_get_data(); 
   joy_key_ext = data_joy;

   if (data_joy != old_data_joy)
        {
            old_data_joy = data_joy;
        //    if (is_menu_mode) sleep_ms(DELAY_JOY);
            return true;
        }
   
            return false;
      

}

//================================================================================
/*
UP 0x08      0000 1000
DW 0x04      0000 0100
LF 0x02      0000 0010
RH 0x01      0000 0001
A 0x20       0010 0000
B 0x10       0001 0000
С 0x30       0011 0000
MODE  0x40   0100 0000
START 0x80   1000 0000 

*/

//================================================================================
bool decode_joy_to_keyboard(void)
{ 
 tuh_task(); // tinyusb host task
   static int16_t delay_key;

   data_joy = d_joy_get_data();// если есть денди джой

        if ((data_joy  == 0x84) || (data_joy  == 0x88))
{
        old_data_joy = data_joy;
        return true;
}


        if ((data_joy & 0x01) == 0x01)
            kb_st_ps2.u[2] |= KB_U2_RIGHT;
        else
            kb_st_ps2.u[2] &= ~KB_U2_RIGHT;

        if ((data_joy & 0x02) == 0x02)
            kb_st_ps2.u[2] |= KB_U2_LEFT;
        else
            kb_st_ps2.u[2] &= ~KB_U2_LEFT;

        if ((data_joy & 0x04) == 0x04)
            kb_st_ps2.u[2] |= KB_U2_DOWN;
        else
            kb_st_ps2.u[2] &= ~KB_U2_DOWN;

        if ((data_joy & 0x08) == 0x08)
            kb_st_ps2.u[2] |= KB_U2_UP;
        else
            kb_st_ps2.u[2] &= ~KB_U2_UP;

        if ((data_joy & 0x20) == 0x20) // joy [A]
            kb_st_ps2.u[1] |= KB_U1_ENTER;
        else
            kb_st_ps2.u[1] &= ~KB_U1_ENTER;

        if ((data_joy & 0x40) == 0x40) // joy [X]
            kb_st_ps2.u[1] |= KB_U1_ESC;
        else
            kb_st_ps2.u[1] &= ~KB_U1_ESC;

          if ((data_joy & 0x10) == 0x10) // joy [B]
            kb_st_ps2.u[1] |= KB_U1_SPACE;
        else
            kb_st_ps2.u[1] &= ~KB_U1_SPACE;

            old_data_joy = data_joy;
        return true;
}
//================================================================================

void d_joy_init()
{
   #if DEVICE_HDMI90_I2S   
      joy_connected=false;
      return;
   #endif   


    gpio_init(D_JOY_CLK_PIN);
    gpio_set_dir(D_JOY_CLK_PIN, GPIO_OUT);
    gpio_init(D_JOY_LATCH_PIN);
    gpio_set_dir(D_JOY_LATCH_PIN, GPIO_OUT);

    gpio_init(D_JOY_DATA_PIN);
    gpio_set_dir(D_JOY_DATA_PIN, GPIO_IN);
  // gpio_pull_up(D_JOY_DATA_PIN);// !!!!!!!!!!!!!!!!!!!
    // gpio_pull_down(D_JOY_DATA_PIN);
    gpio_put(D_JOY_LATCH_PIN, 0);

 data_joy = 0;
 old_data_joy = 0x00;
 joy_connected=false;

}
