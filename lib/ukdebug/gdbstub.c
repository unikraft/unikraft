/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "gdbstub.h"
#include <uk/plat/lcpu.h>

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/isr/string.h>

#include <errno.h>
#include <stddef.h>

struct gdb_excpt_ctx {
	struct __regs *regs;
	int errnr;
};

#define GDB_BUF_SIZE 2048
static char gdb_recv_buffer[GDB_BUF_SIZE];

#define GDB_PACKET_RETRIES 5

#define GDB_STR_A_LEN(str) str, (sizeof(str) - 1)
#define GDB_CHECK(expr)				\
	do {					\
		__ssz __r = (expr);		\
		if (unlikely(__r < 0))		\
			return (int)__r;	\
	} while (0)

typedef int (*gdb_cmd_handler_func)(char *buf, __sz buf_len,
	struct gdb_excpt_ctx *g);

struct gdb_cmd_table_entry {
	gdb_cmd_handler_func f;
	const char *cmd;
	__sz cmd_len;
};

int gdb_dbg_putc(char b __unused)
{
	return -ENOTSUP;
}

int gdb_dbg_getc(void)
{
	return -ENOTSUP;
}

static int gdb_starts_with(const char *buf, __sz buf_len,
			   const char *prefix, __sz prefix_len)
{
	if (unlikely(buf_len < prefix_len))
		return 0;

	return (memcmp_isr(buf, prefix, prefix_len) == 0);
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

static unsigned long gdb_hex2ulong(const char *buf, __sz buf_len, char **endptr)
{
	unsigned long val = 0;
	int i;

	/* Skip any whitespace */
	while ((buf_len > 0) && (*buf == ' ')) {
		buf++;
		buf_len--;
	}

	/* Skip hex prefix if present */
	if ((buf_len >= 2) && (*buf == '0') &&
	    ((*(buf + 1) == 'x') || (*(buf + 1) == 'X'))) {
		buf += 2;
		buf_len -= 2;
	}

	/* Parse hexadecimal integer */
	while ((buf_len > 0) && (val < __UL_MAX)) {
		i = gdb_hex2int(*buf);
		if (i < 0)
			break;

		val <<= 4;
		val |= i;

		buf++;
		buf_len--;
	}

	if (endptr)
		*endptr = DECONST(char *, buf);

	return val;
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

/* ? */
static int gdb_handle_stop_reason(char *buf __unused, __sz buf_len __unused,
				  struct gdb_excpt_ctx *g)
{
	GDB_CHECK(gdb_send_signal_packet(g->errnr));

	return 0;
}

/* c/s [addr] */
static int gdb_handle_step_cont(char *buf, __sz buf_len,
				struct gdb_excpt_ctx *g)
{
	unsigned long addr;
	char *buf_end = buf + buf_len;

	if (buf != buf_end) {
		addr = gdb_hex2ulong(buf, buf_len, &buf);
		if (buf != buf_end) {
			/* Send E22. EINVAL */
			GDB_CHECK(gdb_send_error_packet(EINVAL));
			return 0;
		}

		ukarch_regs_set_pc(addr, g->regs);
	}

	return 1;
}

/* C/S sig[;addr] */
static int gdb_handle_step_cont_sig(char *buf, __sz buf_len,
				    struct gdb_excpt_ctx *g)
{
	unsigned long addr;
	char *buf_end = buf + buf_len;

	/* Just ignore the signal */
	gdb_hex2ulong(buf, buf_end - buf, &buf);
	if (buf != buf_end) {
		if (unlikely(*buf++ != ';')) {
			/* Send E22. EINVAL */
			GDB_CHECK(gdb_send_error_packet(EINVAL));
			return 0;
		}

		addr = gdb_hex2ulong(buf, buf_len, &buf);
		if (unlikely(buf != buf_end)) {
			/* Send E22. EINVAL */
			GDB_CHECK(gdb_send_error_packet(EINVAL));
			return 0;
		}

		ukarch_regs_set_pc(addr, g->regs);
	}

	return 1;
}

/* c[addr] */
static int gdb_handle_continue(char *buf, __sz buf_len,
			       struct gdb_excpt_ctx *g)
{
	int r;

	return (((r = gdb_handle_step_cont(buf, buf_len, g)) <= 0) ?
			r : GDB_DBG_CONT);
}

/* C sig[;addr] */
static int gdb_handle_continue_sig(char *buf, __sz buf_len,
				   struct gdb_excpt_ctx *g)
{
	int r;

	return (((r = gdb_handle_step_cont_sig(buf, buf_len, g)) <= 0) ?
			r : GDB_DBG_CONT);
}

/* s[addr] */
static int gdb_handle_step(char *buf, __sz buf_len,
			   struct gdb_excpt_ctx *g)
{
	int r;

	return (((r = gdb_handle_step_cont(buf, buf_len, g)) <= 0) ?
			r : GDB_DBG_STEP);
}

/* S sig[;addr] */
static int gdb_handle_step_sig(char *buf, __sz buf_len,
			       struct gdb_excpt_ctx *g)
{
	int r;

	return (((r = gdb_handle_step_cont_sig(buf, buf_len, g)) <= 0) ?
			r : GDB_DBG_STEP);
}

static struct gdb_cmd_table_entry gdb_cmd_table[] = {
	{ gdb_handle_stop_reason, GDB_STR_A_LEN("?") },
	{ gdb_handle_continue, GDB_STR_A_LEN("c") },
	{ gdb_handle_continue_sig, GDB_STR_A_LEN("C") },
	{ gdb_handle_step, GDB_STR_A_LEN("s") },
	{ gdb_handle_step_sig, GDB_STR_A_LEN("S") }
};

#define NUM_GDB_CMDS (sizeof(gdb_cmd_table) / \
		sizeof(struct gdb_cmd_table_entry))

static int gdb_main_loop(struct gdb_excpt_ctx *g)
{
	__ssz r;
	__sz i, l;

	GDB_CHECK(gdb_send_signal_packet(g->errnr));

	do {
		r = gdb_recv_packet(gdb_recv_buffer, sizeof(gdb_recv_buffer));
		if (unlikely(r < 0)) {
			break;
		} else if (r == 0) {
			/* We received an empty packet */
			continue;
		}

		for (i = 0; i < NUM_GDB_CMDS; i++) {
			l = gdb_cmd_table[i].cmd_len;

			if (!gdb_starts_with(gdb_recv_buffer, r,
					     gdb_cmd_table[i].cmd, l)) {
				continue;
			}

			r = gdb_cmd_table[i].f(gdb_recv_buffer + l, r - l,
				g);

			break;
		}

		if (i == NUM_GDB_CMDS) {
			/* Send empty packet to signal GDB that
			 * we do not support the command
			 */
			GDB_CHECK(gdb_send_empty_packet());

			r = 0;	/* Ignore unsupported commands */
		}
	} while (r == 0);

	UK_ASSERT(r < 0 || r == GDB_DBG_CONT || r == GDB_DBG_STEP);

	return (int)r;
}

int gdb_dbg_trap(int errnr, struct __regs *regs)
{
	struct gdb_excpt_ctx g = {regs, errnr};
	static int nest_cnt;
	unsigned long irqs;
	int r;

	/* TODO: SMP support
	 * If we have SMP support, we must freeze all other CPUs here and
	 * resume them before returning. However, it might be that another
	 * CPU tries to enter the debugger at the same time. We therefore
	 * need to protect the debugger with a spinlock and try to lock it.
	 * If the current CPU does not get the lock, a different CPU is
	 * already in the debugger. The unsuccessful CPU has to wait for the
	 * lock to become available, but also remain responsive to the freeze
	 * that the first CPU initiates.
	 *
	 * Until we have SMP support, it is safe to just disable interrupts.
	 * We might re-enter nevertheless, for example, due to an UK_ASSERT
	 * in the debugger code itself or if we set a breakpoint in code
	 * called by the debugger.
	 */
#ifdef UKPLAT_LCPU_MULTICORE
#warning The GDB stub does not support multicore systems
#endif

	irqs = ukplat_lcpu_save_irqf();
	if (nest_cnt > 0) {
		ukplat_lcpu_restore_irqf(irqs);
		return GDB_DBG_CONT;
	}

	nest_cnt++;
	r = gdb_main_loop(&g);
	nest_cnt--;

	ukplat_lcpu_restore_irqf(irqs);

	return r;
}
