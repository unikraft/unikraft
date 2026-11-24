/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include "pci_scan.h"

struct pci_scan {
	const struct pci_scan_ops *ops;
	void *ctx;
	unsigned int next, count;
	__u8 buses[PCI_MAX_BUSES];
	__u8 seen[PCI_MAX_BUSES];
};

static void queue_bus(struct pci_scan *scan, __u8 bus)
{
	if (!scan->seen[bus]) {
		scan->seen[bus] = 1;
		scan->buses[scan->count++] = bus;
	}
}

static int scan_function(struct pci_scan *scan, struct pci_address *addr,
			 int *present, __u8 *header)
{
	struct pci_device_id id = { 0 };
	__u32 value;
	int rc;

	*present = 0;
	rc = scan->ops->read(scan->ctx, addr, PCI_VENDOR_ID, 4, &value);
	if (rc)
		return rc;
	if ((value & 0xffff) == PCI_INVALID_ID)
		return 0;
	*present = 1;
	id.vendor_id = value & 0xffff;
	id.device_id = value >> 16;
	rc = scan->ops->read(scan->ctx, addr, PCI_CLASS_REVISION, 4, &value);
	if (rc)
		return rc;
	/* Keep the existing x86 driver-ID representation: base class << 8. */
	id.class_id = (value >> 16) & 0xff00;
	id.sub_class_id = (value >> 16) & 0xff;
	rc = scan->ops->read(scan->ctx, addr, PCI_HEADER_TYPE, 1, &value);
	if (rc)
		return rc;
	*header = value;
	id.subsystem_vendor_id = PCI_ANY_ID;
	id.subsystem_device_id = PCI_ANY_ID;
	if ((*header & 0x7f) == 0) {
		rc = scan->ops->read(scan->ctx, addr, PCI_SUBSYSTEM_VID, 4,
				     &value);
		if (rc)
			return rc;
		id.subsystem_vendor_id = value & 0xffff;
		id.subsystem_device_id = value >> 16;
	}
	rc = scan->ops->visit(scan->ctx, addr, &id);
	if (rc)
		return rc;
	if (id.class_id == 0x0600 && id.sub_class_id == 0x04 &&
	    (*header & 0x7f) == 1) {
		rc = scan->ops->read(scan->ctx, addr, PCI_CONFIG_SECONDARY_BUS,
				     1, &value);
		if (rc)
			return rc;
		queue_bus(scan, (__u8)value);
	}
	return 0;
}

int pci_enumerate(const struct pci_scan_ops *ops, void *ctx)
{
	struct pci_scan scan = { .ops = ops, .ctx = ctx };
	struct pci_address addr = { 0 };
	__u32 value;
	__u8 header;
	unsigned int function, device;
	int present, rc;

	if (!ops || !ops->read || !ops->visit)
		return -EINVAL;
	rc = ops->read(ctx, &addr, PCI_HEADER_TYPE, 1, &value);
	if (rc)
		return rc;
	if (value & PCI_HEADER_MULTI_FUNC) {
		for (function = 0; function < PCI_MAX_FUNCTIONS; function++) {
			addr.function = function;
			rc = ops->read(ctx, &addr, PCI_VENDOR_ID, 2, &value);
			if (rc)
				return rc;
			if (value != PCI_INVALID_ID)
				queue_bus(&scan, function);
		}
	} else {
		queue_bus(&scan, 0);
	}
	/* Visit each bus once using a bounded worklist, including cycles. */
	while (scan.next < scan.count) {
		addr.bus = scan.buses[scan.next++];
		for (device = 0; device < PCI_MAX_DEVICES; device++) {
			addr.devid = device;
			addr.function = 0;
			rc = scan_function(&scan, &addr, &present, &header);
			if (rc)
				return rc;
			if (!present || !(header & PCI_HEADER_MULTI_FUNC))
				continue;
			for (function = 1; function < PCI_MAX_FUNCTIONS;
			     function++) {
				addr.function = function;
				rc = scan_function(&scan, &addr,
						   &present, &header);
				if (rc)
					return rc;
			}
		}
	}
	return 0;
}
