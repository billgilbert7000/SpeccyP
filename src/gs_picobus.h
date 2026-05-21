#pragma once
#include "config.h"
#include <stdio.h>
#include "stdbool.h"
#include "hardware/gpio.h"

//################################################################
//занятость шины picobus  while (picobus_busy) { busy_wait_us(100);} // ожидание свободной шины picobus 
#define PBUS_CS_0 picobus_busy=true
#define PBUS_CS_1 picobus_busy=false
//################################################################


void init_picobus(void);
void out_GSP(uint8_t port,uint8_t value);
uint8_t in_GSP(uint8_t port);
void sys_GS(uint8_t command_sys);