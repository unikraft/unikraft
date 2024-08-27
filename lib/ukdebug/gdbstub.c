/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2021, Karlsruhe Institute of Technology. All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "gdbstub.h"
#include <gdbsup.h>
#include <uk/plat/lcpu.h>

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/arch/limits.h>
#include <uk/isr/string.h>
#include <uk/nofault.h>

#if CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/plat/io.h>
#include <uk/page.h>
#endif /* CONFIG_HAVE_PAGING */

#include <errno.h>
#include <stddef.h>

struct gdb_excpt_ctx {
	struct __regs *regs;
	int errnr;
};

#define GDB_BUF_SIZE 2048
#define GDB_BUF_SIZE_HEX_STR "800"
static char gdb_recv_buffer[GDB_BUF_SIZE];
static char gdb_send_buffer[GDB_BUF_SIZE];

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

/* Linked to architecture-specfic target.xml description */
extern const char __gdb_target_xml_start[];
extern const char __gdb_target_xml_end[];

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

static int gdb_consume_str(char **buf, __sz *buf_len,
			   const char *prefix, __sz prefix_len)
{
	if (!gdb_starts_with(*buf, *buf_len, prefix, prefix_len))
		return 0;

	*buf += prefix_len;
	*buf_len -= prefix_len;

	return 1;
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

static __ssz gdb_mem2bin(char *bin, __sz bin_len, const char *mem, __sz mem_len)
{
	__sz rem_len = bin_len;

	while (mem_len--) {
		switch (*mem) {
		/* The following characters must be escaped */
		case '}':
		case '#':
		case '$':
		case '*':
			if (rem_len < 2)
				return bin_len - rem_len;
			*bin++ = '}';
			*bin++ = *mem++ ^ 0x20;

			rem_len -= 2;
			break;
		default:
			if (rem_len < 1)
				return bin_len - rem_len;
			*bin++ = *mem++;

			rem_len--;
			break;
		}
	}

	return bin_len - rem_len;
}

static __ssz gdb_read_memory(unsigned long addr, __sz len,
			     void *buf, __sz buf_len)
{
	return uk_nofault_memcpy(buf, (void *)addr, MIN(len, buf_len),
				 UK_NOFAULTF_NOPAGING);
}

static __ssz gdb_write_memory(unsigned long addr, __sz len,
			      void *buf, __sz buf_len)
{
#if CONFIG_HAVE_PAGING
	__paddr_t paddr = 0;
	__vaddr_t kmap_vaddr = 0;
	struct uk_pagetable *pt = __NULL;
	unsigned long n_pages = 0;
	__sz rc = 0;

	len = MIN(len, buf_len);
	n_pages = round_pgup(len) / __PAGE_SIZE;

	pt = ukplat_pt_get_active();
	if (!pt) {
		/* Paging isn't initialized. We don't need the kmap detour to
		 * work around permissions.
		 */
		return uk_nofault_memcpy((void *)addr, buf, len,
					 UK_NOFAULTF_NOPAGING);
	}

	paddr = ukplat_virt_to_phys((void *)addr);
	if (unlikely(paddr == __PADDR_INV))
		return 0;

	kmap_vaddr = ukplat_page_kmap(pt, paddr, n_pages, 0);
	if (unlikely(kmap_vaddr == __VADDR_INV))
		return 0;

	/* We use the virtual address from kmap because it grants us write
	 * access to physical pages that are marked read-only under the
	 * original virtual address.
	 */
	rc = uk_nofault_memcpy((void *)kmap_vaddr, buf, len,
			       UK_NOFAULTF_NOPAGING);

	ukplat_page_kunmap(pt, kmap_vaddr, n_pages, 0);

	return rc;
#else /* !CONFIG_HAVE_PAGING */
	return uk_nofault_memcpy((void *)addr, buf, MIN(len, buf_len),
				 UK_NOFAULTF_NOPAGING);
#endif /* !CONFIG_HAVE_PAGING */
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

/* g */
static int gdb_handle_read_registers(char *buf __unused, __sz buf_len __unused,
				     struct gdb_excpt_ctx *g)
{
	char *tmp = gdb_recv_buffer; /* Use receive buffer as temporary space */
	__ssz r;
	int i;

	for (i = 0; i < GDB_REGS_NUM; i++) {
		r = gdb_arch_read_register(i, g->regs, tmp,
					   sizeof(gdb_recv_buffer) - (tmp - gdb_recv_buffer));
		if (r < 0) {
			/* Send Enn. */
			GDB_CHECK(gdb_send_error_packet(-r));
			return 0;
		}

		tmp += r;
	}

	r = gdb_mem2hex(gdb_send_buffer, sizeof(gdb_send_buffer),
			gdb_recv_buffer, tmp - gdb_recv_buffer);

	UK_ASSERT(r >= 0);

	GDB_CHECK(gdb_send_packet(gdb_send_buffer, r));

	return 0;
}

/* G */
static int gdb_handle_write_registers(char *buf, __sz buf_len,
				      struct gdb_excpt_ctx *g)
{
	char *tmp = gdb_send_buffer; /* Use send buffer as temporary space */
	__ssz r, l;
	int i;

	l = gdb_hex2mem(gdb_send_buffer, sizeof(gdb_send_buffer),
			buf, buf_len);
	if (l < 0) {
		/* Send Enn. */
		GDB_CHECK(gdb_send_error_packet(-l));
		return 0;
	}

	for (i = 0; i < GDB_REGS_NUM; i++) {
		r = gdb_arch_write_register(i, g->regs, tmp, l);
		if (r < 0) {
			/* Send Enn. */
			GDB_CHECK(gdb_send_error_packet(-r));
			return 0;
		}

		tmp += r;
		l -= r;
	}

	GDB_CHECK(gdb_send_packet(GDB_STR_A_LEN("OK")));

	return 0;
}

/* m addr,length*/
static int gdb_handle_read_memory(char *buf, __sz buf_len,
				  struct gdb_excpt_ctx *g __unused)
{
	unsigned long addr, length;
	char *buf_end = buf + buf_len;
	__ssz r;

	addr = gdb_hex2ulong(buf, buf_end - buf, &buf);
	if ((buf == buf_end) || (*buf++ != ',')) {
		/* Send E22. EINVAL */
		GDB_CHECK(gdb_send_error_packet(EINVAL));
		return 0;
	}

	length = gdb_hex2ulong(buf, buf_end - buf, &buf);
	if (buf != buf_end) {
		/* Send E22. EINVAL */
		GDB_CHECK(gdb_send_error_packet(EINVAL));
		return 0;
	}

	r = gdb_read_memory(addr, length, gdb_recv_buffer,
				 sizeof(gdb_recv_buffer) / 2);
	if (unlikely(r < 0)) {
		/* Send Enn. */
		GDB_CHECK(gdb_send_error_packet(-r));
		return 0;
	}

	r = gdb_mem2hex(gdb_send_buffer, sizeof(gdb_send_buffer),
			gdb_recv_buffer, r);

	GDB_CHECK(gdb_send_packet(gdb_send_buffer, r));

	return 0;
}

/* M addr,length*/
static int gdb_handle_write_memory(char *buf, __sz buf_len,
				   struct gdb_excpt_ctx *g __unused)
{
	unsigned long addr, length;
	char *buf_end = buf + buf_len;
	__ssz r;

	addr = gdb_hex2ulong(buf, buf_end - buf, &buf);
	if ((buf == buf_end) || (*buf++ != ',')) {
		/* Send E22. EINVAL */
		GDB_CHECK(gdb_send_error_packet(EINVAL));
		return 0;
	}

	length = gdb_hex2ulong(buf, buf_end - buf, &buf);
	if ((buf == buf_end) || (*buf++ != ':')) {
		/* Send E22. EINVAL */
		GDB_CHECK(gdb_send_error_packet(EINVAL));
		return 0;
	}

	r = gdb_hex2mem(gdb_send_buffer, sizeof(gdb_send_buffer),
			buf, buf_end - buf);
	if (unlikely(r < 0)) {
		/* Send Enn. */
		GDB_CHECK(gdb_send_error_packet(-r));
		return 0;
	}

	r = gdb_write_memory(addr, length, gdb_send_buffer, r);
	if (unlikely(r < 0)) {
		/* Send Enn. */
		GDB_CHECK(gdb_send_error_packet(-r));
		return 0;
	}

	GDB_CHECK(gdb_send_packet(GDB_STR_A_LEN("OK")));

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

static int gdb_handle_multiletter_cmd(struct gdb_cmd_table_entry *table,
				      __sz table_entries, char *buf,
				      __sz buf_len, struct gdb_excpt_ctx *g)
{
	__sz i, l, max_len_idx = 0, max_len = 0;

	for (i = 0; i < table_entries; i++) {
		l = table[i].cmd_len;

		if (l > buf_len)
			continue;

		if (memcmp_isr(buf, table[i].cmd, l) != 0)
			continue;

		if (l > max_len) {
			max_len = l;
			max_len_idx = i;
		}
	}

	if (max_len > 0 && max_len <= buf_len) {
		/* If the remaining length if zero, prefer passing a NULL
		 * pointer over passing a pointer to garbage data.
		 */
		buf = max_len == buf_len ? NULL : buf + max_len;
		return table[max_len_idx].f(buf, buf_len - max_len, g);
	}

	/* Send empty packet to signal GDB that
	 * we do not support the command
	 */
	GDB_CHECK(gdb_send_empty_packet());

	return 0;
}

/* qSupported[:gdbfeature [;gdbfeature]... ] */
static int gdb_handle_qsupported(char *buf __unused, __sz buf_len __unused,
				 struct gdb_excpt_ctx *g __unused)
{
#define GDB_SUPPORT_STR \
	"PacketSize=" GDB_BUF_SIZE_HEX_STR \
	";qXfer:features:read+"

	GDB_CHECK(gdb_send_packet(GDB_STR_A_LEN(GDB_SUPPORT_STR)));

	return 0;
}

static int gdb_qXfer_mem(const char *mem, __sz mem_len, __sz offset,
			 __sz length)
{
	__ssz r;

	if (offset >= mem_len) {
		/* Send l. No more data to be read */
		GDB_CHECK(gdb_send_packet(GDB_STR_A_LEN("l")));
	} else {
		length = MIN(length, mem_len - offset);
		length = MIN(length, sizeof(gdb_send_buffer) - 1);

		gdb_send_buffer[0] = 'm'; /* There is more data */

		r = gdb_mem2bin(gdb_send_buffer + 1,
				sizeof(gdb_send_buffer) - 1,
				mem + offset, length);

		UK_ASSERT(r > 0);

		GDB_CHECK(gdb_send_packet(gdb_send_buffer, r + 1));
	}

	return 0;
}

/* qXfer:object:read:annex:offset,length */
static int gdb_handle_qXfer(char *buf, __sz buf_len,
			    struct gdb_excpt_ctx *g __unused)
{
	unsigned long offset, length;
	char *buf_end = buf + buf_len;

	if (!gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(":features:read"))) {
		/* Send empty packet. Unsupported object requested */
		GDB_CHECK(gdb_send_empty_packet());
		return 0;
	}

	if (!gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(":target.xml:"))) {
		/* Send E00. Annex invalid */
		GDB_CHECK(gdb_send_error_packet(0x00));
		return 0;
	}

	offset = gdb_hex2ulong(buf, buf_end - buf, &buf);
	if ((buf >= buf_end) || (*buf++ != ',')) {
		/* Send E00. Request malformed */
		GDB_CHECK(gdb_send_error_packet(0x00));
		return 0;
	}

	length = gdb_hex2ulong(buf, buf_end - buf, &buf);
	if (buf != buf_end) {
		/* Send E00. Request malformed */
		GDB_CHECK(gdb_send_error_packet(0x00));
		return 0;
	}

	return gdb_qXfer_mem(__gdb_target_xml_start,
		__gdb_target_xml_end - __gdb_target_xml_start,
		offset, length);
}

/* qAttached */
static int gdb_handle_qAttached(char *buf __unused, __sz buf_len,
				struct gdb_excpt_ctx *g __unused)
{
	/* NOTE: If multiprocess extensions are enabled, there's an additional
	 * :pid field at the end of the qAttached packet. Since multiprocess
	 * extensions are not currently supported, we just check that this field
	 * is not present.
	 */
	if (buf_len) {
		GDB_CHECK(gdb_send_error_packet(EINVAL));
		return 0;
	}

	/* We attached to an existing process */
	GDB_CHECK(gdb_send_packet(GDB_STR_A_LEN("1")));

	return 0;
}

static struct gdb_cmd_table_entry gdb_q_cmd_table[] = {
	{ gdb_handle_qsupported, GDB_STR_A_LEN("Supported") },
	{ gdb_handle_qXfer, GDB_STR_A_LEN("Xfer") },
	{ gdb_handle_qAttached, GDB_STR_A_LEN("Attached") }
};

#define NUM_GDB_Q_CMDS (sizeof(gdb_q_cmd_table) / \
		sizeof(struct gdb_cmd_table_entry))

