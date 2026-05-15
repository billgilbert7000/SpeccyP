/*
 * SpeccyP -- заглушки USB HID API для режима USB CDC консоли (USB_SERIAL).
 *
 * Когда сборка идёт с -DUSB_SERIAL=ON, мы НЕ линкуем tinyusb_host
 * (используется только встроенный CDC-ACM из pico_stdio_usb), поэтому
 * исходники src/usb_key.c и src/xinput_host.c исключаются из сборки —
 * они тащат за собой tusb.h и весь USB Host стек.
 *
 * Эмулятор всё равно зовёт init_usb_hid() / decode_key() / decode_key_joy() /
 * wait_enter() / wait_esc() и читает kb_st_ps2 + flag_usb_kb +
 * usb_device + joy_k. Здесь приведены минимально-необходимые заглушки,
 * чтобы ZX-эмулятор просто думал, что USB-устройств нет, а ввод идёт
 * исключительно с PS/2 / NES Joy / клавиш на плате.
 */

#include "usb_key.h"
#include "ps2.h"
#include "joy.h"

#include <stdint.h>
#include <stdbool.h>

/* Глобалы joy_k, usb_device, flag_usb_kb, gamepad_addr уже определены
 * в src/config.c — здесь только используем их (extern не нужен,
 * мы их не модифицируем напрямую). */

bool init_usb_hid(void)
{
    /* В CDC-режиме нет USB-хоста — возвращаем "нет устройств". */
    return false;
}

bool decode_key(bool menu_mode)
{
    (void)menu_mode;
    /* USB-клавиатуры нет — пусть ввод идёт через PS/2 / клавиатуру платы. */
    return false;
}

bool decode_key_joy(void)
{
    /* Опрашиваем то, что доступно: PS/2 и NES-джой. */
    extern bool decode_PS2(void);
    extern bool decode_joy_to_keyboard(void);
    decode_joy_to_keyboard();
    decode_PS2();
    return true;
}

void wait_enter(void)
{
    /* Без USB-клавиатуры просто опрашиваем PS/2 в цикле. */
    while (1) {
        decode_key_joy();
        if (!(kb_st_ps2.u[1] & KB_U1_ENTER)) return;
    }
}

void wait_esc(void)
{
    while (1) {
        decode_key_joy();
        if (!(kb_st_ps2.u[1] & KB_U1_ESC)) return;
    }
}
