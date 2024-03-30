/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/plat/common/bootinfo.h>

#if CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE
#include <uk/tty/pl011.h>
#endif /* CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE */

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
#include <uk/tty/ns16550.h>
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

/* KVM early init
 *
 * Initialize early devices and coalesce mrds.
 * TODO Replace with init-based callback registration.
 */
void ukplat_early_init(void)
{
	struct ukplat_bootinfo *bi;
	int rc __maybe_unused = 0;

#if CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE
	rc = ns16550_early_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize ns16550\n");
#endif /* CONFIG_LIBUKTTY_NS16550_EARLY_CONSOLE */

#if CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE
	rc = pl011_early_init();
	if (unlikely(rc))
		UK_CRASH("Could not initialize pl011\n");
#endif /* CONFIG_LIBUKTTY_PL011_EARLY_CONSOLE */

	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Could not obtain bootinfo\n");

	ukplat_memregion_list_coalesce(&bi->mrds);
}
