#include "ff.h"
#include <stdio.h>



#if defined(RTC_NOVA) || defined(RTC_SMUC) || defined(RTC_GLUK)

#include "gs_picobus.h"

// Функция, которую FatFs будет вызывать для получения времени
DWORD get_fattime(void) {
     if (!rtc_enable) {
        // Если RTC не инициализирован, возвращаем фиксированную дату
        return ((2000 - 1980) << 25) | ((uint32_t)11 << 21) | ((uint32_t)7 << 16) |
               ((uint32_t)12 << 11) | ((uint32_t)34 << 5) | ((uint32_t)56 >> 1);
    }  
    sys_GS(RTC_BIN);
 
        return ((tx_buffer[0] + 20) << 25) | 
               ((uint32_t)tx_buffer[1] << 21) | 
               ((uint32_t)tx_buffer[2] << 16) |
               ((uint32_t)tx_buffer[3]  << 11) | 
               ((uint32_t)tx_buffer[4]  << 5)  | 
               ((uint32_t)tx_buffer[5]  >> 1);

}
#else
DWORD get_fattime(void) {
         // возвращаем фиксированную дату
        return ((2000 - 1980) << 25) | ((uint32_t)11 << 21) | ((uint32_t)7 << 16) |
               ((uint32_t)12 << 11) | ((uint32_t)34 << 5) | ((uint32_t)56 >> 1);
    }  


#endif
