#include "config.h"
#include <stdio.h>



//-----------------
#if defined(NO_PSRAM_21)  

void config_defain(void)
{
        conf.version =0137;
        conf.autorun =0;
      //  conf.ay_filter= 1;// 0 off фильтр частот
        conf.turbo = 0;// turbo/normal
        conf.mashine = PENT128;
        conf.ay = 3;// hard turbo sound
        conf.joyMode = 0;
        conf.sound_ffd = 0;
        conf.spi_bd = 1;
        conf.tft_bright = 70;
        conf.ampl_tab = 0;
        conf.autorun = 0; //off   1 trdos
        conf.DiskName[4][LENF+1]; // имя диска инфо
        conf.vol_bar = 16; //громкость
        
        conf.activefilename[0] = 0;
}

#else

void config_defain(void)
{
        conf.version =0137;
        conf.autorun =0;
       // conf.ay_filter= 1;// 0 off фильтр частот
        conf.turbo = 0;// turbo/normal
        conf.mashine = PENT128;
        conf.ay = 1;// soft turbo sound
        conf.joyMode = 0;
        conf.sound_ffd = 0;
        conf.spi_bd = 1;
        conf.tft_bright = 70;
        conf.ampl_tab = 0;
        conf.autorun = 0; //off   1 trdos
        conf.DiskName[4][LENF+1]; // имя диска инфо
        conf.vol_bar = 16; //громкость
        
        conf.activefilename[0] = 0;
}
#endif