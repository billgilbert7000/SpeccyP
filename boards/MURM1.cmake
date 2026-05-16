
#######################################################
#    Конфигурация для платы MURM1
#    MURM1  два вида PSRARM поддерживается
#    PIO 0 I2S or HARD_AY_595 SM 0 SM FREE 1,2,3
#    PIO 1 PS/2 (2) and VIDEO  TFT (0 1) PSRAM_BOARD(SM=3)
#    
# VIDEO         4 DMA
# I2S AUDIO     2 DMA
# PSRAM BOARD   2 или 3 DMA
# PS/2          2 DMA
# SD            2 DMA
#######################################################
set (M_VERSION m1)# версия платы

target_compile_definitions(${PROJECT_NAME} PRIVATE

MURM1
LED_BOARD=25
VOLTAGE_RUN=VREG_VOLTAGE_1_30
VOLTAGE_WORK=VREG_VOLTAGE_1_30
#########################################################################
##PicoBus ONLY GENERAL SOUND
PBUS_OUT_PIN=26
PBUS_IN_PIN=27
PBUS_PIO=pio0   #only pio 0
## Beep
BEEP_PIN=28  # на IN
#######################################################################

#PS/2 клавиатура
beginPS2_PIN=0
PIO_PS2=pio0
SM_PS2=3 #  

#VIDEO
beginVideo_PIN=6

beginHDMI_PIN_data=beginVideo_PIN+2
beginHDMI_PIN_clk=beginVideo_PIN
# video pio1 и sm 2шт 
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

TFT_CLK_PIN=13
TFT_DATA_PIN=12
TFT_RST_PIN=8
TFT_DC_PIN=10
TFT_CS_PIN=6 
TFT_LED_PIN=9

##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_CS=5
SDCARD_PIN_SCK=2
SDCARD_PIN_MOSI=3
SDCARD_PIN_MISO=4

##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PIO_PSRAM=pio1 
SM_PSRAM=3  # критично при отключении pio  программы
# PSRAM_MUTEX=1
PSRAM_SPINLOCK=1
PSRAM_ASYNC
PSRAM_PIN_CS=18
PSRAM_PIN_SCK=19
PSRAM_PIN_MOSI=20
PSRAM_PIN_MISO=21
PSRAM_BUTTER_PIN_CS=19 #это PSRAM_PIN_CS для платы murm1 psram бутерброд
##AUDIO PWM
ZX_AY_PWM_R=26 
ZX_AY_PWM_L=27 
ZX_BEEP_PIN=28 
BEEP_PWM_WRAP=1000 # ШИМ 
AY_PWM_WRAP=4000 # ШИМ 
MAX_VOL_PWM=100
DEFAULT_VOLUME_PWM=50
BEEP=12 # бипер микшируется в PWM R/L  или на gpio

#BEEP=1 #вывод звук бипера на отдельный шим
##AUDIO 595  hard AY 
PIO_AY595=pio0 
SM_AY595=0 
CLK_AY_PIN=29 # CLK_AY_PIN=21
CLK_LATCH_595_BASE_PIN=26 # и 27 
DATA_595_PIN=28
##AUDIO i2s
I2S_DATA_PIN=26
I2S_CLK_BASE_PIN=27
I2S_FREQ=111111 ##111111 Hz
PIO_I2S=pio0 # Only  pio0 !
SM_I2S=0
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=1

PIN_ZX_LOAD=22

# NES JOY
D_JOY_DATA_PIN=16
D_JOY_CLK_PIN=14
D_JOY_LATCH_PIN=15

DELAY_JOY=200 #задержка NES JOY
DELAY_KEY=127 #задержка USB keyboard

)

# Конфигурация PSRAM для MURM2
          if( ${PICO_PLATFORM} MATCHES "rp2040")
            target_compile_definitions(${PROJECT_NAME} PRIVATE
           PSRAM_BOARD
           # PSRAM_NOSUPORT
             )
            message("Конфигурация: MURM1 + PICO_RP2040 (PSRAM)")
        
        elseif( ${PICO_PLATFORM} MATCHES "rp2350")
            # Для PICO_RP2350 определяем все три варианта
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_BUTTER_OR_PSRAM_PSRAM_BOARD
           # PSRAM_BOARD
           # PSRAM_BUTTER
           # PSRAM_NOSUPORT   
           # NO_PSRAM
            )
            message("Конфигурация: MURM1 + PICO_RP2350 (PSRAM + PSRAM_BUTTER)")
        endif()    