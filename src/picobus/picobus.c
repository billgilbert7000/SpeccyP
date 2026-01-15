/*
 * Pico PIO Connect, a utility to connect Raspberry Pi Picos together
 * Copyright (C) 2023 Andrew Menadue
 * Copyright (C) 2023 Derek Fountain
 * 
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "picobus.h"
#include "picobus.pio.h"

/* #define PBUS_OUT_PIN 26
#define PBUS_IN_PIN 27
#define PBUS_PIO pio0 */

// Глобальный контекст (static для инкапсуляции)
static picobus_t picobus;

void picobus_link_init(void)
{
  gpio_init(PBUS_OUT_PIN); gpio_set_dir(PBUS_OUT_PIN,GPIO_OUT); gpio_put(PBUS_OUT_PIN, 1);
  gpio_set_function(PBUS_OUT_PIN, GPIO_FUNC_PIO0);

  gpio_init(PBUS_IN_PIN); gpio_set_dir(PBUS_IN_PIN,GPIO_IN);
  gpio_set_function(PBUS_IN_PIN, GPIO_FUNC_PIO0);

  /* Outgoing side of the link */
  int linkout_sm  = pio_claim_unused_sm(PBUS_PIO, true);
  uint offset     = pio_add_program(PBUS_PIO, &picoputerlinkout_program);
  picoputerlinkout_program_init(PBUS_PIO, linkout_sm, offset, PBUS_OUT_PIN);

  /* Incoming side of the link */
  int linkin_sm   = pio_claim_unused_sm(PBUS_PIO, true);
  offset          = pio_add_program(PBUS_PIO, &picoputerlinkin_program);
  picoputerlinkin_program_init(PBUS_PIO, linkin_sm, offset, PBUS_IN_PIN);
 

    picobus.pio = PBUS_PIO;
    picobus.linkin_sm = linkin_sm;
    picobus.linkout_sm = linkout_sm;

}
//-------------------------------------------------------
/*
 * Receive a byte from the link. The received value goes into the given location,
 * and the routine returns one of the status values indicating no data received,
 * an acknowledgement received, or data received.
 *
 * The function name is a bit of a misnomer; if you're expecting an ACK then you
 * don't actually receive a byte. In this case you should pass received_value as
 * NULL.
 */
static link_received_t __not_in_flash_func(receive_byte)(uint8_t *received_value)
{
    uint32_t data;
    if (picoputerlinkin_get(picobus.pio, picobus.linkin_sm, &data) == false) {
        return LINK_BYTE_NONE;
    }
/* Invert what's been received */
    data = data ^ 0xFFFFFFFF;
/* It arrives from the PIO at the top end of the word, so shift down */
    data >>= 22;
/* Magic value indicates a ACK on the wire */
    if (data == 0x100) {
        return LINK_BYTE_ACK;
    } else {
        /* Remove stop bit in LSB */
        data >>= 1;
        /* Mask out data, just to be sure */
        data &= 0xff;

        if (received_value != NULL)
            *received_value = (uint8_t)data;

        return LINK_BYTE_DATA;
    }
}
/*
 * Receive a byte and ACK it back to the sender
 */
link_received_t __not_in_flash_func(receive_acked_byte)(uint8_t *received_value)
{
    if (receive_byte(received_value) == LINK_BYTE_NONE)
        return LINK_BYTE_NONE;

    send_ack_to_link();
    return LINK_BYTE_DATA;
}

/*
 * Receive a number of bytes into the given buffer. Bytes are acknowledged.
 */

void __not_in_flash_func(receive_buffer)(uint8_t *data, uint32_t count)
{
    while (count) {
        while (receive_acked_byte(data) == LINK_BYTE_NONE);
        data++;
        count--;
    }
} 


/*
 * Send an ACK.
 */
void __not_in_flash_func(send_ack_to_link)(void)
{
    pio_sm_put_blocking(picobus.pio, picobus.linkout_sm, (uint32_t)0x2ff);
}

/*
 * Send a byte and wait for the receiver to acknowledge it.
 */

void __not_in_flash_func(send_byte)(uint8_t data)
{
    pio_sm_put_blocking(picobus.pio, picobus.linkout_sm, 0x200 | (((uint32_t)data ^ 0xff) << 1));                    

    while (receive_byte(NULL) != LINK_BYTE_ACK);
}


/*
 * Send a buffer of bytes. All bytes are acknowledged.
 */

void __not_in_flash_func(send_buffer)(const uint8_t *data, uint32_t count)
{
    while (count) {
        send_byte(*data);
        data++;
        count--;
    }
} 

/*
 * Send a magic sequence of known bytes. This marries up with the
 * wait_for_init_sequence() function and is used to initialise the
 * link. It gets the two sides in sync.
 */
void __not_in_flash_func(send_init_sequence)(void)
{
    const uint8_t init_msg[] = { 0x02, 0x04, 0x08, 0 };
    send_buffer(init_msg, sizeof(init_msg));
    while (receive_byte(NULL) != LINK_BYTE_ACK);
}

/*
 * Wait for the initialisation sequence sent by the send_init_sequence()
 * function.
 */
void wait_for_init_sequence(void)
{
    uint8_t init_msg[] = { 0x02, 0x04, 0x08, 0 };
    uint8_t *init_msg_ptr = init_msg;
    
    while (1) {
        uint8_t chr;
        while (receive_acked_byte(&chr) == LINK_BYTE_NONE);
        
        if (chr == *init_msg_ptr) {
            init_msg_ptr++;
            if (chr == '\0') break;
        } else {
            init_msg_ptr = init_msg;
        }
    }
    send_ack_to_link();
}

//-------------------------------------------------------


void __time_critical_func(blip_test_pin)( int pin )
{
  gpio_put( pin, 1 );
  __asm volatile ("nop");
  __asm volatile ("nop");
  __asm volatile ("nop");
  __asm volatile ("nop");
  gpio_put( pin, 0 );
  __asm volatile ("nop");
  __asm volatile ("nop");
  __asm volatile ("nop");
  __asm volatile ("nop");
}


/*
 * Standard 16 bit checksum, nicked from the Wikipedia entry.
 * Not part of the protocol as such, but might be useful.
 */
uint16_t fletcher16( uint8_t *data, int count )
{
  uint32_t c0, c1;

  for (c0 = c1 = 0; count > 0; )
  {
    size_t blocklen = count;
    if (blocklen > 5802) {
      blocklen = 5802;
    }
    count -= blocklen;
    do {
      c0 = c0 + *data++;
      c1 = c1 + c0;
    } while (--blocklen);
    c0 = c0 % 255;
    c1 = c1 % 255;
  }
  return (c1 << 8 | c0);
}
