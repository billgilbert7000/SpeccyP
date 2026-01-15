#include <stdio.h>
#include "gs_picobus.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hw_util.h"



// GS picobus
#if defined  GENERAL_SOUND

#include "picobus/picobus.h"

// ========== КОНФИГУРАЦИЯ ==========
//#define PICOBUS_BAUDRATE (1000*1000)     // Скорость шины picobus 
//#define LINKOUT_PIN     = 20;
//#define LINKIN_PIN      = 21;
/* const uint8_t LINKOUT_PIN     = 20;
const uint8_t LINKIN_PIN      = 21;
#define PIO_picobus pio0 */
//################################################################
//uint8_t tx_buffer[64];// буфер picobus
//int linkout_sm;
//int linkin_sm;
// gpio_put(LED_BOARD, 0);
//################################################################
// Инициализация/переинициализация picobus
void init_picobus(void)
{
  //  gpio_put(LED_BOARD, 0);  
    picobus_link_init();
//
      //   gpio_put(LED_BOARD, 1);
     uint8_t operator;
     uint8_t value;
 //   printf("receive_buffer .....");
 //  receive_buffer( rx_buffer, 2); // получаем 2 байта
PBUS_CS_0;
    const uint8_t init_msg[] = { PICOBUS_CONNECT , PICOBUS_CONNECT}; // команда инициализации если вдруг picobus работает
    send_buffer(init_msg, sizeof(init_msg));
    if (receive_acked_byte(&value) == LINK_BYTE_NONE) send_init_sequence(); // Инициализация передачи
PBUS_CS_1;
 /*    const uint8_t init_msg[] = { PICOBUS_CONNECT,PICOBUS_CONNECT }; // команда инициализации если вдруг picobus работает
 repeat:    
    send_buffer(init_msg, sizeof(init_msg));
  
   receive_acked_byte(&value); 
 //    if (value==PICOBUS_CONNECT ) return;
     goto repeat;




 */



   // send_init_sequence(); // Инициализация передачи
   // gpio_put(LED_BOARD, 0);          
}
//################################################################################
void (out_GSP)(uint8_t command_bus,uint8_t value)
{
PBUS_CS_0;

        tx_buffer[0] = command_bus;
        tx_buffer[1] =  value;
        send_buffer(tx_buffer, 2);
PBUS_CS_1;
}
//################################################################################
uint8_t (in_GSP)(uint8_t command_bus)
{
PBUS_CS_0;
        tx_buffer[0] = command_bus;
        send_buffer( tx_buffer, 2);
        receive_buffer(tx_buffer , 1 );
PBUS_CS_1;
      //  printf("IN(0xBB) : 0x%02X\n",  tx_buffer[0] );
    return tx_buffer[0];
}
//################################################################################
void sys_GS(uint8_t command_sys)
{
PBUS_CS_0;    
        tx_buffer[0] = 0;
        tx_buffer[1] =  command_sys;
    //    gpio_put(LED_BOARD, 0);  

  
        send_buffer(tx_buffer, 2);
//gpio_put(LED_BOARD, 1); 
PBUS_CS_1;
       if (command_sys==0)   receive_buffer(tx_buffer , 64 ); // получение инфы от GS
       if (command_sys==1) return; // сброс GS
        

     //   printf("Read : 0x%02X 0x%02X 0x%02X 0x%02X  \n",  tx_buffer[0] , tx_buffer[1],  tx_buffer[2] , tx_buffer[3] );
  //  return tx_buffer[0];
}
//################################################################################





//static void fast(
/* 
  --== Порты со стороны спектрума ==--

порт $B3, запись - регистр данных GS (на запись), устанавливает data bit (см. ниже).

порт $B3, чтение - регистр данных GS (на чтение), сбрасывает data bit.

порт $BB, запись - регистр команд GS (на запись, аналогично регистру данных), устанавливает command bit.

порт $BB, чтение - биты состояния GS:
 D7 - data bit
 D6..D1 - не определены
 D0 - command bit
  */
//порт $B3, запись - регистр данных GS
// формат передачи 3 байта если это не служкбный код
// 0. код 0-служебный
//    код 1 часы ;)
/*
#define SERVICE_COMMAND 0x00 // служебная команда 
// дефайны эмуляции портов GS для общения по i2c spi и так далее
#define GS_READ_IN_B3      0x01      // in(0xB3)
#define GS_STATUS_IN_BB    0x02      // in(0xBB)
#define GS_WRITE_OUT_B3    0x03      // out(0xB3)
#define GS_COMMAND_OUT_BB  0x04      // out(0xBB) */


#endif