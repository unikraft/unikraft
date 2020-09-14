/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Unikraft Banner
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
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

#include <uk/config.h>
#include <uk/plat/console.h>
#include <stdio.h>

/*
 * Color palette
 */
#if CONFIG_LIBUKBOOT_BANNER_POWEREDBY_ANSI || \
    CONFIG_LIBUKBOOT_BANNER_POWEREDBY_EAANSI || \
    CONFIG_LIBUKBOOT_BANNER_POWEREDBY_U8ANSI
/* Blue version (ANSI) */
#define B_RST UK_ANSI_MOD_RESET
#define B_TXT UK_ANSI_MOD_RESET UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE)
#define B_LTR UK_ANSI_MOD_RESET UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
#define B_HL0 UK_ANSI_MOD_BOLD  UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define B_HL1 UK_ANSI_MOD_RESET UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define B_HL2 UK_ANSI_MOD_BOLD  UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
#define B_HL3 UK_ANSI_MOD_RESET UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLUE)
#elif CONFIG_LIBUKBOOT_BANNER_POWEREDBY_ANSI2 || \
      CONFIG_LIBUKBOOT_BANNER_POWEREDBY_EAANSI2 || \
      CONFIG_LIBUKBOOT_BANNER_POWEREDBY_U8ANSI2
/* Gray version (ANSI) */
#define B_RST UK_ANSI_MOD_RESET
#define B_TXT UK_ANSI_MOD_RESET UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_WHITE)
#define B_LTR UK_ANSI_MOD_BOLD  UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_BLACK)
#define B_HL0 UK_ANSI_MOD_BOLD  UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define B_HL1 UK_ANSI_MOD_RESET UK_ANSI_MOD_COLORFG(UK_ANSI_COLOR_RED)
#define B_HL2 B_TXT
#define B_HL3 B_LTR
#else
/* No colors */
#define B_RST ""
#define B_TXT B_RST
#define B_LTR B_RST
#define B_HL0 B_RST
#define B_HL1 B_RST
#define B_HL2 B_RST
#define B_HL3 B_RST
#endif


void print_banner(FILE *out) {
#if CONFIG_LIBUKBOOT_BANNER_POWEREDBY || \
    CONFIG_LIBUKBOOT_BANNER_POWEREDBY_ANSI || \
    CONFIG_LIBUKBOOT_BANNER_POWEREDBY_ANSI2
	fprintf(out, B_TXT "Powered by\n");
	fprintf(out, B_HL2 "o" B_HL1 ".   " B_HL2 "." B_HL3 "o"
		B_LTR "       _ _               __ _\n");
	fprintf(out, B_HL0 "O" B_HL2 "o   " B_HL1 "O" B_HL0 "o"
		B_LTR "  ___ (_) | __ __  __ _ ' _) :_\n");
	fprintf(out, B_HL3 "o" B_HL0 "O   " B_HL2 "o" B_HL3 "O"
		B_LTR " ' _ `| | |/ /  _)' _` | |_|  _)\n");
	fprintf(out, B_HL1 "o" B_HL2 "Oo " B_HL1 "o" B_HL3 "O" B_HL1 "O"
		B_LTR "| | | | |   (| | | (_) |  _) :_\n");
	fprintf(out, B_HL3 " O" B_HL0 "o" B_HL3 "O" B_HL2 "o" B_HL3 "O "
		B_LTR "._, ._:_:_,\\_._,  .__,_:_, \\___)\n");
	fprintf(out, B_TXT "%39s" B_RST "\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#elif CONFIG_LIBUKBOOT_BANNER_POWEREDBY_EA ||		\
      CONFIG_LIBUKBOOT_BANNER_POWEREDBY_EAANSI ||	\
      CONFIG_LIBUKBOOT_BANNER_POWEREDBY_EAANSI2
	fprintf(out, B_TXT "Powered by\n");
	fprintf(out, B_HL2 "\xDF" B_HL1 "\xFE   " B_HL2 "\xFE" B_HL3 "\xDC"
		B_LTR "       _ _               __ _\n");
	fprintf(out, B_HL0 "\xDF" B_HL2 "\xFE   " B_HL1 "\xDF" B_HL0 "\xDC"
		B_LTR "  ___ (_) | __ __  __ _ ' _) :_\n");
	fprintf(out, B_HL3 "\xDC" B_HL0 "\xDF   " B_HL2 "\xDC" B_HL3 "\xFE"
		B_LTR " ' _ `| | |/ /  _)' _` | |_|  _)\n");
	fprintf(out, B_HL1 "\xFE" B_HL2 "\xDF\xFE " B_HL1 "\xFE" B_HL3 "\xDC" B_HL1 "\xDF"
		B_LTR "| | | | |   (| | | (_) |  _) :_\n");
	fprintf(out, B_HL3 " \xFE" B_HL0 "\xDC" B_HL3 "\xDF" B_HL2 "\xDC" B_HL3 "\xFE "
		B_LTR "._, ._:_:_,\\_._,  .__,_:_, \\___)\n");
	fprintf(out, B_TXT "%39s" B_RST "\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#elif CONFIG_LIBUKBOOT_BANNER_POWEREDBY_U8 ||		\
	CONFIG_LIBUKBOOT_BANNER_POWEREDBY_U8ANSI ||	\
	CONFIG_LIBUKBOOT_BANNER_POWEREDBY_U8ANSI2
	fprintf(out, B_TXT "Powered by\n");
	fprintf(out, B_HL2 "■" B_HL1 "▖   " B_HL2 "▖" B_HL3 "■"
		B_LTR "       _ _               __ _\n");
	fprintf(out, B_HL0 "■" B_HL2 "▚   " B_HL1 "■" B_HL0 "▞"
		B_LTR "  ___ (_) | __ __  __ _ ´ _) :_\n");
	fprintf(out, B_HL3 "▀" B_HL0 "■   " B_HL2 "■" B_HL3 "▄"
		B_LTR " ´ _ `| | |/ /  _)´ _` | |_|  _)\n");
	fprintf(out, B_HL1 "▄" B_HL2 "▀▄ " B_HL1 "▗" B_HL3 "▀" B_HL1 "▄"
		B_LTR "| | | | |   (| | | (_) |  _) :_\n");
	fprintf(out, B_HL3 " ▚" B_HL0 "▄" B_HL3 "■" B_HL2 "▄" B_HL3 "▞"
		B_LTR " ._, ._:_:_,\\_._,  .__,_:_, \\___)\n");
	fprintf(out, B_TXT "%39s" B_RST "\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#elif CONFIG_LIBUKBOOT_BANNER_CLASSIC
	fprintf(out, "Welcome to  _ __             _____\n");
	fprintf(out, " __ _____  (_) /__ _______ _/ _/ /_\n");
	fprintf(out, "/ // / _ \\/ /  '_// __/ _ `/ _/ __/\n");
	fprintf(out, "\\_,_/_//_/_/_/\\_\\/_/  \\_,_/_/ \\__/\n");
	fprintf(out, "%35s\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#else /* CONFIG_LIBUKBOOT_BANNER_MINIMAL  */
	fprintf(out, "Powered by Unikraft " STRINGIFY(UK_CODENAME)
		" (" STRINGIFY(UK_FULLVERSION) ")\n");
#endif
}
