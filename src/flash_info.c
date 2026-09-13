#include "flash_info.h"
#include <hardware/flash.h>

// Глобальный массив JEDEC ID
uint8_t rx[4] = {0};

// Чтение JEDEC ID
void __not_in_flash_func(flash_info)(void) {
    if (rx[0] == 0)
     {
        uint8_t tx[4] = {0x9F};  // Команда Read JEDEC ID
        flash_do_cmd(tx, rx, 4);
    }
}

// Получение размера в мегабайтах
uint32_t flash_get_size_mb(void) {
    if (rx[0] == 0) return 0;
    // Размер вычисляется как 2^rx[3]
    // rx[3] = 0x14 (20) → 1 MB
    // rx[3] = 0x15 (21) → 2 MB
    // rx[3] = 0x16 (22) → 4 MB
    // rx[3] = 0x17 (23) → 8 MB
    // rx[3] = 0x18 (24) → 16 MB
    if (rx[3] >= 0x14 && rx[3] <= 0x1A) {
        uint32_t size_bytes = (uint32_t)(1 << rx[3]);
     //   return rx[3];
        return size_bytes / (1024 * 1024);
    }
    return 0;
}

// Получение названия производителя
const char* flash_get_manufacturer(void) {
    if (rx[0] == 0) return "*";
    
    switch (rx[1]) {
        case 0xEF: return "Winbond";
        case 0x85: return "Puya";
     //   case 0xC2: return "Macronix";
     //   case 0xC8: return "GigaDevice";
        case 0x1C: return "EON";
        case 0x20: return "XMC";
        case 0xBF: return "SST";
        case 0x9D: return "ISSI";
        default:   return "---";
    }
}

// Проверка, был ли прочитан JEDEC ID
bool flash_is_read(void) {
    return rx[0] != 0;
}