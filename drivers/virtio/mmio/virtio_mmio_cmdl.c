/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <uk/alloc.h>
#include <uk/bus/platform.h>
#include <uk/libparam.h>
#include <uk/essentials.h>

#include <virtio/virtio_mmio.h>

#define MAX_DEV_CMDLINE		CONFIG_VIRTIO_MMIO_MAX_DEV_CMDLINE

static struct uk_alloc *virtio_mmio_cmdl_alloc;

static char *uk_libparam_devices[MAX_DEV_CMDLINE];

UK_LIBPARAM_PARAM_ARR_ALIAS(device, uk_libparam_devices, charp,
			    MAX_DEV_CMDLINE, "virtio-mmio devices");

/* Helper to parse numeric string with size suffix (eg "4K") into numeric */
static char *parse_size(const char *size_str, __sz *sz)
{
	char *endptr;

	*sz = strtoull(size_str, &endptr, 0);

	switch (*endptr) {
	case 'g':
	case 'G':
		*sz *= 1024;
		__fallthrough;
	case 'm':
	case 'M':
		*sz *= 1024;
		__fallthrough;
	case 'k':
	case 'K':
		*sz *= 1024;
		++endptr;
		break;
	default:
		break;
	}

	return endptr;
}

/* Helper to parse device string formatted as <size>@<base>:<irq>[:<id>] */
static struct pf_device *uk_mmio_parse_dev(const char *str)
{
	int chunks = 0;
	struct pf_device *dev;

	dev = uk_calloc(virtio_mmio_cmdl_alloc, 1, sizeof(*dev));
	if (unlikely(!dev))
		return NULL;

	str = parse_size(str, &dev->size);
	if (unlikely(!dev->size)) {
		free(dev);
		return NULL;
	}

	/* Ignore the device id as it has no significance in Unikraft */
	chunks = sscanf(str, "@%" __PRIx64 ":%" __PRIu64, &dev->base,
			&dev->irq);

	if (unlikely(chunks < 2)) {
		free(dev);
		return NULL;
	}

	return dev;
}

static int virtio_mmio_cmdl_probe(void)
{
	int rc;
	const char *str;
	struct pf_device *dev;

	for (unsigned int i = 0; i < MAX_DEV_CMDLINE; i++) {
		str = ((char **)uk_libparam_devices)[i];
		if (!str)
			break;

		uk_pr_info("virtio-mmio device %s\n", str);

		dev = uk_mmio_parse_dev(str);
		if (unlikely(!dev)) {
			uk_pr_err("Could not parse device str\n");
			return -EINVAL;
		}

		rc = virtio_mmio_add_dev(dev);
		if (unlikely(rc)) {
			uk_pr_err("Could not add device (%d)\n", rc);
			free(dev);
			return rc;
		}
	}

	return 0;
}

static int virtio_mmio_cmdl_init(struct uk_alloc *alloc)
{
	UK_ASSERT(alloc);

	virtio_mmio_cmdl_alloc = alloc;

	return 0;
}

/* Register with uk_bus. We have to hook into a point where:
 *
 * 1. uklibparam has already parsed the cmdline.
 *
 * 2. virtio drivers have been initialized, as virtio_mmio->add_device
 *    expects that the corresponding virtio device is initialized.
 */
static struct uk_bus virtio_mmio_cmdl_ops = {
	.init = virtio_mmio_cmdl_init,
	.probe = virtio_mmio_cmdl_probe
};

UK_BUS_REGISTER_PRIORITY(&virtio_mmio_cmdl_ops, 2);
