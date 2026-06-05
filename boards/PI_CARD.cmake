#######################################################
#    Конфигурация для платы PI_CARD
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
#    pio 0 используется для управления GPIO 16...48
#######################################################
set (M_VERSION pc)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
VOLTAGE_RUN=VREG_VOLTAGE_1_40
PI_CARD
HDMI_ONLY # только HDMI
SOUND_I2S_ONLY # только i2s звук
RP2350B
  PICO_PIO_USE_GPIO_BASE # для использования GPIO >31 раскомментировать если RP2350B 

#PS/2 клавиатура
beginPS2_PIN=38
PIO_PS2=pio0 # Only  pio0 ! # использование GPIO 16...48
SM_PS2=2

#VIDEO
beginVideo_PIN=12

beginHDMI_PIN_data=beginVideo_PIN+2
beginHDMI_PIN_clk=beginVideo_PIN

PIO_VIDEO=pio1
PIO_VIDEO_ADDR=pio1
SM_VIDEO=-1
SM_CONV=-1
#
#TFT на PiCard это отсутствует
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
#SDCARD_SPI_BUS spi0
SDCARD_PIN_CS=1
SDCARD_PIN_SCK=2
SDCARD_PIN_MOSI=3
SDCARD_PIN_MISO=4
##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PSRAM_BUTTER_PIN_CS=0

##AUDIO PWM на PiCard это отсутствует
ZX_AY_PWM_R=45
ZX_AY_PWM_L=46
ZX_BEEP_PIN=47
BEEP_PWM_WRAP=1000 # ШИМ 
AY_PWM_WRAP=4000 # ШИМ 
MAX_VOL_PWM=20
DEFAULT_VOLUME_PWM=50
BEEP=12# вывод звук бипера на пин классика ШИМ на плате MURM2 портит звук Soft AY
#BEEP=1 #вывод звук бипера на отдельный шим
##AUDIO 595  hard AY на PiCard это отсутствует
PIO_AY595=pio0 
SM_AY595=0  
CLK_AY_PIN=29 # CLK_AY_PIN=21
CLK_LATCH_595_BASE_PIN=10 # и 11
DATA_595_PIN=9

##AUDIO i2s
I2S_DATA_PIN=45
I2S_CLK_BASE_PIN=46 # и 47
PIO_I2S=pio0 # использование GPIO 16...48
SM_I2S=0
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=0

# NES JOY
D_JOY_DATA_PIN=26
D_JOY_CLK_PIN=20
D_JOY_LATCH_PIN=21

DELAY_KEY=127 #задержка USB keyboard
DELAY_JOY=200 #задержка NES JOY

)
########### PSRAM для PI CARD ###########################################
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_BUTTER
            )
            message("Конфигурация: PI CARD + PICO_RP2350 (PSRAM_BUTTER)")
######################################################################### 