static int gdb_handle_q_cmd(char *buf, __sz buf_len,
			    struct gdb_excpt_ctx *g)
{
	return gdb_handle_multiletter_cmd(gdb_q_cmd_table, NUM_GDB_Q_CMDS,
		buf, buf_len, g);
}

/* vCont[;action[:thread-id]]... */
static int gdb_handle_vCont(char *buf, __sz buf_len,
			    struct gdb_excpt_ctx *g __unused)
{
	char *buf_end = buf + buf_len;
	int rc = 0;

	/* NOTE: We don't support the thread-related features of the gdb remote
	 * protocol. That's why we make the implicit assumption that there is
	 * only one thread. And we don't accept any values in the thread-id
	 * field other than 0 (any thread) or -1 (all threads). Together this
	 * means we only accept a single action and optional thread-id field.
	 * Whatever the thread-id field's value (omitted, 0 or -1), then
	 * behavior is the same: apply the action to the current thread of
	 * execution.
	 */

	if (gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(";c"))) {
		rc = GDB_DBG_CONT;
	} else if (gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(";C"))) {
		gdb_hex2ulong(buf, buf_len, &buf); /* Ignore the signal */
		rc = GDB_DBG_CONT;
	} else if (gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(";s"))) {
		rc = GDB_DBG_STEP;
	} else if (gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(";S"))) {
		gdb_hex2ulong(buf, buf_len, &buf); /* Ignore the signal */
		rc = GDB_DBG_STEP;
	} else {
		/* Either no action was specified (which is considered an error)
		 * or the action is not supported (also an error because the
		 * response to vCont? tells the host which packets are supported
		 */
		GDB_CHECK(gdb_send_error_packet(EINVAL));
		return 0;
	}

	if (buf == buf_end ||
	    gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(":0")) ||
	    gdb_consume_str(&buf, &buf_len, GDB_STR_A_LEN(":-1"))) {
		return rc;
	}

	GDB_CHECK(gdb_send_error_packet(EINVAL));
	return 0;
}

