/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cason Schindler & Jack Raney <cason.j.schindler@gmail.com>
 *          Cezar Craciunoiu <cezar.craciunoiu@gmail.com>
 *
 * Copyright (c) 2019, The University of Texas at Austin. All rights reserved.
 *               2021, University Politehnica of Bucharest. All rights reserved.
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

#ifndef _PLAT_DRV_BALLOON_H_
#define _PLAT_DRV_BALLOON_H_

#include <inttypes.h>
#include <sys/types.h>

/**
 * Deflates the balloon and asks for pages from the host
 *
 * @param pages_to_guest the pages to give to the guest
 * @param num the number of pages
 * @return the number of pages taken from host or < 0 on error
 */
int deflate_balloon(uintptr_t *pages_to_guest, uint32_t num);

/**
 * Inflates the balloon and returns unused pages to the host
 *
 * @param pages_to_host the pages to give to the host
 * @param num the number of pages
 * @return the number of pages taken from guest or < 0 on error
 */
int inflate_balloon(uintptr_t *pages_to_host, uint32_t num);

#endif /* _PLAT_DRV_BALLOON_H_ */
