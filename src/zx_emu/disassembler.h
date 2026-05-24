#pragma once
#ifndef DISASSEMBLER_H_
#define DISASSEMBLER_H_

#include <stdint.h>
#include "Z80.h"

// Глобальные переменные для дизассемблера
extern char tmp_opcode[16];
extern uint16_t dis_adres;
extern uint16_t address_pc;

// Функции дизассемблера
void disassembler(void);
uint8_t opcode_z80(void);
void list_disassm(void);
void list_dump(void);
int OpcodeLen(uint16_t p);

#endif // DISASSEMBLER_H_