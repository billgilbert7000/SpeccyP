#    Конфигурация для платы WS_ZERO2
#######################################################
#    WS_ZERO2 Wave Share Zero2  2025.09.16 
#    PIO 1    I2S  PS/2
#    PIO 0 VIDEO пины GPIO 16...48
#######################################################
#    pio 0 используется для управления GPIO 16...48
#######################################################

set (M_VERSION z0)# версия платы

target_compile_definitions(${PROJECT_NAME} PRIVATE
WS_ZERO2
VOLTAGE_RUN=VREG_VOLTAGE_1_50###1_30
VOLTAGE_WORK=VREG_VOLTAGE_1_50

HDMI_ONLY # только HDMI
##HDMI_DIV=1.0 #HDMI 60 Hz 378
##HDMI_DIV=1.0 #HDMI 60 Hz 315   HDMI 90 Hz 378 
RP2350B

##SOUND_PWM_ONLY
##SOUND_I2S_ONLY # только i2s звук
PICO_PIO_USE_GPIO_BASE # для использования GPIO >31 раскомментировать если RP2350B 

LED_BOARD=255  # 255 светодиод не подключен

#PS/2 клавиатура
$<IF:$<BOOL:${PCM5122}>,beginPS2_PIN=14,beginPS2_PIN=2>
PIO_PS2=pio1
SM_PS2=3 #

#VIDEO
HDMI_PIN_ORDER_REVERSE

beginVideo_PIN=32

beginHDMI_PIN_data=32
beginHDMI_PIN_clk=38

PIO_VIDEO=pio0
PIO_VIDEO_ADDR=pio0
SM_VIDEO=0
SM_CONV=1
#
#TFT не используется
PIO_SPI_TFT=pio0
SM_SPI_TFT=0
PIO_SPI_TFT_CONV=pio0
SM_SPI_TFT_CONV=1

TFT_CLK_PIN=13
TFT_DATA_PIN=12
TFT_RST_PIN=8
TFT_DC_PIN=10
TFT_CS_PIN=6
TFT_LED_PIN=9


##SDCARD
SDCARD_SPI_BUS=spi1
SDCARD_PIN_CS=43
SDCARD_PIN_SCK=30
SDCARD_PIN_MOSI=31
SDCARD_PIN_MISO=40

#SDCARD_PIO=pio0 #устанавливается при нестандартных gpio
#SDCARD_PIO_SM=0 #устанавливается при нестандартных gpio


##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PSRAM_BUTTER_PIN_CS=47 #это PSRAM_PIN_CS для  psram бутерброд

##AUDIO PWM
ZX_AY_PWM_R=10
ZX_AY_PWM_L=11
ZX_BEEP_PIN=12
BEEP_PWM_WRAP=1000 # ШИМ
AY_PWM_WRAP=4000 # ШИМ
MAX_VOL_PWM=20
DEFAULT_VOLUME_PWM=50
BEEP=12# вывод звук бипера на пин классика ШИМ на плате MURM2 портит звук Soft AY
#BEEP=1 #вывод звук бипера на отдельный шим
##AUDIO 595  hard AY
PIO_AY595=pio1
SM_AY595=3 # не используется
CLK_AY_PIN=0 # не используется
CLK_LATCH_595_BASE_PIN=0# и 27 # не используется
DATA_595_PIN=0 # не используется

##AUDIO i2s
$<IF:$<BOOL:${PCM5122}>,I2S_DATA_PIN=21,I2S_DATA_PIN=10>
$<IF:$<BOOL:${PCM5122}>,I2S_CLK_BASE_PIN=18,I2S_CLK_BASE_PIN=11>
I2S_FREQ=111111 ##111111 Hz
PIO_I2S=pio1 #
SM_I2S=0

DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=0

## PCM5122 DAC (I2C control) — только при -DPCM5122=ON
$<$<BOOL:${PCM5122}>:PCM5122_DAC>
$<$<BOOL:${PCM5122}>:PCM5122_I2C_SDA=2>
$<$<BOOL:${PCM5122}>:PCM5122_I2C_SCL=3>

PIN_ZX_LOAD=17


# NES JOY
D_JOY_DATA_PIN=7
D_JOY_CLK_PIN=4
D_JOY_LATCH_PIN=5

DELAY_JOY=200 #задержка NES JOY
DELAY_KEY=127 #задержка USB keyboard

)
########### PSRAM для WS ZERO 2 #########################################
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_BUTTER
            )
            message("Конфигурация: WS_ZERO2 + PICO_RP2350B (PSRAM_BUTTER)")
#########################################################################     