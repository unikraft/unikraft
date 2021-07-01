/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * read_allsyms.c: printing of symbolic oopses and stack traces.
 * Authors: Jia He <justin.he@arm.com>
 *
 * Copyright (c) 2019, Arm Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * This is simplified and rewritten from kernel/kallsyms.c
 * CommitID: 2a1a3fa0
 */
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <uk/essentials.h>
#include <uk/print.h>
#include <uk/assert.h>

#include "read_allsymbol.h"

/*
 * These will be re-linked against their real values
 * during the second link stage.
 */
extern const unsigned long ukallsyms_addresses[] __weak;
extern const __u8 ukallsyms_names[] __weak;

extern const unsigned long ukallsyms_num_syms __weak;

extern const __u8 ukallsyms_token_table[] __weak;
extern const __u16 ukallsyms_token_index[] __weak;
extern const unsigned long ukallsyms_markers[] __weak;

struct stackframe {
	unsigned long fp;
	unsigned long pc;
};

/*
 * Expand a compressed symbol data into the resulting uncompressed string,
 * if uncompressed string is too long (>= maxlen), it will be truncated,
 * given the offset to where the symbol is in the compressed stream.
 * e.g.
 * 0x03, 0xbe, 0xbc, 0x71
 * len is 0x03, then ukallsyms_token_index[0xbe] is the index of this token
 * in ukallsyms_token_table[]
 */
static unsigned int expand_symbol(unsigned int off, char *result, size_t maxlen)
{
	int len, skipped = 0;
	const __u8 *token, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &ukallsyms_names[off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		token = &ukallsyms_token_table[ukallsyms_token_index[*data]];
		data++;
		len--;

		/* skip the first char which is the length */
		while (*token) {
			if (skipped) {
				if (maxlen <= 1)
					goto tail;
				*result = *token;
				result++;
				maxlen--;
			} else
				skipped = 1;
			token++;
		}
	}

tail:
	/* Truncate if it is longer than maxlen */
	if (maxlen)
		*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}

/*
 * Find the offset on the compressed stream given and index in the
 * ukallsyms array.
 */
static unsigned int get_symbol_offset(unsigned long pos)
{
	const __u8 *name;
	unsigned int i;

	/*
	 * Use the closest marker we have. We have markers every 256 positions,
	 * so that should be close enough.
	 */
	name = &ukallsyms_names[ukallsyms_markers[pos >> 8]];

	/*
	 * Sequentially scan all the symbols up to the point we're searching
	 * for. Every symbol is stored in a [<len>][<len> bytes of data] format,
	 * so we just need to add the len to the current pointer for every
	 * symbol we wish to skip.
	 */
	for (i = 0; i < (unsigned int)(pos & 0xFF); i++)
		name = name + (*name) + 1;

	return name - ukallsyms_names;
}

static unsigned long get_symbol_pos(unsigned long addr,
				    unsigned long *symbol_size,
				    unsigned long *offset)
{
	unsigned long symbol_start = 0, symbol_end = 0;
	unsigned long i, low, high, mid = 0;

	/* Do a binary search on the sorted ukallsyms_addresses array. */
	low = 0;
	high = ukallsyms_num_syms;

	while (high - low > 1) {
		mid = low + (high - low) / 2;
		if (ukallsyms_addresses[mid] <= addr)
			low = mid;
		else
			high = mid;
	}

	/*
	 * Search for the first aliased symbol. Aliased
	 * symbols are symbols with the same address.
	 */
	while (low && ukallsyms_addresses[low-1] == ukallsyms_addresses[low])
		--low;

	symbol_start = ukallsyms_addresses[low];

	/* Search for next non-aliased symbol. */
	for (i = low + 1; i < ukallsyms_num_syms; i++) {
		if (ukallsyms_addresses[i] > symbol_start) {
			symbol_end = ukallsyms_addresses[i];
			break;
		}
	}

	/* If we found no next symbol, we use the end of the section. */
	if (!symbol_end) {
		symbol_end = (unsigned long)__END;
	}

	if (symbol_size)
		*symbol_size = symbol_end - symbol_start;
	if (offset)
		*offset = addr - symbol_start;

	return low;
}

/*
 * Lookup an address
 * - We guarantee that the returned name is valid until we reschedule even if.
 *   It resides in a module.
 */
static const char *allsyms_lookup(unsigned long addr,
			    unsigned long *symbol_size,
			    unsigned long *offset,
			    char *namebuf)
{
	namebuf[KSYM_NAME_LEN - 1] = 0;
	namebuf[0] = 0;

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, symbol_size, offset);
		/* Grab name */
		expand_symbol(get_symbol_offset(pos), namebuf, KSYM_NAME_LEN);

		return namebuf;
	}

	return 0;
}


/**
 * sprint_symbol - Look up a kernel symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function looks up a kernel symbol with @address and stores its name,
 * offset, size and module name to @buffer if possible. If no symbol was found,
 * just saves its @address as is.
 *
 * This function returns the number of bytes stored in @buffer.
 */
int sprint_symbol(char *buffer, unsigned long address)
{
	const char *name;
	unsigned long offset, size;
	int len;

	name = allsyms_lookup(address, &size, &offset, buffer);
	if (!name)
		return sprintf(buffer, "0x%lx", address);

	if (name != buffer)
		strcpy(buffer, name);
	len = strlen(buffer);

	len += sprintf(buffer + len, "+%#lx/%#lx", offset, size);

	return len;
}

/*
 * AArch64 PCS assigns the frame pointer to x29.
 *
 * A simple function prologue looks like this:
 * 	sub	sp, sp, #0x10
 *   	stp	x29, x30, [sp]
 *	mov	x29, sp
 *
 * A simple function epilogue looks like this:
 *	mov	sp, x29
 *	ldp	x29, x30, [sp]
 *	add	sp, sp, #0x10
 */
static int unwind_frame(struct stackframe *frame)
{
	unsigned long fp = frame->fp;

	if (fp & 0xf)
		return -1;

	frame->fp = (*(unsigned long *)(fp));
	frame->pc = (*(unsigned long *)(fp + 8));

	/*
	 * Frames created upon entry from EL0 have NULL FP and PC values, so
	 * don't bother reporting these. Frames created by __noreturn functions
	 * might have a valid FP even if PC is bogus, so only terminate where
	 * both are NULL.
	 */
	if (!frame->fp && !frame->pc)
		return -1;

	if (!is_ksym_addr(frame->pc))
		return -1;
	return 0;
}

static void dump_backtrace_entry(unsigned long where)
{
	char buffer[KSYM_NAME_LEN];

	sprint_symbol(buffer, where);
	printf("\t[0x%lx] %s\n", where, buffer);
}

int ukallsyms_on_each_symbol(int (*fn)(void *, const char *, unsigned long),
			    void *data)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;
	int ret;

	for (i = 0, off = 0; i < ukallsyms_num_syms; i++) {
		off = expand_symbol(off, namebuf, ARRAY_SIZE(namebuf));
		ret = fn(data, namebuf, ukallsyms_addresses[i]);
		if (ret != 0)
			return ret;
	}
	return 0;
}

void uk_dump_backtrace(void)
{
	struct stackframe frame;

	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.pc = (unsigned long)uk_dump_backtrace;

	printf("Call trace:\n");
	do {
		dump_backtrace_entry(frame.pc);
	} while (!unwind_frame(&frame));
}