/* vCont? */
static int gdb_handle_vCont_ask(char *buf __unused, __sz buf_len __unused,
				struct gdb_excpt_ctx *g __unused)
{
	GDB_CHECK(gdb_send_packet(GDB_STR_A_LEN("vCont;c;C;s;S")));

	return 0;
}

static struct gdb_cmd_table_entry gdb_v_cmd_table[] = {
	{ gdb_handle_vCont_ask, GDB_STR_A_LEN("Cont?") },
	{ gdb_handle_vCont, GDB_STR_A_LEN("Cont") },
};

#define NUM_GDB_V_CMDS (sizeof(gdb_v_cmd_table) / \
		sizeof(struct gdb_cmd_table_entry))

static int gdb_handle_v_cmd(char *buf, __sz buf_len,
			    struct gdb_excpt_ctx *g)
{
	return gdb_handle_multiletter_cmd(gdb_v_cmd_table, NUM_GDB_V_CMDS,
		buf, buf_len, g);
}

static struct gdb_cmd_table_entry gdb_cmd_table[] = {
	{ gdb_handle_stop_reason, GDB_STR_A_LEN("?") },
	{ gdb_handle_read_registers, GDB_STR_A_LEN("g") },
	{ gdb_handle_write_registers, GDB_STR_A_LEN("G") },
	{ gdb_handle_read_memory, GDB_STR_A_LEN("m") },
	{ gdb_handle_write_memory, GDB_STR_A_LEN("M") },
	{ gdb_handle_continue, GDB_STR_A_LEN("c") },
	{ gdb_handle_continue_sig, GDB_STR_A_LEN("C") },
	{ gdb_handle_step, GDB_STR_A_LEN("s") },
	{ gdb_handle_step_sig, GDB_STR_A_LEN("S") },
	{ gdb_handle_q_cmd, GDB_STR_A_LEN("q") },
	{ gdb_handle_v_cmd, GDB_STR_A_LEN("v") }
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
