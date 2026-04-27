cmake_minimum_required(VERSION 3.13)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Устанавливаем параметры платы и чипа и так далее
set(PICO_BOARD pico2 CACHE STRING "Board type")

set(DEBUG OFF)

# == Конфигурация  SpeccyP ==
set (BASE_VERSION "1.6.0")
set(MOS2 OFF) # компиляция под MOS2 ON

# Конфигурация сборки (можно задать через -DSPECCY_CONFIG=... и -DM_BOARD=...)
# SPECCY_CONFIG: SPECCY | SPECCY_WSZERO2 | GSP_FULL | GSP_LITE
# M_BOARD:       MURM1 | MURM2 | PI_CARD | WS_ZERO2 | PICO_DV | PICO_PC
set(SPECCY_CONFIG "SPECCY" CACHE STRING "Конфигурация сборки: SPECCY, SPECCY_WSZERO2, GSP_FULL, GSP_LITE")
set_property(CACHE SPECCY_CONFIG PROPERTY STRINGS SPECCY SPECCY_WSZERO2 GSP_FULL GSP_LITE)

set(M_BOARD "MURM1" CACHE STRING "Тип платы: MURM1, MURM2, PI_CARD, WS_ZERO2, PICO_DV, PICO_PC")
set_property(CACHE M_BOARD PROPERTY STRINGS MURM1 MURM2 PI_CARD WS_ZERO2 PICO_DV PICO_PC)

option(PCM5122 "PCM5122 DAC для WS_ZERO2 (I2S 21/18/19 + I2C 2/3, PS/2 на 14/15)" OFF)
#---------------------
###


###
# Определяем опции в зависимости от конфигурации 
if(SPECCY_CONFIG STREQUAL "GSP_FULL")
    set(SPECCY_CONFIG_DEFINITIONS 
        
        CPU_MHZ=378 # 378 MHz
        FLASH_MAX_FREQ_MHZ=166
        PSRAM_MAX_FREQ_MHZ=166
        HDMI_DIV=1.5 # hdmi 60Hz

        TEST_DEBUG #

        GENERAL_SOUND 
        TURBOSOUND
        Z_CONTROLER
        MIDI
    )
      set(GENERAL_SOUND ON)
    
elseif(SPECCY_CONFIG STREQUAL "GSP_LITE")
    set(SPECCY_CONFIG_DEFINITIONS  

        CPU_MHZ=378 # 378 MHz
        FLASH_MAX_FREQ_MHZ=166
        PSRAM_MAX_FREQ_MHZ=166
        HDMI_DIV=1.5 # hdmi 60Hz

        GENERAL_SOUND 
        TURBOSOUND

    )
      set(GENERAL_SOUND ON)

elseif(SPECCY_CONFIG STREQUAL "SPECCY_WSZERO2")
    set(SPECCY_CONFIG_DEFINITIONS  

      #  CPU_MHZ=252 # 378 MHz
        CPU_MHZ=378 # 
        FLASH_MAX_FREQ_MHZ=66
        PSRAM_MAX_FREQ_MHZ=100
        HDMI_DIV=1.5 # hdmi 60Hz
       # HDMI_DIV=1.0 # hdmi 75Hz 378  60Hz 252
        TEST_DEBUG #

    )
      set(GENERAL_SOUND OFF)


else()
    # По умолчанию используем SPECCY
    set(SPECCY_CONFIG "SPECCY")
    set(SPECCY_CONFIG_DEFINITIONS 

        CPU_MHZ=378# 378 MHz
        FLASH_MAX_FREQ_MHZ=166
        PSRAM_MAX_FREQ_MHZ=166

        TEST_DEBUG #
 
        HDMI_DIV=1.5 # hdmi 60Hz

    )
     set(GENERAL_SOUND OFF) 
endif()

###
if(MOS2)
    set(PORT_VERSION "${BASE_VERSION}SE")
elseif(GENERAL_SOUND)
    set(PORT_VERSION "${BASE_VERSION}GS")
 
else()
    set(PORT_VERSION "${BASE_VERSION}") ##set(PORT_VERSION "${BASE_VERSION}LE")
endif()


