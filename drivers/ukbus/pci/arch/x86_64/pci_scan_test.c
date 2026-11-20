/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/test.h>
#include "pci_scan.h"

struct fake_device {
	struct pci_address addr;
	__u8 config[256];
};

struct fixture {
	struct fake_device devices[8];
	struct pci_address found[16];
	unsigned int count, found_count, bad_access;
	unsigned int bus_scans[PCI_MAX_BUSES];
};

static struct fake_device *add_device(struct fixture *f, __u8 bus,
				      __u8 slot, __u8 function,
				      __u16 device_id, __u8 class,
				      __u8 subclass, __u8 header,
				      __u8 secondary)
{
	struct fake_device *d = &f->devices[f->count++];

	d->addr.bus = bus;
	d->addr.devid = slot;
	d->addr.function = function;
	d->config[0] = 0x86;
	d->config[1] = 0x80;
	d->config[2] = device_id & 0xff;
	d->config[3] = device_id >> 8;
	d->config[PCI_CLASS_OFFSET] = class;
	d->config[PCI_SUBCLASS_OFFSET] = subclass;
	d->config[PCI_HEADER_TYPE] = header;
	d->config[0x18] = bus;
	d->config[PCI_CONFIG_SECONDARY_BUS] = secondary;
	d->config[0x1a] = secondary;
	return d;
}

static int fake_read(void *ctx, const struct pci_address *addr,
		     __u16 offset, __u8 width, __u32 *value)
{
	struct fixture *f = ctx;
	unsigned int i, byte;

	if (addr->domain || addr->devid >= PCI_MAX_DEVICES ||
	    addr->function >= PCI_MAX_FUNCTIONS ||
	    (width != 1 && width != 2 && width != 4) ||
	    offset > 256 - width || (offset & (width - 1))) {
		f->bad_access++;
		return -EINVAL;
	}
	if (!addr->devid && !addr->function && !offset && width == 4)
		f->bus_scans[addr->bus]++;
	*value = width == 4 ? ~0U : (1U << (width * 8)) - 1;
	for (i = 0; i < f->count; i++) {
		struct fake_device *d = &f->devices[i];

		if (d->addr.bus != addr->bus || d->addr.devid != addr->devid ||
		    d->addr.function != addr->function)
			continue;
		*value = 0;
		for (byte = 0; byte < width; byte++)
			*value |= (__u32)d->config[offset + byte] << (8 * byte);
		break;
	}
	return 0;
}

static int fake_visit(void *ctx, const struct pci_address *addr,
		      const struct pci_device_id *id __unused)
{
	struct fixture *f = ctx;

	if (f->found_count == ARRAY_SIZE(f->found))
		return -ENOSPC;
	f->found[f->found_count++] = *addr;
	return 0;
}

static int found(struct fixture *f, __u8 bus, __u8 slot, __u8 function)
{
	unsigned int i;

	for (i = 0; i < f->found_count; i++)
		if (f->found[i].bus == bus && f->found[i].devid == slot &&
		    f->found[i].function == function)
			return 1;
	return 0;
}

static const struct pci_scan_ops fake_ops = {
	.read = fake_read,
	.visit = fake_visit
};

UK_TESTCASE(pci_scan, holes_and_multifunction)
{
	struct fixture f = { 0 };

	add_device(&f, 0, 0, 0, 0x29c0, 6, 0, 0, 0);
	add_device(&f, 0, 3, 0, 0x1000, 2, 0, 0x80, 0);
	add_device(&f, 0, 3, 3, 0x1001, 2, 0, 0, 0);
	add_device(&f, 0, 6, 0, 0x2000, 2, 0, 0, 0);
	UK_TEST_EXPECT_ZERO(pci_enumerate(&fake_ops, &f));
	UK_TEST_EXPECT_SNUM_EQ(f.found_count, 4);
	UK_TEST_EXPECT_SNUM_EQ(found(&f, 0, 3, 3), 1);
	UK_TEST_EXPECT_SNUM_EQ(found(&f, 0, 6, 0), 1);
	UK_TEST_EXPECT_ZERO(f.bad_access);
}

UK_TESTCASE(pci_scan, nested_bridges_and_cycle)
{
	struct fixture f = { 0 };

	add_device(&f, 0, 0, 0, 0x29c0, 6, 0, 0, 0);
	add_device(&f, 0, 30, 0, 0x244e, 6, 4, 1, 1);
	add_device(&f, 1, 1, 0, 0x0001, 6, 4, 1, 2);
	add_device(&f, 2, 18, 0, 0x1000, 2, 0, 0, 0);
	add_device(&f, 2, 20, 0, 0x0002, 6, 4, 1, 1);
	UK_TEST_EXPECT_ZERO(pci_enumerate(&fake_ops, &f));
	UK_TEST_EXPECT_SNUM_EQ(f.found_count, 5);
	UK_TEST_EXPECT_SNUM_EQ(found(&f, 2, 18, 0), 1);
	UK_TEST_EXPECT_SNUM_EQ(f.bus_scans[0], 1);
	UK_TEST_EXPECT_SNUM_EQ(f.bus_scans[1], 1);
	UK_TEST_EXPECT_SNUM_EQ(f.bus_scans[2], 1);
	UK_TEST_EXPECT_ZERO(f.bad_access);
}

UK_TESTCASE(pci_scan, host_controller_function_holes)
{
	struct fixture f = { 0 };

	add_device(&f, 0, 0, 0, 0x0001, 6, 0, 0x80, 0);
	add_device(&f, 0, 0, 2, 0x0002, 6, 0, 0, 0);
	add_device(&f, 2, 7, 0, 0x1000, 2, 0, 0, 0);
	UK_TEST_EXPECT_ZERO(pci_enumerate(&fake_ops, &f));
	UK_TEST_EXPECT_SNUM_EQ(f.found_count, 3);
	UK_TEST_EXPECT_SNUM_EQ(found(&f, 2, 7, 0), 1);
	UK_TEST_EXPECT_SNUM_EQ(f.bus_scans[2], 1);
	UK_TEST_EXPECT_ZERO(f.bad_access);
}

uk_testsuite_late_prio(pci_scan, NULL, 8);
