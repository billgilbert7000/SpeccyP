#######################################################
#    PICO_PC Olimex RP2040-PICO-PC / RP2350  2026/04/22
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
set (M_VERSION pc)# версия платы

target_compile_definitions(${PROJECT_NAME} PRIVATE
PICO_PC
FW_VERSION="${PORT_VERSION} [pico pc]"
NO_PSRAM

SOUND_PWM_ONLY # только PWM звук (R=27, L=28, BEEP=26)
LED_BOARD=25
VOLTAGE_RUN=VREG_VOLTAGE_1_40
VOLTAGE_WORK=VREG_VOLTAGE_1_40
#########################################################################
##PicoBus ONLY GENERAL SOUND — на PICO_PC не используется (конфликт пинов с PWM audio)
PBUS_OUT_PIN=29
PBUS_IN_PIN=29
PBUS_PIO=pio0   #only pio 0
## Beep
BEEP_PIN=26

#PS/2 клавиатура
beginPS2_PIN=0 # 0 и 1 
PIO_PS2=pio0 # Only  pio0 !
SM_PS2=2 #  

#VIDEO
HDMI_ONLY # только HDMI
HDMI_PIN_SWAP_RG # Olimex PICO-PC: TMDS D1(G)=GPIO18-19, D2(R)=GPIO16-17
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

TFT_CLK_PIN=13
TFT_DATA_PIN=12
TFT_RST_PIN=8
TFT_DC_PIN=10
TFT_CS_PIN=6
TFT_LED_PIN=9

##SDCARD
SDCARD_SPI_BUS=spi0
SDCARD_PIN_CS=22
SDCARD_PIN_SCK=6
SDCARD_PIN_MOSI=7
SDCARD_PIN_MISO=4

SDCARD_PIO=pio0 # PIO SPI для SD (pio1 занят видео+I2S)
SDCARD_PIO_SM=0 # SM2=PS/2 (инициализируется позже)

##PSRAM
#PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PSRAM_BUTTER_PIN_CS=47 #PSRAM butter CS для PICO_PC

##AUDIO PWM  Olimex RP2040-PICO-PC: R=GPIO27, L=GPIO28, BEEP=GPIO26
ZX_AY_PWM_R=27
ZX_AY_PWM_L=28
ZX_BEEP_PIN=26
BEEP_PWM_WRAP=1000 # ШИМ
AY_PWM_WRAP=4000 # ШИМ
MAX_VOL_PWM=100
DEFAULT_VOLUME_PWM=50
BEEP=2 # бипер микшируется в PWM R/L (на Olimex PICO-PC отдельного пина бипера нет)
##AUDIO 595  hard AY — не используется
PIO_AY595=pio1
SM_AY595=0
CLK_AY_PIN=29 # не используется
CLK_LATCH_595_BASE_PIN=26 # не используется
DATA_595_PIN=28 # не используется
##AUDIO i2s — не используется
I2S_DATA_PIN=255
I2S_CLK_BASE_PIN=255
I2S_FREQ=111111
PIO_I2S=pio1
SM_I2S=2
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=0

PIN_ZX_LOAD=5

# NES JOY — пины 8,9 заняты HDMI, джойстик отключен
D_JOY_DATA_PIN=255
D_JOY_CLK_PIN=255
D_JOY_LATCH_PIN=255

DELAY_JOY=200 #задержка NES JOY
DELAY_KEY=127 #задержка USB keyboard

)
            message("Конфигурация: PICO_PC PICO_RP2350 (NO PSRAM)")

