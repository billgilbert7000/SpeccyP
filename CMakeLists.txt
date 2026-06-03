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
# ======================================================================================
# Устанавливаем параметры платы и чипа и так далее
set(PICO_BOARD pico2 CACHE STRING "Board type")

set(DEBUG ON)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 6)
set(VERSION_PATCH 6)
# Базовая версия (оставлено для совместимости)
set(BASE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
## Номер версии конфигурационного файла
math(EXPR CONFIG_VERSION_VALUE "2000 + ${VERSION_MAJOR}*100 + ${VERSION_MINOR}*10 + ${VERSION_PATCH}")

# == Конфигурация SpeccyP ==

# Список допустимых конфигураций (единое место для изменений)
set(SPECCY_CONFIG_VALUES SPECCY SPECCY_WSZERO GSP_FULL GSP_LITE)

# Конфигурация сборки (можно задать через -DSPECCY_CONFIG=...)
set(SPECCY_CONFIG "SPECCY" CACHE STRING "Конфигурация сборки: ${SPECCY_CONFIG_VALUES}")
set_property(CACHE SPECCY_CONFIG PROPERTY STRINGS ${SPECCY_CONFIG_VALUES})

# Проверка валидности введённого значения
if(NOT SPECCY_CONFIG IN_LIST SPECCY_CONFIG_VALUES)
    message(FATAL_ERROR "Недопустимое значение SPECCY_CONFIG='${SPECCY_CONFIG}'. "
                        "Допустимые значения: ${SPECCY_CONFIG_VALUES}")
endif()

# Список плат
set(M_BOARD_VALUES MURM1 MURM2 PI_CARD WS_ZERO1 WS_ZERO2 PICO_DV PICO_PC GSP_M2)
set(M_BOARD "MURM2" CACHE STRING "Тип платы: ${M_BOARD_VALUES}")
set_property(CACHE M_BOARD PROPERTY STRINGS ${M_BOARD_VALUES})

# Проверка валидности M_BOARD
if(NOT M_BOARD IN_LIST M_BOARD_VALUES)
    message(FATAL_ERROR "Недопустимое значение M_BOARD='${M_BOARD}'. "
                        "Допустимые значения: ${M_BOARD_VALUES}")
endif()

option(PCM5122 "PCM5122 DAC для WS_ZERO2 (I2S 21/18/19 + I2C 2/3, PS/2 на 14/15)" OFF)

# HDMI-драйвер на родной HSTX-периферии RP2350 (плюс HDMI-audio через
# Data Islands, если соберётесь его включать). Полностью заменяет
# легаси-драйвер на PIO и путь I2S-аудио.
# Аппаратные ограничения: HSTX занимает GPIO 12-19, поэтому работает
# только на плате M2. clk_sys должен быть целым кратным 126 МГц
# (домен пиксельклока).
option(HDMI_HSTX "Драйвер HDMI на HSTX RP2350 (только M2)" OFF)

# USB CDC-консоль вместо USB HID Host. С ней нельзя пользоваться
# USB-клавиатурой/джойстиком, зато появляется CDC-ACM stdio: можно
# писать printf и читать его через terminal на хосте. На старте
# даётся 5 секунд, чтобы хост успел поднять /dev/cu.usbmodem* и
# терминал подцепился до того, как эмулятор тронется с места.
option(USB_SERIAL "USB CDC console (USB HID Host отключается)" OFF)
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
#
     
elseif(SPECCY_CONFIG STREQUAL "GSP_LITE")
    set(SPECCY_CONFIG_DEFINITIONS  
        CPU_MHZ=378 # 378 MHz
        FLASH_MAX_FREQ_MHZ=166
        PSRAM_MAX_FREQ_MHZ=166
        HDMI_DIV=1.5 # hdmi 60Hz
        TEST_DEBUG #
        GENERAL_SOUND 
        TURBOSOUND
    )
      set(GENERAL_SOUND ON)
###
elseif(SPECCY_CONFIG STREQUAL "SPECCY_WSZERO")
    set(SPECCY_CONFIG_DEFINITIONS  
        CPU_MHZ=378 MHz
        FLASH_MAX_FREQ_MHZ=66
        PSRAM_MAX_FREQ_MHZ=100
        HDMI_DIV=1.5 # hdmi 60Hz*378
        TEST_DEBUG #
    )
      set(GENERAL_SOUND OFF)
###

else()
    # По умолчанию используем SPECCY
    set(SPECCY_CONFIG "SPECCY")
    set(SPECCY_CONFIG_DEFINITIONS 

        CPU_MHZ=378 ## MHz
        FLASH_MAX_FREQ_MHZ=166
        PSRAM_MAX_FREQ_MHZ=166
        TEST_DEBUG #
        HDMI_DIV=1.5 # hdmi 60Hz/378
    )
     set(GENERAL_SOUND OFF) 
endif()

###
if(GENERAL_SOUND)
    set(PORT_VERSION "${BASE_VERSION}GS")
else()
    set(PORT_VERSION "${BASE_VERSION}") 
endif()


# Автовыбор PICO_BOARD по типу платы
set(PICO2_BOARDS WS_ZERO2 PI_CARD PICO_DV PICO_PC)
if(M_BOARD IN_LIST PICO2_BOARDS)
    set(PICO_BOARD "pico2" CACHE STRING "Board type" FORCE)
endif()

# Определяем Build Type на основе выбранного чипа
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

if(HDMI_HSTX AND NOT M_BOARD STREQUAL "MURM2")
    message(FATAL_ERROR
        "HDMI_HSTX требует M_BOARD=MURM2 (HSTX выводится на GPIO 12-19). "
        "У M_BOARD=${M_BOARD} HDMI разведён на других пинах.")
endif()

if(HDMI_HSTX)
    # HSTX хочет целочисленный делитель clk_sys в свой 126 МГц домен
    # пиксельклока. CPU_MHZ выставляется выше через SPECCY_CONFIG_DEFINITIONS;
    # пересчитываем его в MODE_HSTX_CLK_DIV (504/4=126, 252/2=126 и т.д.).
    string(REGEX MATCH "CPU_MHZ=([0-9]+)" _cpu_mhz_match "${SPECCY_CONFIG_DEFINITIONS}")
    set(_cpu_mhz_value "${CMAKE_MATCH_1}")
    if(_cpu_mhz_value EQUAL 504)
        set(HSTX_CLK_DIV 4)
    elseif(_cpu_mhz_value EQUAL 378)
        set(HSTX_CLK_DIV 3)
    elseif(_cpu_mhz_value EQUAL 252)
        set(HSTX_CLK_DIV 2)
    elseif(_cpu_mhz_value EQUAL 126)
        set(HSTX_CLK_DIV 1)
    else()
        message(FATAL_ERROR
            "Для HDMI_HSTX нужен CPU_MHZ, делящийся на 126 нацело "
            "(126/252/378/504); получено ${_cpu_mhz_value}.")
    endif()
    message(STATUS "HDMI_HSTX: clk_sys=${_cpu_mhz_value} MHz, "
            "MODE_HSTX_CLK_DIV=${HSTX_CLK_DIV}")
endif()

# Звук: по умолчанию идёт через драйвер audio_i2s (PIO). При
# HDMI_HSTX мы подменяем i2s_init/i2s_out/i2s_deinit на свои из
# drivers/hdmi_hstx/HDMI_hstx_audio.c — они кладут сэмплы в очередь
# HDMI Data Islands вместо PIO. Сам сабдиректорий audio_i2s
# подключаем всегда: внутри его CMakeLists проверяет HDMI_HSTX и
# при включённом HSTX исключает audio_i2s.c из сборки, чтобы не
# было multiple-definition.
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

file(GLOB_RECURSE SRC_BASE
 "src/*.c"
 "src/*.h"
 "src/zx_emu/*.c" "src/zx_emu/*.h"
 "src/zx_emu/rom/*.h"
 "src/zx_emu/z80/*.h"
)
# Файлы picobus только при GENERAL_SOUND
if(GENERAL_SOUND)
    # Добавляем  файлы picobus 
    file(GLOB_RECURSE SRC_PICOBUS_C "src/picobus/*.c src/picobus/*.h")
    set(SRC ${SRC_BASE} ${SRC_PICOBUS_C})
    message(STATUS "GENERAL_SOUND: ON, добавлены .c файлы picobus")
else()
    set(SRC ${SRC_BASE})
    # При GENERAL_SOUND OFF — исключаем всё из picobus
    list(FILTER SRC EXCLUDE REGEX "/picobus/")
    message(STATUS "GENERAL_SOUND: OFF, файлы picobus исключены")
endif()
###

# usb_serial_stubs.c пускаем в сборку только в режиме USB_SERIAL —
# в обычном режиме он бы дал multiple-definition с usb_key.c.
list(FILTER SRC EXCLUDE REGEX "/src/usb_serial_stubs\\.c$")

# При HDMI_HSTX вместо src/HDMI.c подключаем драйвер из
# drivers/hdmi_hstx. src/HDMI.h оставляем как есть — там
# tab_color[], которая нужна обоим путям.
if(HDMI_HSTX)
    list(FILTER SRC EXCLUDE REGEX "/src/HDMI\\.c$")
    list(APPEND SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hdmi_hstx/HDMI_hstx.c
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hdmi_hstx/HDMI_hstx_audio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hdmi_hstx/video_output.c
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hdmi_hstx/hstx_packet.c
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hdmi_hstx/hstx_data_island_queue.c
    )
endif()

# В режиме USB_SERIAL отключаем USB HID-хост: убираем usb_key.c,
# xinput_host.c (требуют tinyusb_host) и подсовываем заглушки,
# реализующие минимальный API (init_usb_hid, tuh_task, decode_key).
if(USB_SERIAL)
    list(FILTER SRC EXCLUDE REGEX "/src/usb_key\\.c$")
    list(FILTER SRC EXCLUDE REGEX "/src/xinput_host\\.c$")
    list(APPEND SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_serial_stubs.c)
endif()

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})

