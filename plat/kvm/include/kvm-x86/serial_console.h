/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dafna Hirschfeld <dafna3@gmail.com>
 *
 * Copyright (c) 2018 Dafna Hirschfeld <dafna3@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __KVM_SERIAL_CONSOLE__
#define __KVM_SERIAL_CONSOLE__

#define COM1 0x3f8

#define COM1_DATA (COM1 + 0)
#define COM1_INTR (COM1 + 1)
#define COM1_CTRL (COM1 + 3)
#define COM1_STATUS (COM1 + 5)

/* only when DLAB is set */
#define COM1_DIV_LO (COM1 + 0)
#define COM1_DIV_HI (COM1 + 1)

/* baudrate divisor */
#define COM1_BAUDDIV_HI 0x00

#if CONFIG_KVM_SERIAL_BAUD_19200
#define COM1_BAUDDIV_LO 0x04
#elif CONFIG_KVM_SERIAL_BAUD_38400
#define COM1_BAUDDIV_LO 0x03
#elif CONFIG_KVM_SERIAL_BAUD_57600
#define COM1_BAUDDIV_LO 0x02
#else /* default, CONFIG_KVM_SERIAL_BAUD_115200 */
#define COM1_BAUDDIV_LO 0x01
#endif

#define DLAB 0x80
#define PROT 0x03 /* 8N1 (8 bits, no parity, one stop bit) */

#if !__ASSEMBLY__
void _libkvmplat_init_serial_console(void);
void _libkvmplat_serial_putc(char a);
int  _libkvmplat_serial_getc(void);
#endif /* !__ASSEMBLY__ */
#endif /* __KVM_SERIAL_CONSOLE__ */
