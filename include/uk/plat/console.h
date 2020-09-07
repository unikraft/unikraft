/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#ifndef __UKPLAT_CONSOLE_H__
#define __UKPLAT_CONSOLE_H__

#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ANSI control sequence helpers
 */
#define __UK_ANSI_INTRO			"\033["
#define __UK_ANSI_CMD0(cmd)		__UK_ANSI_INTRO cmd
#define __UK_ANSI_CMD1(cmd, arg)	__UK_ANSI_INTRO arg cmd
#define __UK_ANSI_CMD2(cmd, arg0, arg1)	__UK_ANSI_INTRO arg0 ";" arg1 cmd
#define UK_ANSI_CLEARLINE		__UK_ANSI_CMD0("K")
#define UK_ANSI_CLEARSCREEN		__UK_ANSI_CMD1("J", "2")
#define UK_ANSI_HOME			__UK_ANSI_CMD0("H")
#define UK_ANSI_UP(n)			__UK_ANSI_CMD1("A", STRINGIFY(n))
#define UK_ANSI_DOWN(n)			__UK_ANSI_CMD1("B", STRINGIFY(n))
#define UK_ANSI_RIGHT(n)		__UK_ANSI_CMD1("C"; STRINGIFY(n))
#define UK_ANSI_LEFT(n)			__UK_ANSI_CMD1("D", STRINGIFY(n))
#define UK_ANSI_POS(l, c)		__UK_ANSI_CMD2("H", STRINGIFY(l), \
						       STRINGIFY(c))

#define __UK_ANSI_MOD1(m)		__UK_ANSI_CMD1("m", m)
#define __UK_ANSI_MOD2(m0, m1)		__UK_ANSI_CMD2("m", m0, m1)
#define UK_ANSI_MOD_RESET		__UK_ANSI_MOD1("0")
#define UK_ANSI_MOD_BOLD		__UK_ANSI_MOD1("1")
#define UK_ANSI_MOD_UNDERSCORE		__UK_ANSI_MOD1("4")
#define UK_ANSI_MOD_BLINK		__UK_ANSI_MOD1("5")
#define UK_ANSI_MOD_INVERT		__UK_ANSI_MOD1("7")
#define UK_ANSI_MOD_HIDE		__UK_ANSI_MOD1("8")

#define __UK_ANSI_MOD_COLOR_FG0		"30"
#define __UK_ANSI_MOD_COLOR_FG1		"31"
#define __UK_ANSI_MOD_COLOR_FG2		"32"
#define __UK_ANSI_MOD_COLOR_FG3		"33"
#define __UK_ANSI_MOD_COLOR_FG4		"34"
#define __UK_ANSI_MOD_COLOR_FG5		"35"
#define __UK_ANSI_MOD_COLOR_FG6		"36"
#define __UK_ANSI_MOD_COLOR_FG7		"37"
#define __UK_ANSI_MOD_COLOR_BG0		"40"
#define __UK_ANSI_MOD_COLOR_BG1		"41"
#define __UK_ANSI_MOD_COLOR_BG2		"42"
#define __UK_ANSI_MOD_COLOR_BG3		"43"
#define __UK_ANSI_MOD_COLOR_BG4		"44"
#define __UK_ANSI_MOD_COLOR_BG5		"45"
#define __UK_ANSI_MOD_COLOR_BG6		"46"
#define __UK_ANSI_MOD_COLOR_BG7		"47"

#define UK_ANSI_MOD_COLOR(fg, bg)	__UK_ANSI_MOD2( \
					 UK_CONCAT(__UK_ANSI_MOD_COLOR_FG, fg),\
					 UK_CONCAT(__UK_ANSI_MOD_COLOR_BG, bg))
#define UK_ANSI_MOD_COLORFG(fg)		__UK_ANSI_MOD1( \
					 UK_CONCAT(__UK_ANSI_MOD_COLOR_FG, fg))
#define UK_ANSI_MOD_COLORBG(bg)		__UK_ANSI_MOD1( \
					 UK_CONCAT(__UK_ANSI_MOD_COLOR_BG, bg))

#define UK_ANSI_COLOR_BLACK		0
#define UK_ANSI_COLOR_RED		1
#define UK_ANSI_COLOR_GREEN		2
#define UK_ANSI_COLOR_YELLOW		3
#define UK_ANSI_COLOR_BLUE		4
#define UK_ANSI_COLOR_MAGENTA		5
#define UK_ANSI_COLOR_CYAN		6
#define UK_ANSI_COLOR_WHITE		7


/**
 * Outputs a string to kernel console
 * Note that printing does not stop on null termination
 * @param buf Buffer with characters
 * @param len Length of string buffer (if 0 str has to be zero-terminated),
 *            < 0 ignored
 * @return Number of printed characters, errno on < 0
 */
int ukplat_coutk(const char *buf, unsigned int len);

/**
 * Outputs a string to debug console
 * Note that printing does not stop on null termination
 * @param buf Buffer with characters
 * @param len Length of string buffer (if 0 str has to be zero-terminated)
 * @return Number of printed characters, errno on < 0
 */
int ukplat_coutd(const char *buf, unsigned int len);

/**
 * Reads characters from kernel console
 * Note that returned buf is not null terminated.
 * @param buf Target buffer
 * @param len Length of string buffer
 * @return Number of read characters, errno on < 0
 */
int ukplat_cink(char *buf, unsigned int maxlen);

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_CONSOLE_H__ */