if(HDMI_HSTX)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/hdmi_hstx
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/audio_i2s
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        HDMI_HSTX=1
        MODE_HSTX_CLK_DIV=${HSTX_CLK_DIV}
        # Полярность дифпар приёмника. PCB M2 разводит CLK- на GPIO12
        # (чётный), CLK+ на GPIO13 (нечётный). Ветка драйвера по
        # умолчанию (без HSTX_INVERT_DIFFPAIRS) укладывает полярность
        # ровно так — оставляем как есть; включение этого флага
        # инвертирует все линии и сломает HDMI.
    )
endif()

if(USB_SERIAL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USB_SERIAL=1)
endif()



if( ${PICO_PLATFORM} MATCHES "rp2040" )
	pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
	target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
	pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)

	endif()

pico_set_program_name(SpeccyP "ZX Spectrum emulator")
pico_set_program_version(SpeccyP ${PORT_VERSION})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

###################################################
#    ГЛОБАЛЬНЫЕ НАСТРОЙКИ ЖЕЛЕЗА
###################################################
set (FATFS_SPI spi0)

target_compile_definitions(${PROJECT_NAME} PRIVATE
## Номер версии конфигурационного файла
CONFIG_VERSION=${CONFIG_VERSION_VALUE}
CPU_KHZ=CPU_MHZ*1000
PICOBUS_SPEED=20   #// 160000000 = 20MB/s // 80000000 = 10MB/s
POWER_MODE_WEACT=1
VOLTAGE_RP2040=VREG_VOLTAGE_1_30
##DEBUG_MSG
##LEDBLINK           #define LEDBLINK
###################################################
    # Дата и время компиляции
    BUILD_DATE="${BUILD_DATE}"
    BUILD_TIME="${BUILD_TIME}"

#NO_GMX 
KEY_UPDATE_MODE
DEVICE_DEFAULT # стандартная конфигурация
#DEVICE_HDMI90_I2S # Монитор HDMI 90Hz i2s
# DEVICE_PI_CARD # Монитор HDMI 60Hz i2s

# Конфигурация в зависимости от GSP_CONFIG
${SPECCY_CONFIG_DEFINITIONS}

# частота cloc'a 595  (в герцах)
FREQ_595=50000000  #//750000000  // Например: 75 МГц 

#################################
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
##################################
)