# Автовыбор PICO_BOARD по типу платы
if(M_BOARD STREQUAL "WS_ZERO2" OR M_BOARD STREQUAL "PI_CARD" OR M_BOARD STREQUAL "PICO_DV" OR M_BOARD STREQUAL "PICO_PC")
    set(PICO_BOARD "pico2" CACHE STRING "Board type" FORCE)
endif()

# Определяем платформу на основе выбранной платы
if(PICO_BOARD STREQUAL "pico")
    set(PICO_PLATFORM "rp2040" CACHE STRING "Chip type" FORCE)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
    endif()
elseif(PICO_BOARD STREQUAL "pico2")
    set(PICO_PLATFORM "rp2350" CACHE STRING "Chip type" FORCE)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
else()
    message(FATAL_ERROR "Unsupported PICO_BOARD: ${PICO_BOARD}")
endif()

include(pico_sdk_import.cmake)
## инициализация проекта
project(SpeccyP C CXX ASM)

string(TIMESTAMP BUILD_DATE "%d.%m.%y")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}  -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections  ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections  ")
#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}  -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections  -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections  -O3")

##set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffast-math  -Os")
#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffast-math  -O3")
 ##set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops -ffast-math -feliminate-unused-debug-types -ffunction-sections -fdata-sections -Os")
#-O2 оптимизация
#оптимизации циклов -funroll-loops
#-устранение неиспользуемых-отладочных-типов -feliminate-unused-debug-types
# помещает каждую функцию в отдельную секцию -ffunction-sections 
# -fdata-sections помещает каждую функцию или элемент данных в отдельную секцию в выходном файле
#-Wall  Включает предупреждения
#-fito удаение неиспользуемых функций
#set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

### инициализация Pico SDK
pico_sdk_init()
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build")


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

add_subdirectory(drivers/audio_i2s)
add_subdirectory(drivers/pico_fatfs)

# Только для MURM1 добавляем psram
if(${M_BOARD} MATCHES "MURM1")
    add_subdirectory(drivers/psram)
endif()

# PCM5122 DAC для WS_ZERO2
if(${M_BOARD} MATCHES "WS_ZERO2" AND PCM5122)
    add_subdirectory(drivers/pcm5122)
endif()



file(GLOB_RECURSE SRC 
 "src/*.c"
 "src/*.h"
 "src/zx_emu/*.c" "src/zx_emu/*.h" 
 "src/zx_emu/rom/*.h" 
 "src/zx_emu/z80/*.h" 
"src/picobus/*.c"
)
message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})



if( ${PICO_PLATFORM} MATCHES "rp2040" )
	pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
	target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
	pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)

	endif()



set (FATFS_SPI spi0)

pico_set_program_name(SpeccyP "ZX Spectrum emulator")
pico_set_program_version(SpeccyP ${PORT_VERSION})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
###################################################
#    ГЛОБАЛЬНЫЕ НАСТРОЙКИ ЖЕЛЕЗА
###################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
## Номер версии конфигурационного файла
CONFIG_VERSION=270426
CPU_KHZ=CPU_MHZ*1000
PICOBUS_SPEED=20   #// 160000000 = 20MB/s // 80000000 = 10MB/s
POWER_MODE_WEACT=1

##DEBUG_MSG
##LEDBLINK           #define LEDBLINK
###################################################
    # Дата и время компиляции
    BUILD_DATE="${BUILD_DATE}"
    BUILD_TIME="${BUILD_TIME}"

$<IF:$<BOOL:${MOS2}>,MOS2,NO_MOS2>


#NO_GMX 
KEY_UPDATE_MODE


DEVICE_DEFAULT # стандартная конфигурация
#DEVICE_HDMI90_I2S # Монитор HDMI 90Hz i2s
# DEVICE_PI_CARD # Монитор HDMI 60Hz i2s

# Конфигурация в зависимости от GSP_CONFIG
${SPECCY_CONFIG_DEFINITIONS}


# частота cloc'a 595  (в герцах)
FREQ_595=50000000  #//750000000  // Например: 75 МГц 

