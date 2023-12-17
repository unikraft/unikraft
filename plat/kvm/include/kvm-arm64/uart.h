/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#if CONFIG_LIBUKTTY_NS16550
#include <uk/tty/ns16550.h>
#endif /* CONFIG_LIBUKTTY_NS16550 */

#if CONFIG_LIBUKTTY_PL011
#include <uk/tty/pl011.h>
#endif /* CONFIG_LIBUKTTY_PL011 */

static inline void kvm_console_init(void *fdt)
{
#if CONFIG_LIBUKTTY_NS16550
	ns16550_console_init(fdt);
#endif /* CONFIG_LIBUKTTY_NS16550 */

#if CONFIG_LIBUKTTY_PL011
	pl011_console_init(fdt);
#endif /* CONFIG_LIBUKTTY_PL011 */
}
