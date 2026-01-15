#pragma once
#include "inttypes.h"
#include "stdbool.h"
#include <stdio.h>
#include "kb_u_codes.h"



 
//uint8_t key_report[8];
//static int16_t delay_key;

//bool (*decode_key)();  // определение указателя на функцию  
//void init_decode_key(uint8_t kms);
//bool decode_USB();
//bool decode_USB_HID();
bool init_usb_hid(void);
bool decode_key (bool menu_mode);
bool decode_key_joy();
void keyboard (void);
void task_usb(void);
void wait_enter(void);
void wait_esc(void);



