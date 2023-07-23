/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, Xingjian Zhang <zhxj9823@qq.com>.
 * All rights reserved.
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

#if CONFIG_ARCH_ARM64
#include <libfdt.h>
#endif
#include <uk/console.h>
#include <uk/print.h>
#include <uk/assert.h>

static struct uk_console_ops *uk_console_ops;

int ukplat_coutk(const char *buf, unsigned int len)
{
	return uk_console_ops->coutk(buf, len);
}

int ukplat_coutd(const char *buf, unsigned int len)
{
	return uk_console_ops->coutd(buf, len);
}

int ukplat_cink(char *buf, unsigned int maxlen)
{
	return uk_console_ops->cink(buf, maxlen);
}

#if CONFIG_ARCH_X86_64
extern const struct uk_console_ops uk_console_kvm_ops;

void uk_console_init(void)
{
	uk_console_ops = &uk_console_kvm_ops;

	/* Initialize the corresponding console */
	uk_console_ops->init();

	uk_pr_info("Console init finished\n");
}
#else  /* !CONFIG_ARCH_X86_64 */
extern const struct uk_console_ops uk_console_pl011_ops;
extern const struct uk_console_ops uk_console_ns16550_ops;

void uk_console_init(const void *dtb)
{
	int offset;

	/* find the device by name */
	if (fdt_node_offset_by_compatible(dtb, -1, "arm,pl011") >= 0)
		uk_console_ops = &uk_console_pl011_ops;

	if ((offset = fdt_node_offset_by_compatible(dtb, -1, "ns16550")) >= 0
	    && (offset = fdt_node_offset_by_compatible(dtb, -1, "ns16550a"))
		   >= 0) {
		uk_console_ops = &uk_console_ns16550_ops;
	}

	if (!uk_console_ops)
		UK_CRASH("No console UART found!\n");

	/* Initialize the corresponding console */
	uk_console_ops->init(dtb);

	uk_pr_info("Console init finished\n");
}
#endif /* !CONFIG_ARCH_X86_64 */
