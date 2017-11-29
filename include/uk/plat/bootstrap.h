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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef __UKPLAT_BOOTSTRAP__
#define __UKPLAT_BOOTSTRAP__

#include <uk/arch/types.h>
#include <uk/essentials.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Called by platform library during initialization
 * This function has to be provided by a non-platform library for bootstrapping
 * It is called directly by platform libraries that have parsed arguments
 * already, otherwise a platform library will call ukplat_entry_argp() to let
 * the arguments parsed from a string buffer
 * @param argc Number of arguments
 * @param args Array to '\0'-terminated arguments
 */
void ukplat_entry(int argc, char *argv[]) __noreturn;

/**
 * Called by platform library during initialization
 * This function has to be provided by a non-platform library for bootstrapping
 * It is intended to do the argument parsing only and then call ukplat_entry()
 * @param arg0     Optional buffer for first argument (argv[0]), can be __NULL
 *                 Arguments found on argb will be placed on argv[1] onwards
 * @param argb     Buffer of arguments, can be __NULL
 * @param argb_len Length of arg buffer (set to __SZ_MAX, when argb should be
 *                 handled as zero-terminated)
 */
void ukplat_entry_argp(char *arg0, char *argb, __sz argb_len) __noreturn;

enum ukplat_gstate {
	UKPLAT_HALT,
	UKPLAT_RESTART,
	UKPLAT_SUSPEND,
	UKPLAT_CRASH,
};

/**
 * Terminates this guest
 * @param request Specify the type of shutdown
 *                Invalid requests are mapped to UKPLAT_CRASH
 */
void ukplat_terminate(enum ukplat_gstate request) __noreturn;

/**
 * Halts this guest
 */
#define ukplat_halt()				\
	(ukplat_terminate(UKPLAT_HALT))

/**
 * Restarts this guest
 */
#define ukplat_restart()			\
	(ukplat_terminate(UKPLAT_RESTART))

/**
 * Halts this guest with signalling a crash
 */
#define ukplat_crash()				\
	(ukplat_terminate(UKPLAT_CRASH))

/**
 * Suspends this guest
 * @return 0 after guest suspend ended, <0 on errors
 */
int ukplat_suspend(void);

#ifdef __cplusplus
}
#endif

#endif /* __UKPLAT_BOOTSTRAP__ */