############################################################
###   для эмулятора Z80 REDCODE
Z80_STATIC
Z80_EXTERNAL_HEADER="Z80_compat.h"
##Z80_WITH_EXECUTE
##Z80_WITH_Q
Z80_WITH_PRECOMPUTED_DAA
##Z80_WITH_ZILOG_NMOS_LD_A_IR_BUG
#Z80_HALT_CANCEL
#Z80_MODEL_ZILOG_NMOS
##Z80_MODEL_ZILOG_CMOS

############################################################

${PLATFORM_DEFINITIONS}
)
#######################################################
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
if( ${M_BOARD} MATCHES "MURM1")
set (M_VERSION m1)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE

MURM1
LED_BOARD=25

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
endif()
#######################################################
#    MURM2
#    PIO 0 I2S or HARD_AY_595 SM 0 SM FREE 1,2,3
#    PIO 1 PS/2 (2) and VIDEO  TFT (0 1) PSRAM_BOARD(SM=3) - нет
#######################################################
if( ${M_BOARD} MATCHES "MURM2")
set (M_VERSION m2)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
MURM2
LED_BOARD=25

##PicoBus ONLY GENERAL SOUND
PBUS_OUT_PIN=10
PBUS_IN_PIN=11
PBUS_PIO=pio0   #only pio 0
## Beep
BEEP_PIN=9  # на IN

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

##AUDIO PWM
ZX_AY_PWM_R=10
ZX_AY_PWM_L=11
ZX_BEEP_PIN=9 
BEEP_PWM_WRAP=1000 # ШИМ 
AY_PWM_WRAP=4000 # ШИМ 
MAX_VOL_PWM=100
DEFAULT_VOLUME_PWM=50
BEEP=12 # бипер микшируется в PWM R/L 
#BEEP=0# вывод звук бипера на пин классика ШИМ на плате MURM2 портит звук Soft AY
#BEEP=1 #вывод звук бипера на отдельный шим
##AUDIO 595  hard AY 
PIO_AY595=pio0 
SM_AY595=0  
CLK_AY_PIN=29 # CLK_AY_PIN=21
CLK_LATCH_595_BASE_PIN=10 # и 11
DATA_595_PIN=9

##AUDIO i2s
I2S_DATA_PIN=9
I2S_CLK_BASE_PIN=10
I2S_FREQ=111111 ##111111 #48000
PIO_I2S=pio0
SM_I2S=0
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
endif()

#######################################################
#    PI_CARD
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
#    pio 0 используется для управления GPIO 16...48
#######################################################
if( ${M_BOARD} MATCHES "PI_CARD")
set (M_VERSION pc)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
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
I2S_FREQ=111111 ##111111 #48000
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
endif()
###############################################

#######################################################
#    WS_ZERO2 Wave Share Zero2  2025.09.16 
#    PIO 1    I2S  PS/2
#    PIO 0 VIDEO пины GPIO 16...48
#######################################################
#    pio 0 используется для управления GPIO 16...48
#######################################################
if( ${M_BOARD} MATCHES "WS_ZERO2")
set (M_VERSION ws)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
WS_ZERO2
HDMI_ONLY # только HDMI
##HDMI_DIV=1.0 #HDMI 60 Hz 378
##HDMI_DIV=1.0 #HDMI 60 Hz 315   HDMI 90 Hz 378 
RP2350B

##SOUND_PWM_ONLY
##SOUND_I2S_ONLY # только i2s звук
PICO_PIO_USE_GPIO_BASE # для использования GPIO >31 раскомментировать если RP2350B 

LED_BOARD=255  # 255 светодиод не подключен

#########################################################################
##PicoBus ONLY GENERAL SOUND
PBUS_OUT_PIN=10
PBUS_IN_PIN=11
PBUS_PIO=pio0   #only pio 0
#define PICOBUS_BAUDRATE (1000*1000)     // Скорость шины picobus 

## Beep
BEEP_PIN=12  # на IN

#######################################################################
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
endif()
#######################################################
#    PICO_DV Pimoroni RP2350  2025/09/19
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
if( ${M_BOARD} MATCHES "PICO_DV")
set (M_VERSION dv)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
PICO_DV
SOUND_I2S_ONLY # только i2s звук
LED_BOARD=25

#########################################################################
##PicoBus ONLY GENERAL SOUND
PBUS_OUT_PIN=26
PBUS_IN_PIN=27
PBUS_PIO=pio0   #only pio 0
## Beep
BEEP_PIN=28

