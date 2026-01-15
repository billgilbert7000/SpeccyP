#pragma once
#include "inttypes.h"
#include "stdio.h"
#include "pico/stdlib.h"
#define fast __not_in_flash_func
//#define _POSIX_TIMERS

void ext_delay_ms(uint32_t delay);
void ext_delay_us(uint32_t delay);
//uint64_t ext_get_ns();
uint32_t get_ticks();

//функции можно подменить выводом на консоль или отключить

#define G_PRINTF  printf
#define G_PRINTF_INFO  printf
#define G_PRINTF_DEBUG  printf
#define G_PRINTF_ERROR  printf
