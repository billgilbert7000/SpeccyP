#pragma once

#include <stddef.h>
#include <stdint.h>

#define QCPM85_DRVA    (0b01)
#define QCPM85_DRVB    (0b10)
#define QCPM85_DRVMASK (0b11)
#define QCPM85_SIDE    (1<<4)
#define QCPM85_MOTOR   (1<<5)

void machine_Quorum1024(Z80 *cpu);
void init_rom_ram_Q1024();
void fast(rom_select_Quorum1024)();
void fast(pager7ffd_Quorum1024)(uint8_t val);
