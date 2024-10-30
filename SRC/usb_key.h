#pragma once
#include "inttypes.h"
#include <stdio.h>
#include "kb_u_codes.h"


extern kb_u_state kb_st_ps2;
uint8_t ibuff[33] ;
bool flag_usb_kb;
uint8_t key_report[8];
int16_t delay_key;

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

#define DELAY_KEY  127// 120 // задержка USB keyboard