# === подключаем файл конфигурации для выбранной платы ===
include(boards/${M_BOARD}.cmake)

######################################################
if( ${PICO_PLATFORM} MATCHES "rp2040")
set (P_VERSION p1)# версия чипа 
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_RP2040
        ## AY-3-8910 
        AY_DELTA=2
        # -9  111111 Гц  ## -21 μs = 47619 Гц	-0,79% ??? 
        AY_SAMPLE_RATE=-9 ##Чем чаше вызывать тем качество эмуляции AY лучше
        I2S_FREQ=48000 ##111111 #48000 
        TRANSFER_COUNT=1<<31
)
endif()
#-----------------------------------------
if( ${PICO_PLATFORM} MATCHES "rp2350")
set (P_VERSION p2)# версия чипа 
target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_RP2350
        RP2350_256K   ##для памяти RAM 256kB на RP2350 не используя PSRAM
        ## AY-3-8910 
        AY_DELTA=2
        # -9  111111 Гц  ## -21 μs = 47619 Гц	-0,79% ??? 
        # для RP2350 можно и -5 выставить
        AY_SAMPLE_RATE=-9 ##Чем чаше вызывать тем качество эмуляции AY лучше 
        I2S_FREQ=48000 ##111111 #48000 
        TRANSFER_COUNT=1<<24
)
endif()
#----------------------------------------------
# Формирование FW_VERSION:

