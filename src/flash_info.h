#ifndef FLASH_INFO_H
#define FLASH_INFO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Глобальный массив для хранения JEDEC ID (как в вашем main.cpp)
extern uint8_t rx[4];

// Чтение JEDEC ID
void flash_info(void);

// Получение размера в мегабайтах из уже прочитанного rx[]
uint32_t flash_get_size_mb(void);

// Получение названия производителя
const char* flash_get_manufacturer(void);

// Проверка, был ли прочитан JEDEC ID
bool flash_is_read(void);

#ifdef __cplusplus
}
#endif

#endif // FLASH_INFO_H