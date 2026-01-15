#include "g_config.h"
#include <pico/stdlib.h>

//#include "hardware/clocks.h"

int	null_printf(const char *str, ...){return 0;};

void g_delay_ms(int delay)
{
    sleep_ms(delay);
    
};






