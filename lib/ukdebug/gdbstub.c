/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/limits.h>

#include <errno.h>
#include <stddef.h>

#define GDB_PACKET_RETRIES 5

#define GDB_CHECK(expr)				\
	do {					\
		__ssz __r = (expr);		\
		if (unlikely(__r < 0))		\
			return (int)__r;	\
	} while (0)

int gdb_dbg_putc(char b __unused)
{
	return -ENOTSUP;
}

int gdb_dbg_getc(void)
{
	return -ENOTSUP;
}

static char gdb_checksum(const char *buf, __sz len)
{
	char c = 0;

	while (len--)
		c += *buf++;

	return c;
}

static char *gdb_byte2hex(char *hex, __sz hex_len, char b)
{
	static const char map_byte2hex[] = "0123456789abcdef";

	if (unlikely(hex_len < 2))
		return NULL;

	*hex++ = map_byte2hex[(b & 0xf0) >> 4]; /* Encode high nibble */
	*hex++ = map_byte2hex[(b & 0x0f) >> 0]; /* Encode low nibble */

	return hex;
}

static __ssz gdb_mem2hex(char *hex, __sz hex_len, const char *mem, __sz mem_len)
{
	__sz l = mem_len;

	if (unlikely(hex_len < mem_len * 2))
		return -ENOMEM;

	while (l--)
		hex = gdb_byte2hex(hex, 2, *mem++);

	return mem_len * 2;
}

static int gdb_hex2int(char hex)
{
	if ((hex >= '0') && (hex <= '9'))
		return hex - '0';
	else if ((hex >= 'a') && (hex <= 'f'))
		return hex - 'a' + 0xa;
	else if ((hex >= 'A') && (hex <= 'F'))
		return hex - 'A' + 0xa;

	return -EINVAL;
}

static __ssz gdb_hex2mem(char *mem, __sz mem_len, const char *hex, __sz hex_len)
{
	__sz l = hex_len;
	int i;

	if (unlikely(hex_len % 2 != 0))
		return -EINVAL;

	if (unlikely(mem_len < hex_len / 2))
		return -ENOMEM;

	for (; l > 0; l -= 2, mem++) {
		/* Decode high nibble */
		i = gdb_hex2int(*hex++);
		if (i < 0)
			return i;

		*mem = i << 4;

		/* Decode low nibble */
		i = gdb_hex2int(*hex++);
		if (i < 0)
			return i;

		*mem |= i;
	}

	return hex_len / 2;
}

static __ssz gdb_send(const char *buf, __sz len)
{
	__sz l = len;

	while (l--)
		GDB_CHECK(gdb_dbg_putc(*buf++));

	return len;
}

static __ssz gdb_recv(char *buf, __sz len)
{
	__sz l = len;
	int r;

	while (l--) {
		r = gdb_dbg_getc();
		if (r < 0)
			return r;

		*buf++ = (char)r;
	}

	return len;
}

static int gdb_send_ack(void)
{
	return gdb_dbg_putc('+');
}

static int gdb_send_nack(void)
{
	return gdb_dbg_putc('-');
}

static int gdb_recv_ack(void)
{
	int r;

	r = gdb_dbg_getc();
	if (unlikely(r < 0))
		return r;

	return (char)r != '+';
}

static __ssz gdb_send_packet(const char *buf, __sz len)
{
	char hex[2];
	char chksum = gdb_checksum(buf, len);
	int r, retries = 0;

	gdb_mem2hex(hex, sizeof(hex), &chksum, 1);

	/* GDB packet format: $<DATA>#<CC>
	 * where CC is the GDB packet checksum
	 */
	do {
		if (unlikely(retries++ > GDB_PACKET_RETRIES))
			return -1;

		GDB_CHECK(gdb_dbg_putc('$'));
		GDB_CHECK(gdb_send(buf, len));
		GDB_CHECK(gdb_dbg_putc('#'));
		GDB_CHECK(gdb_send(hex, sizeof(hex)));
	} while ((r = gdb_recv_ack()) > 0);

	return (!r) ? (__ssz)len : r;
}

/* '' */
static __ssz gdb_send_empty_packet(void)
{
	return gdb_send_packet(NULL, 0);
}

/* S nn */
static __ssz gdb_send_signal_packet(int errnr)
{
	char buf[3];

	UK_ASSERT((errnr & 0xff) == errnr);

	buf[0] = 'S';
	gdb_byte2hex(buf + 1, sizeof(buf) - 1, errnr);

	return gdb_send_packet(buf, sizeof(buf));
}

/* E nn */
static __ssz gdb_send_error_packet(int err)
{
	char buf[3];

	UK_ASSERT((err & 0xff) == err);

	buf[0] = 'E';
	gdb_byte2hex(buf + 1, sizeof(buf) - 1, err);

	return gdb_send_packet(buf, sizeof(buf));
}

static __ssz gdb_recv_packet(char *buf, __sz len)
{
	int c, retries = 0;
	char *p = buf;
	__sz n = 0;

	/* Wait for packet start character */
	while ((c = gdb_dbg_getc()) != '$') {
		if (unlikely(c < 0))
			return c;
	}

	while (n < len) {
		c = gdb_dbg_getc();
		if (unlikely(c < 0)) {
			return c;
		} else if (c == '#') {
			char hex[2];
			char chksum = 0;

			GDB_CHECK(gdb_recv(hex, sizeof(hex)));

			if (gdb_hex2mem(&chksum, 1, hex, sizeof(hex)) < 0) {
				GDB_CHECK(gdb_send_nack());
				continue;
			}

			if (chksum != gdb_checksum(buf, n)) {
				GDB_CHECK(gdb_send_nack());
				continue;
			}

			GDB_CHECK(gdb_send_ack());

			/* Null-terminate the data */
			*p = '\0';
			return n;
		} else if (c == '$') {
			if (retries++ > GDB_PACKET_RETRIES)
				break;

			/* We received a packet start character and maybe
			 * missed some characters on the way.
			 * Start all over again.
			 */
			p = buf;
			n = 0;
		} else {
			*p++ = c;
			n++;
		}
	}

	/* We ran out of space */
	return -ENOMEM;
}
