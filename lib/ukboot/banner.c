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
#include <stdio.h>


void print_banner(FILE *out) {
#if CONFIG_LIBUKBOOT_BANNER_POWEREDBY
	fprintf(out, "Powered by\n");
	fprintf(out, "o.   .o       _ _               __ _\n");
	fprintf(out, "Oo   Oo  ___ (_) | __ __  __ _ ' _) :_\n");
	fprintf(out, "oO   oO ' _ `| | |/ /  _)' _` | |_|  _)\n");
	fprintf(out, "oOo oOO| | | | |   (| | | (_) |  _) :_\n");
	fprintf(out, " OoOoO ._, ._:_:_,\\_._,  .__,_:_, \\___)\n");
	fprintf(out, "%39s\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#elif CONFIG_LIBUKBOOT_BANNER_POWEREDBY_EA
	fprintf(out, "Powered by\n");
	fprintf(out, "\xDF\xFE   \xFE\xDC       _ _               __ _\n");
	fprintf(out, "\xDF\xFE   \xDF\xDC  ___ (_) | __ __  __ _ ' _) :_\n");
	fprintf(out, "\xDC\xDF   \xDC\xFE ' _ `| | |/ /  _)' _` | |_|  _)\n");
	fprintf(out, "\xFE\xDF\xFE \xFE\xDC\xDF| | | | |   (| | | (_) |  _) :_\n");
	fprintf(out, " \xFE\xDC\xDF\xDC\xFE ._, ._:_:_,\\_._,  .__,_:_, \\___)\n");
	fprintf(out, "%39s\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#elif CONFIG_LIBUKBOOT_BANNER_POWEREDBY_U8
	fprintf(out, "Powered by\n");
	fprintf(out, "■▖   ▖■       _ _               __ _\n");
	fprintf(out, "■▚   ■▞  ___ (_) | __ __  __ _ ´ _) :_\n");
	fprintf(out, "▀■   ■▄ ´ _ `| | |/ /  _)´ _` | |_|  _)\n");
	fprintf(out, "▄▀▄ ▗▀▄| | | | |   (| | | (_) |  _) :_\n");
	fprintf(out, " ▚▄■▄▞ ._, ._:_:_,\\_._,  .__,_:_, \\___)\n");
	fprintf(out, "%39s\n",
		STRINGIFY(UK_CODENAME) " " STRINGIFY(UK_FULLVERSION));

#elif CONFIG_LIBUKBOOT_BANNER_POWEREDBY_CLASSIC
	fprintf(out, "Powered by  _ __             _____\n");
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