if( ${M_BOARD} MATCHES "PICO_DV")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [pico dv]"
)
endif()

if( ${M_BOARD} MATCHES "WS_ZERO1")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [z0p1]"
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

if( ${M_BOARD} MATCHES "GSP_M2")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FW_VERSION="${PORT_VERSION} [${M_VERSION}${P_VERSION}]"
)
endif()
##########################################################################
# Формирование BUILD_NAME
if(DEBUG)
    SET(BUILD_NAME "${PROJECT_NAME}")
elseif(HDMI_HSTX)    
    SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}_hdmi_audio")  
elseif(PCM5122)
    SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}_pcm5122")
else()
    SET(BUILD_NAME "${PROJECT_NAME}_${PORT_VERSION}_${M_VERSION}${P_VERSION}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

# Условная линковка библиотек
target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_runtime
        pico_stdlib
        pico_multicore
        hardware_dma
        hardware_pio
     #   hardware_i2c
        hardware_spi
        hardware_interp
        hardware_timer
        hardware_clocks
        hardware_pwm
        hardware_flash
       tinyusb_host 
       tinyusb_board
      # audio_i2s
       pico_fatfs
)

# Условная линковка библиотек
if(GENERAL_SOUND)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE
        audio_i2s
)
endif()

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

# USB-стек: по умолчанию TinyUSB Host (для USB-клавиатуры/джойстика);
# при USB_SERIAL=ON — CDC-консоль из pico_stdio_usb. В CDC-режиме
# TinyUSB Host не линкуется вообще, чтобы он не конфликтовал с CDC
# device-стеком Pico SDK на том же root-порту.
if(USB_SERIAL)
    pico_enable_stdio_usb(${PROJECT_NAME} 1)
    pico_enable_stdio_uart(${PROJECT_NAME} 0)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        # Не блокируемся в ожидании коннекта терминала — иначе при
        # отсутствии хоста мы вообще не выходим из stdio_init_all().
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=0
        # Отключаем vendor reset interface (USB_RESET_INTERFACE),
        # чтобы устройство было pure CDC (bDeviceClass=02), иначе
        # macOS Sonoma+ не подцепляет AppleUSBCDCACMData driver
        # и /dev/cu.usbmodem* не появляется.
        PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0
        PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK=0
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        tinyusb_host
        tinyusb_board
    )
endif()


target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage --data-sections)

pico_add_extra_outputs(${PROJECT_NAME})