#PS/2 клавиатура
beginPS2_PIN=0 # 0 и 1 
PIO_PS2=pio0 # Only  pio0 !
SM_PS2=2 #  

#VIDEO
HDMI_ONLY # только HDMI
beginVideo_PIN=6

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

##SDCARD — пины не на spi1, используем PIO SPI
SDCARD_USE_PIO
SDCARD_PIN_CS=22
SDCARD_PIN_SCK=5
SDCARD_PIN_MOSI=18
SDCARD_PIN_MISO=19

SDCARD_PIO=pio0 # PIO SPI для SD (pio1 занят видео+I2S)
SDCARD_PIO_SM=0 # SM2=PS/2 (инициализируется позже)

##PSRAM
PSRAM_DIV=2.0 #  1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
PSRAM_BUTTER_PIN_CS=47 #PSRAM butter CS для PICO_DV

##AUDIO PWM
ZX_AY_PWM_R=26 # не используется
ZX_AY_PWM_L=27 # не используется
ZX_BEEP_PIN=28 # не используется
BEEP_PWM_WRAP=1000 # ШИМ 
AY_PWM_WRAP=4000 # ШИМ 
MAX_VOL_PWM=20
DEFAULT_VOLUME_PWM=50
BEEP=12# вывод звук бипера на пин классика ШИМ на плате MURM2 портит звук Soft AY
#BEEP=1 #вывод звук бипера на отдельный шим
##AUDIO 595  hard AY 
PIO_AY595=pio1 
SM_AY595=0 
CLK_AY_PIN=29 # CLK_AY_PIN=21 # не используется
CLK_LATCH_595_BASE_PIN=26 # и 27 # не используется
DATA_595_PIN=28 # не используется
##AUDIO i2s
I2S_DATA_PIN=26
I2S_CLK_BASE_PIN=27
I2S_FREQ=111111 ##111111 Hz
PIO_I2S=pio1 # SM0,SM1 заняты видео
SM_I2S=2
DEFAULT_VOLUME_I2S=50
AUDIO_BUSTER_DEFAULT=0

PIN_ZX_LOAD=20

# NES JOY — пины 8,9 заняты HDMI, джойстик отключен
D_JOY_DATA_PIN=255
D_JOY_CLK_PIN=255
D_JOY_LATCH_PIN=255

DELAY_JOY=200 #задержка NES JOY
DELAY_KEY=127 #задержка USB keyboard

)
endif()
#######################################################
#    PICO_PC Olimex RP2040-PICO-PC / RP2350  2026/04/22
#    PIO 0 PS/2  and I2S or HARD_AY_595
#    PIO 1 VIDEO TFT PSRAM_BOARD(SM=2)
#######################################################
if( ${M_BOARD} MATCHES "PICO_PC")
set (M_VERSION pc)# версия платы
target_compile_definitions(${PROJECT_NAME} PRIVATE
PICO_PC
SOUND_PWM_ONLY # только PWM звук (R=27, L=28, BEEP=26)
LED_BOARD=25

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
endif()
###############################################
if( ${PICO_PLATFORM} MATCHES "rp2040")
set (P_VERSION p1)# версия чипа 
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_RP2040
        ## AY-3-8910 
        AY_DELTA=2
        AY_SAMPLE_RATE=-9 #-9
        TRANSFER_COUNT=1<<31
)
endif()
#-----------------------------------------
if( ${PICO_PLATFORM} MATCHES "rp2350")
set (P_VERSION p2)# версия чипа 
target_compile_definitions(${PROJECT_NAME} PRIVATE
      ##  PICO_PIO_USE_GPIO_BASE # для использования GPIO >31 раскомментировать если RP2350B 
        PICO_RP2350
        RP2350_256K   ##для памяти RAM 256kB на RP2350 не используя PSRAM
        ## AY-3-8910 
        AY_DELTA=2
        AY_SAMPLE_RATE=-9#(-1000000 / (4* 48000) ) ##//-9// 
        TRANSFER_COUNT=1<<24
)
endif()
#-----------------------------------------

