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

void _libkvmplat_init_serial_console(void);
void _libkvmplat_serial_putc(char a);
int  _libkvmplat_serial_getc(void);

#endif /* __KVM_SERIAL_CONSOLE__ */
