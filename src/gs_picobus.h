#pragma once
#include "config.h"
#include <stdio.h>
#include "stdbool.h"
#include "hardware/gpio.h"
void init_picobus(void);
void out_GSP(uint8_t port,uint8_t value);
uint8_t in_GSP(uint8_t port);
void sys_GS(uint8_t command_sys);