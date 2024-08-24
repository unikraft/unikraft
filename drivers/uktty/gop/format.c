/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "format.h"

/* State of the ANSI escape code parser */
enum state {
	PRINTABLE_CHAR,	/* Encountered a printable char */
	ESCAPED_HALF,	/* Encountered escape char `\033` */
	ESCAPED_FULL,	/* Encountered `[` right after `\033` */
};

/* Char formatting */
__u8 format_fg;
__u8 format_bg;

/* The code is of the format `\033[<code>m` */
static __u8 cur_code = 0; /* Part of the code that has been parsed */
static enum state cur_state = PRINTABLE_CHAR;

/* Set formatting of char.
 * Return false if the char is part of an
 * escape sequence and must not be printed.
 * Return true if the char is printable.
 */
bool format_char(char c)
{
	switch (cur_state) {
	case PRINTABLE_CHAR:
		/* Print char if it is not `\033` */
		if (c == '\033') {
			cur_state = ESCAPED_HALF;
			break;
		}
		return true;
	case ESCAPED_HALF:
		if (c == '[') {
			cur_state = ESCAPED_FULL;
		} else if (c != '\033') {
			cur_state = PRINTABLE_CHAR;
			return true;
		}
		break;
	case ESCAPED_FULL:
		if (c >= '0' && c <= '9') {
			cur_code = cur_code * 10 + (c - '0');
		} else if (c == 'm') {
			cur_state = PRINTABLE_CHAR;
			if (cur_code == 0) {
				/* Reset colors */
				format_fg = 37;
				format_bg = 40;
			} else if (cur_code >= 30 && cur_code < 40) {
				format_fg = cur_code;
			} else if (cur_code >= 40 && cur_code < 50) {
				format_bg = cur_code;
			}
			cur_code = 0;
		} else if (c == '\033') {
			cur_state = ESCAPED_HALF;
		} else {
			cur_state = PRINTABLE_CHAR;
			return true;
		}
		break;
	default:
		;
	}
	return false;
}
