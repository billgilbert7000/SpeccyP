#pragma once
#include "inttypes.h" 

void AY_select_reg1(uint8_t N_reg);
uint8_t AY_get_reg1();
void AY_set_reg1(uint8_t val);

uint16_t*  get_AY_Out1(uint8_t delta);
void  AY_reset1();
void AY_print_state_debug1();

