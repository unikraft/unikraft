/* SPDX-License-Identifier: BSD-3-Clause */
#include <uk/test.h>
#include "../pci_bus_private.h"

UK_TESTCASE(pci_bar, io_address_width)
{
	UK_TEST_EXPECT(pci_bar_io_size(0x0000ffe0U) == 32);
	UK_TEST_EXPECT(pci_bar_io_size(0xffffffe0U) == 32);
	UK_TEST_EXPECT(pci_bar_io_size(0x0000fffcU) == 4);
	UK_TEST_EXPECT(pci_bar_io_size(0xfffffffcU) == 4);
}

#if CONFIG_ARCH_X86_64
UK_TESTCASE(pci_bar, consumer_mapping)
{
	struct pci_driver drv = { .driver_maps_bars = 1 };
	struct pci_device dev = { .drv = &drv };

	/* An inaccessible, unused BAR0 must not gate the driver's probe. */
	dev.bar[0].type = PCI_BAR_IO;
	dev.bar[0].pbase = 0x10000;
	dev.bar[0].size = 32;
	UK_TEST_EXPECT(pci_device_prepare_base(&dev) == 0);
	UK_TEST_EXPECT(dev.base == 0 && dev.bar[0].vbase == 0);
	drv.driver_maps_bars = 0;
	UK_TEST_EXPECT(pci_device_prepare_base(&dev) == -ERANGE);
	dev.bar[0].pbase = 0x1000;
	UK_TEST_EXPECT(pci_device_prepare_base(&dev) == 0);
	UK_TEST_EXPECT(dev.base == 0x1000);
}
#endif /* CONFIG_ARCH_X86_64 */

uk_testsuite_late_prio(pci_bar, NULL, UK_PRIO_BEFORE(UK_PRIO_LATEST));
