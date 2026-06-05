#    Конфигурация для платы  GSP_M2
#######################################################
#    GENERAL SOUND M2
#    PIO 0 I2S or HARD_AY_595 SM 0 SM FREE 1,2,3
#    PIO 1 PS/2 (2) and VIDEO  TFT (0 1) PSRAM_BOARD(SM=3) - нет
#######################################################
set (M_VERSION m2)# версия платы

target_compile_definitions(${PROJECT_NAME} PRIVATE
MURM2
LED_BOARD=25
VOLTAGE_RUN=VREG_VOLTAGE_1_30
#########################################################################
##PicoBus ONLY GENERAL SOUND
PBUS_OUT_PIN=10
PBUS_IN_PIN=11
PBUS_PIO=pio0   #only pio 0
## Beep
BEEP_PIN=9  #  # на IN
#######################################################################

#PS/2 клавиатура
beginPS2_PIN=2
PIO_PS2=pio1 
SM_PS2=3 # 

#VIDEO
beginVideo_PIN=12

beginHDMI_PIN_data=beginVideo_PIN+2
beginHDMI_PIN_clk=beginVideo_PIN

PIO_VIDEO=pio1
PIO_VIDEO_ADDR=pio1
SM_VIDEO=0
SM_CONV=1
#
#TFT
PIO_SPI_TFT=pio1
SM_SPI_TFT=0
PIO_SPI_TFT_CONV=pio1
SM_SPI_TFT_CONV=1

TFT_CLK_PIN=19
TFT_DATA_PIN=18
TFT_RST_PIN=14
TFT_DC_PIN=16
TFT_CS_PIN=12
TFT_LED_PIN=15

##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_CS=5
SDCARD_PIN_SCK=6
SDCARD_PIN_MOSI=7
SDCARD_PIN_MISO=4

##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PSRAM_BUTTER_PIN_CS=8

ZX_BEEP_PIN=9 ## не используется

MAX_VOL_PWM=100 ## не используется
 DEFAULT_VOLUME_PWM=50 ## не используется

DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=0

PIN_ZX_LOAD=22


# NES JOY
D_JOY_DATA_PIN=26
D_JOY_CLK_PIN=20
D_JOY_LATCH_PIN=21

DELAY_KEY=127 #задержка USB keyboard
DELAY_JOY=200 #задержка NES JOY

)
###################### PSRAM для SPECCY GSP M2 #################################
            if( ${PICO_PLATFORM} MATCHES "rp2040")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_NOSUPORT
            )
            message("Конфигурация: SPECCY GSP M2 + PICO_RP2040 (NO_PSRAM)")
        
        elseif( ${PICO_PLATFORM} MATCHES "rp2350")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
           PSRAM_BUTTER
           ## NO_PSRAM
            )
            message("Конфигурация: SPECCY GSP M2 + PICO_RP2350 (PSRAM_BUTTER)")
        endif()
##############################################################################