#---------------------------------------------------------------
# Конфигурация дефайнов для использования PSRAM
#---------------------------------------------------------------

    # Обработка конфигураций
    if( ${M_BOARD} MATCHES "MURM1")
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
    
    elseif( ${M_BOARD} MATCHES "MURM2")
            if( ${PICO_PLATFORM} MATCHES "rp2040")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_NOSUPORT
            )
            message("Конфигурация: MURM2 + PICO_RP2040 (NO_PSRAM)")
        
        elseif( ${PICO_PLATFORM} MATCHES "rp2350")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
           PSRAM_BUTTER
           ## NO_PSRAM
            )
            message("Конфигурация: MURM2 + PICO_RP2350 (PSRAM_BUTTER)")
        endif()
    endif()
########### PSRAM для PI CARD ###########################################
   if( ${M_BOARD} MATCHES "PI_CARD")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_BUTTER
            )
            message("Конфигурация: PI CARD + PICO_RP2350 (PSRAM_BUTTER)")
        endif()
#########################################################################      
########### PSRAM для WS ZERO ###########################################
   if( ${M_BOARD} MATCHES "WS_ZERO2")
            target_compile_definitions(${PROJECT_NAME} PRIVATE 
            PSRAM_BUTTER
            )
            message("Конфигурация: WS_ZERO2 + PICO_RP2350B (PSRAM_BUTTER)")
        endif()
#########################################################################      
########### PSRAM для PICO_DV ###########################################
   if( ${M_BOARD} MATCHES "PICO_DV")
            target_compile_definitions(${PROJECT_NAME} PRIVATE
            PSRAM_BUTTER
            )
            message("Конфигурация: PICO_DV+ PICO_RP2350 (PSRAM_BUTTER)")
        endif()
#########################################################################      
########### PSRAM для PICO_PC ###########################################
   if( ${M_BOARD} MATCHES "PICO_PC")
            target_compile_definitions(${PROJECT_NAME} PRIVATE
              NO_PSRAM
            )
            message("Конфигурация: PICO_PC PICO_RP2350 (NO PSRAM)")
        endif()
#########################################################################      

#----------------------------------------------
# Формирование FW_VERSION:

if( ${M_BOARD} MATCHES "PICO_DV")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [pico dv]"
)
endif()


if( ${M_BOARD} MATCHES "WS_ZERO2")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [z0p2]"
)
endif()

if( ${M_BOARD} MATCHES "PI_CARD")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [picard]"
)
endif()

if( ${M_BOARD} MATCHES "MURM1")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [${M_VERSION}${P_VERSION}]"
)
endif()

if( ${M_BOARD} MATCHES "MURM2")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [${M_VERSION}${P_VERSION}]"
)
endif()

if( ${M_BOARD} MATCHES "PICO_PC")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [pico pc]"
)
endif()

#SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}_${VOUT}")
# Модифицируем BUILD_NAME:
 
if(PCM5122)
    SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}_pcm5122")
else()
    SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}")
endif()

if(DEBUG)
    SET(BUILD_NAME "${PROJECT_NAME}")
    
    # 
#pico_enable_stdio_usb(${PROJECT_NAME} 0)
#pico_enable_stdio_uart(${PROJECT_NAME} 1)


endif()


set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")


target_link_libraries(${PROJECT_NAME} PRIVATE
 
        pico_runtime
        pico_stdlib
        pico_multicore
        hardware_dma
        hardware_pio
        hardware_i2c
        hardware_spi
        hardware_interp
        hardware_timer
        hardware_clocks
        hardware_pwm
        hardware_flash
       tinyusb_host 
       tinyusb_board
       audio_i2s
       pico_fatfs
)

# Условная линковка библиотек
if(${M_BOARD} MATCHES "MURM1")
    # Для MURM1 добавляем psram
    target_link_libraries(${PROJECT_NAME} PRIVATE
        psram
    )
endif()

# PCM5122 DAC для WS_ZERO2
if(${M_BOARD} MATCHES "WS_ZERO2" AND PCM5122)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pcm5122
    )
endif()




if(MOS2)
    pico_set_linker_script(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/memmap.ld")
endif()

target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage --data-sections)

pico_add_extra_outputs(${PROJECT_NAME})

