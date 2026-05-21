#    Конфигурация для платы WS_ZERO1
#######################################################
#    WS_ZERO1 Wave Share Zero1  2026.05.02 
#    PIO 0 I2S or HARD_AY_595 SM 0 SM FREE 1,2,3
#    PIO 1 PS/2 (2) and VIDEO  TFT (0 1) PSRAM_BOARD(SM=3)
#######################################################

set (M_VERSION z0)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE

WS_ZERO1
HDMI_ONLY # только HDMI
LED_BOARD=255  # 255 светодиод не подключен
VOLTAGE_RUN=VREG_VOLTAGE_1_30
VOLTAGE_WORK=VREG_VOLTAGE_1_30

#PS/2 клавиатура
beginPS2_PIN=0
PIO_PS2=pio0
SM_PS2=3 #  

#VIDEO
HDMI_PIN_ORDER_REVERSE
beginVideo_PIN=22
beginHDMI_PIN_data=22
beginHDMI_PIN_clk=28
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

TFT_CLK_PIN=22
TFT_DATA_PIN=23
TFT_RST_PIN=24
TFT_DC_PIN=25
TFT_CS_PIN=26
TFT_LED_PIN=27

##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_CS=21
SDCARD_PIN_SCK=18
SDCARD_PIN_MOSI=19
SDCARD_PIN_MISO=20

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
ZX_AY_PWM_R=10 
ZX_AY_PWM_L=11 
ZX_BEEP_PIN=12 
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
I2S_DATA_PIN=10
I2S_CLK_BASE_PIN=11
PIO_I2S=pio0 # Only  pio0 !
SM_I2S=0
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=1

PIN_ZX_LOAD=17

# NES JOY
D_JOY_DATA_PIN=7
D_JOY_CLK_PIN=8
D_JOY_LATCH_PIN=9

DELAY_JOY=200 #задержка NES JOY
DELAY_KEY=127 #задержка USB keyboard

)
########### PSRAM для WS ZERO 1 #########################################
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_NOSUPORT
            )
            message("Конфигурация: WS_ZERO1 + PICO_RP2040 (NO_PSRAM)")

######################################################################### 